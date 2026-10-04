use std::sync::Arc;

use anyhow::{Context, Result};
use egui::{ClippedPrimitive, TexturesDelta};
use egui_wgpu::{RendererOptions, ScreenDescriptor};
use winit::window::Window;

use crate::drawmode::{
    ScalingMode, frame_rect,
    scaler::{PaletteData, Scaler},
};

/// A frame of the game's 8-bit output
pub struct Frame<'a> {
    pub pixels: &'a [u8],
    pub size: [usize; 2],
    pub source: Option<[usize; 4]>,
    pub palette: &'a PaletteData,
}

/// The window's wgpu surface and the pass that draws the game's 8-bit frame
pub struct Renderer {
    surface: wgpu::Surface<'static>,
    device: wgpu::Device,
    queue: wgpu::Queue,
    config: wgpu::SurfaceConfiguration,
    scaler: Scaler,
    scaling: ScalingMode,
    egui: egui_wgpu::Renderer,
}

impl Renderer {
    pub fn new(window: Arc<Window>) -> Result<Self> {
        let size = window.inner_size();

        let instance = wgpu::Instance::new(&wgpu::InstanceDescriptor::default());
        let surface = instance.create_surface(window)?;
        let adapter = pollster::block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
            compatible_surface: Some(&surface),
            ..Default::default()
        }))?;
        let (device, queue) =
            pollster::block_on(adapter.request_device(&wgpu::DeviceDescriptor::default()))?;

        let mut config = surface
            .get_default_config(&adapter, size.width.max(1), size.height.max(1))
            .context("the adapter cannot present to the window")?;
        // The palette is in gamma space
        let formats = surface.get_capabilities(&adapter).formats;
        if let Some(format) = formats.iter().find(|format| !format.is_srgb()) {
            config.format = *format;
        }
        config.present_mode = wgpu::PresentMode::AutoVsync;
        surface.configure(&device, &config);

        let scaler = Scaler::new(&device, config.format);
        let egui = egui_wgpu::Renderer::new(
            &device,
            config.format,
            RendererOptions {
                msaa_samples: 1,
                depth_stencil_format: None,
                dithering: false,
                predictable_texture_filtering: false,
            },
        );

        Ok(Self {
            surface,
            device,
            queue,
            config,
            scaler,
            scaling: ScalingMode::from_settings(),
            egui,
        })
    }

    /// Called when the window's size in physical pixels changes
    pub fn resize(&mut self, width: u32, height: u32) {
        // Minimized
        if width == 0 || height == 0 {
            return;
        }
        self.config.width = width;
        self.config.height = height;
        self.surface.configure(&self.device, &self.config);
    }

    /// The largest texture egui can allocate
    pub fn max_texture_side(&self) -> usize {
        self.device.limits().max_texture_dimension_2d as usize
    }

    /// Scales the game's frame to the largest rect of its shape that fits the window, draws egui's output over it, and presents
    pub fn render(
        &mut self,
        frame: Option<Frame>,
        primitives: &[ClippedPrimitive],
        textures_delta: &TexturesDelta,
        pixels_per_point: f32,
    ) {
        let surface_texture = match self.surface.get_current_texture() {
            Ok(surface_texture) => surface_texture,
            Err(wgpu::SurfaceError::Lost | wgpu::SurfaceError::Outdated) => {
                self.surface.configure(&self.device, &self.config);
                return;
            }
            Err(error) => {
                tracing::warn!("couldn't acquire the next frame: {error}");
                return;
            }
        };

        let window_size = [self.config.width as f32, self.config.height as f32];
        let output_rect = frame
            .as_ref()
            .and_then(|frame| frame_rect(window_size, frame.size));
        if let (Some(frame), Some((_, output_size))) = (&frame, output_rect) {
            let [x, y, width, height] =
                frame.source.unwrap_or([0, 0, frame.size[0], frame.size[1]]);
            self.scaler.upload_palette(&self.queue, frame.palette);
            self.scaler.upload(
                &self.device,
                &self.queue,
                frame.pixels,
                frame.size[0],
                [x, y],
                [width, height],
            );
            self.scaler.set_params(
                &self.queue,
                [width as f32, height as f32],
                output_size,
                self.scaling,
            );
        }

        let screen_descriptor = ScreenDescriptor {
            size_in_pixels: [self.config.width, self.config.height],
            pixels_per_point,
        };

        let view = surface_texture
            .texture
            .create_view(&wgpu::TextureViewDescriptor::default());
        let mut encoder = self
            .device
            .create_command_encoder(&wgpu::CommandEncoderDescriptor::default());

        for (id, delta) in &textures_delta.set {
            self.egui
                .update_texture(&self.device, &self.queue, *id, delta);
        }
        let mut command_buffers = self.egui.update_buffers(
            &self.device,
            &self.queue,
            &mut encoder,
            primitives,
            &screen_descriptor,
        );

        {
            let render_pass = encoder.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("main_pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &view,
                    depth_slice: None,
                    resolve_target: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color::BLACK),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                timestamp_writes: None,
                occlusion_query_set: None,
            });
            // egui's renderer requires it
            let mut render_pass = render_pass.forget_lifetime();
            if let Some((origin, size)) = output_rect {
                render_pass.set_viewport(origin[0], origin[1], size[0], size[1], 0.0, 1.0);
                self.scaler.draw(&mut render_pass);
            }
            self.egui
                .render(&mut render_pass, primitives, &screen_descriptor);
        }

        command_buffers.push(encoder.finish());
        self.queue.submit(command_buffers);
        surface_texture.present();

        for id in &textures_delta.free {
            self.egui.free_texture(id);
        }
    }
}
