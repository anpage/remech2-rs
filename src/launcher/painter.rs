//! Adapted from egui-wgpu's `winit.rs`

use std::{ffi::c_void, num::NonZeroU32};

use egui::{ViewportId, ViewportIdMap, ViewportIdSet};
use egui_wgpu::{RenderState, SurfaceErrorAction, WgpuConfiguration, WgpuError};
use raw_window_handle::WindowsDisplayHandle;
use tracing::{debug, warn};
use windows::Win32::{
    Foundation::{HWND, RECT},
    UI::WindowsAndMessaging::GetClientRect,
};

struct SurfaceState {
    surface: wgpu::Surface<'static>,
    alpha_mode: wgpu::CompositeAlphaMode,
    width: u32,
    height: u32,
}

/// Everything you need to paint egui with [`wgpu`] in a Win32 window.
///
/// NOTE: all egui viewports share the same painter.
pub struct Painter {
    configuration: WgpuConfiguration,
    msaa_samples: u32,
    support_transparent_backbuffer: bool,
    dithering: bool,
    depth_format: Option<wgpu::TextureFormat>,

    surfaces: ViewportIdMap<SurfaceState>,

    instance: wgpu::Instance,
    render_state: Option<RenderState>,

    // Per viewport/window:
    depth_texture_view: ViewportIdMap<wgpu::TextureView>,
    msaa_texture_view: ViewportIdMap<wgpu::TextureView>,
}

impl Painter {
    /// Manages [`wgpu`] state, including surface state, required to render egui.
    ///
    /// Only the [`wgpu::Instance`] is initialized here. Device selection and the initialization
    /// of render + surface state is deferred until the painter is given its first window target
    /// via [`set_window()`](Self::set_window). (Ensuring that a device that's compatible with the
    /// native window is chosen)
    ///
    /// Before calling [`paint_and_update_textures()`](Self::paint_and_update_textures) a
    /// [`wgpu::Surface`] must be initialized (and corresponding render state) by calling
    /// [`set_window()`](Self::set_window) once you have a valid window handle.
    pub async fn new(
        configuration: WgpuConfiguration,
        msaa_samples: u32,
        depth_format: Option<wgpu::TextureFormat>,
        support_transparent_backbuffer: bool,
        dithering: bool,
    ) -> Self {
        let instance = configuration.wgpu_setup.new_instance().await;

        Self {
            configuration,
            msaa_samples,
            support_transparent_backbuffer,
            dithering,
            depth_format,

            instance,
            render_state: None,

            depth_texture_view: Default::default(),
            surfaces: Default::default(),
            msaa_texture_view: Default::default(),
        }
    }

    /// Get the [`RenderState`].
    ///
    /// Will return [`None`] if the render state has not been initialized yet.
    pub fn render_state(&self) -> Option<RenderState> {
        self.render_state.clone()
    }

    fn configure_surface(
        surface_state: &SurfaceState,
        render_state: &RenderState,
        config: &WgpuConfiguration,
    ) {
        let width = surface_state.width;
        let height = surface_state.height;

        let mut surf_config = wgpu::SurfaceConfiguration {
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
            format: render_state.target_format,
            present_mode: config.present_mode,
            alpha_mode: surface_state.alpha_mode,
            view_formats: vec![render_state.target_format],
            ..surface_state
                .surface
                .get_default_config(&render_state.adapter, width, height)
                .expect("The surface isn't supported by this adapter")
        };

        if let Some(desired_maximum_frame_latency) = config.desired_maximum_frame_latency {
            surf_config.desired_maximum_frame_latency = desired_maximum_frame_latency;
        }

        surface_state
            .surface
            .configure(&render_state.device, &surf_config);
    }

    pub async unsafe fn set_window(
        &mut self,
        viewport_id: ViewportId,
        window: Option<&super::Window>,
    ) -> Result<(), WgpuError> { unsafe {
        if let Some(window) = window {
            let size = {
                let rect = &mut RECT::default();
                GetClientRect(HWND(window.hwnd.get() as *mut c_void), rect)
                    .expect("GetClientRect failed");
                (
                    (rect.right - rect.left) as u32,
                    (rect.bottom - rect.top) as u32,
                )
            };
            if !self.surfaces.contains_key(&viewport_id) {
                let surface = unsafe {
                    self.instance
                        .create_surface_unsafe(wgpu::SurfaceTargetUnsafe::RawHandle {
                            raw_display_handle: WindowsDisplayHandle::new().into(),
                            raw_window_handle: (*window).into(),
                        })?
                };
                self.add_surface(surface, viewport_id, size).await?;
            }
        } else {
            warn!("No window - clearing all surfaces");
            self.surfaces.clear();
        }
        Ok(())
    }}

    async fn add_surface(
        &mut self,
        surface: wgpu::Surface<'static>,
        viewport_id: ViewportId,
        size: (u32, u32),
    ) -> Result<(), WgpuError> {
        let render_state = if let Some(render_state) = &self.render_state {
            render_state
        } else {
            let render_state = RenderState::create(
                &self.configuration,
                &self.instance,
                Some(&surface),
                self.depth_format,
                self.msaa_samples,
                self.dithering,
            )
            .await?;
            self.render_state.get_or_insert(render_state)
        };
        let alpha_mode = if self.support_transparent_backbuffer {
            let supported_alpha_modes = surface.get_capabilities(&render_state.adapter).alpha_modes;

            // Prefer pre multiplied over post multiplied!
            if supported_alpha_modes.contains(&wgpu::CompositeAlphaMode::PreMultiplied) {
                wgpu::CompositeAlphaMode::PreMultiplied
            } else if supported_alpha_modes.contains(&wgpu::CompositeAlphaMode::PostMultiplied) {
                wgpu::CompositeAlphaMode::PostMultiplied
            } else {
                warn!("Transparent window was requested, but the active wgpu surface does not support a `CompositeAlphaMode` with transparency.");
                wgpu::CompositeAlphaMode::Auto
            }
        } else {
            wgpu::CompositeAlphaMode::Auto
        };
        self.surfaces.insert(
            viewport_id,
            SurfaceState {
                surface,
                width: size.0,
                height: size.1,
                alpha_mode,
            },
        );
        let Some(width) = NonZeroU32::new(size.0) else {
            debug!("The window width was zero; skipping generate textures");
            return Ok(());
        };
        let Some(height) = NonZeroU32::new(size.1) else {
            debug!("The window height was zero; skipping generate textures");
            return Ok(());
        };
        self.resize_and_generate_depth_texture_view_and_msaa_view(viewport_id, width, height);
        Ok(())
    }

    /// Returns the maximum texture dimension supported if known
    ///
    /// This API will only return a known dimension after `set_window()` has been called
    /// at least once, since the underlying device and render state are initialized lazily
    /// once we have a window (that may determine the choice of adapter/device).
    pub fn max_texture_side(&self) -> Option<usize> {
        self.render_state
            .as_ref()
            .map(|rs| rs.device.limits().max_texture_dimension_2d as usize)
    }

    fn resize_and_generate_depth_texture_view_and_msaa_view(
        &mut self,
        viewport_id: ViewportId,
        width_in_pixels: NonZeroU32,
        height_in_pixels: NonZeroU32,
    ) {
        let width = width_in_pixels.get();
        let height = height_in_pixels.get();

        let render_state = self.render_state.as_ref().unwrap();
        let surface_state = self.surfaces.get_mut(&viewport_id).unwrap();

        surface_state.width = width;
        surface_state.height = height;

        Self::configure_surface(surface_state, render_state, &self.configuration);

        if let Some(depth_format) = self.depth_format {
            self.depth_texture_view.insert(
                viewport_id,
                render_state
                    .device
                    .create_texture(&wgpu::TextureDescriptor {
                        label: Some("egui_depth_texture"),
                        size: wgpu::Extent3d {
                            width,
                            height,
                            depth_or_array_layers: 1,
                        },
                        mip_level_count: 1,
                        sample_count: self.msaa_samples,
                        dimension: wgpu::TextureDimension::D2,
                        format: depth_format,
                        usage: wgpu::TextureUsages::RENDER_ATTACHMENT
                            | wgpu::TextureUsages::TEXTURE_BINDING,
                        view_formats: &[depth_format],
                    })
                    .create_view(&wgpu::TextureViewDescriptor::default()),
            );
        }

        if let Some(render_state) = (self.msaa_samples > 1)
            .then_some(self.render_state.as_ref())
            .flatten()
        {
            let texture_format = render_state.target_format;
            self.msaa_texture_view.insert(
                viewport_id,
                render_state
                    .device
                    .create_texture(&wgpu::TextureDescriptor {
                        label: Some("egui_msaa_texture"),
                        size: wgpu::Extent3d {
                            width,
                            height,
                            depth_or_array_layers: 1,
                        },
                        mip_level_count: 1,
                        sample_count: self.msaa_samples,
                        dimension: wgpu::TextureDimension::D2,
                        format: texture_format,
                        usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
                        view_formats: &[texture_format],
                    })
                    .create_view(&wgpu::TextureViewDescriptor::default()),
            );
        };
    }

    pub fn on_window_resized(
        &mut self,
        viewport_id: ViewportId,
        width_in_pixels: NonZeroU32,
        height_in_pixels: NonZeroU32,
    ) {
        if self.surfaces.contains_key(&viewport_id) {
            self.resize_and_generate_depth_texture_view_and_msaa_view(
                viewport_id,
                width_in_pixels,
                height_in_pixels,
            );
        } else {
            warn!("Ignoring window resize notification with no surface created via Painter::set_window()");
        }
    }

    /// Returns the approximate number of seconds spent on vsync-waiting (if any)
    pub fn paint_and_update_textures(
        &mut self,
        viewport_id: ViewportId,
        pixels_per_point: f32,
        clear_color: [f32; 4],
        clipped_primitives: &[egui::ClippedPrimitive],
        textures_delta: &egui::TexturesDelta,
    ) -> f32 {
        let mut vsync_sec = 0.0;

        let Some(render_state) = self.render_state.as_mut() else {
            return vsync_sec;
        };
        let Some(surface_state) = self.surfaces.get(&viewport_id) else {
            return vsync_sec;
        };

        let mut encoder =
            render_state
                .device
                .create_command_encoder(&wgpu::CommandEncoderDescriptor {
                    label: Some("encoder"),
                });

        // Upload all resources for the GPU.
        let screen_descriptor = egui_wgpu::ScreenDescriptor {
            size_in_pixels: [surface_state.width, surface_state.height],
            pixels_per_point,
        };

        let user_cmd_bufs = {
            let mut renderer = render_state.renderer.write();
            for (id, image_delta) in &textures_delta.set {
                renderer.update_texture(
                    &render_state.device,
                    &render_state.queue,
                    *id,
                    image_delta,
                );
            }

            renderer.update_buffers(
                &render_state.device,
                &render_state.queue,
                &mut encoder,
                clipped_primitives,
                &screen_descriptor,
            )
        };

        let output_frame = {
            // This is what vsync-waiting happens on my Mac.
            let start = web_time::Instant::now();
            let output_frame = surface_state.surface.get_current_texture();
            vsync_sec += start.elapsed().as_secs_f32();
            output_frame
        };

        let output_frame = match output_frame {
            Ok(frame) => frame,
            Err(err) => match (*self.configuration.on_surface_error)(err) {
                SurfaceErrorAction::RecreateSurface => {
                    Self::configure_surface(surface_state, render_state, &self.configuration);
                    return vsync_sec;
                }
                SurfaceErrorAction::SkipFrame => {
                    return vsync_sec;
                }
            },
        };

        {
            let renderer = render_state.renderer.read();
            let frame_view = output_frame
                .texture
                .create_view(&wgpu::TextureViewDescriptor::default());

            let (view, resolve_target) = (self.msaa_samples > 1)
                .then_some(self.msaa_texture_view.get(&viewport_id))
                .flatten()
                .map_or((&frame_view, None), |texture_view| {
                    (texture_view, Some(&frame_view))
                });

            let render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("egui_render"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view,
                    resolve_target,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: clear_color[0] as f64,
                            g: clear_color[1] as f64,
                            b: clear_color[2] as f64,
                            a: clear_color[3] as f64,
                        }),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: self.depth_texture_view.get(&viewport_id).map(|view| {
                    wgpu::RenderPassDepthStencilAttachment {
                        view,
                        depth_ops: Some(wgpu::Operations {
                            load: wgpu::LoadOp::Clear(1.0),
                            // It is very unlikely that the depth buffer is needed after egui finished rendering
                            // so no need to store it. (this can improve performance on tiling GPUs like mobile chips or Apple Silicon)
                            store: wgpu::StoreOp::Discard,
                        }),
                        stencil_ops: None,
                    }
                }),
                timestamp_writes: None,
                occlusion_query_set: None,
            });

            // Forgetting the pass' lifetime means that we are no longer compile-time protected from
            // runtime errors caused by accessing the parent encoder before the render pass is dropped.
            // Since we don't pass it on to the renderer, we should be perfectly safe against this mistake here!
            renderer.render(
                &mut render_pass.forget_lifetime(),
                clipped_primitives,
                &screen_descriptor,
            );
        }

        let encoded = { encoder.finish() };

        // Submit the commands: both the main buffer and user-defined ones.
        {
            // wgpu doesn't document where vsync can happen. Maybe here?
            let start = web_time::Instant::now();
            render_state
                .queue
                .submit(user_cmd_bufs.into_iter().chain([encoded]));
            vsync_sec += start.elapsed().as_secs_f32();
        };

        // Free textures marked for destruction **after** queue submit since they might still be used in the current frame.
        // Calling `wgpu::Texture::destroy` on a texture that is still in use would invalidate the command buffer(s) it is used in.
        // However, once we called `wgpu::Queue::submit`, it is up for wgpu to determine how long the underlying gpu resource has to live.
        {
            let mut renderer = render_state.renderer.write();
            for id in &textures_delta.free {
                renderer.free_texture(id);
            }
        }

        {
            // wgpu doesn't document where vsync can happen. Maybe here?
            let start = web_time::Instant::now();
            output_frame.present();
            vsync_sec += start.elapsed().as_secs_f32();
        }

        vsync_sec
    }

    pub fn gc_viewports(&mut self, active_viewports: &ViewportIdSet) {
        self.surfaces.retain(|id, _| active_viewports.contains(id));
        self.depth_texture_view
            .retain(|id, _| active_viewports.contains(id));
        self.msaa_texture_view
            .retain(|id, _| active_viewports.contains(id));
    }

    #[expect(clippy::needless_pass_by_ref_mut, clippy::unused_self)]
    pub fn destroy(&mut self) {
        // TODO(emilk): something here?
    }
}
