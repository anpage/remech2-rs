use std::num::NonZeroU64;

use egui::PaintCallbackInfo;
use egui_wgpu::{CallbackResources, CallbackTrait, RenderState, ScreenDescriptor};

use crate::settings::SETTINGS;

/// How the framebuffer is filtered when we scale it to the window size
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
pub enum ScalingMode {
    /// Plain bilinear.
    /// Smooth, but could be considered blurry.
    Bilinear,
    /// Integer prescale followed by a bilinear remainder.
    /// Sharp pixels without the artifacts of nearest-neighbor.
    #[default]
    SharpBilinear,
    /// Nearest-neighbor.
    /// Sharpest, but makes lines uneven and can introduce shimmering.
    Nearest,
}

impl ScalingMode {
    pub fn from_settings() -> Self {
        match SETTINGS
            .get(Some("video"), "scaling")
            .as_deref()
            .map(str::trim)
        {
            Some("bilinear") => Self::Bilinear,
            Some("nearest") => Self::Nearest,
            Some("sharp-bilinear") | None => Self::SharpBilinear,
            Some(other) => {
                tracing::warn!("unknown video.scaling value {other:?}, using sharp-bilinear");
                Self::SharpBilinear
            }
        }
    }

    /// Prescale factor to force on the shader.
    /// `0.0` means "derive it from the output size"
    fn prescale_override(self) -> f32 {
        match self {
            Self::Bilinear => 1.0,
            Self::SharpBilinear => 0.0,
            Self::Nearest => 64.0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, bytemuck::Pod, bytemuck::Zeroable)]
struct Params {
    /// xy = source size in texels, zw = output size in physical pixels
    sizes: [f32; 4],
    /// x = prescale override, yzw unused
    opts: [f32; 4],
}

/// 256 gamma-space RGBA colors, as the shader's uniform expects them
pub type PaletteData = [[f32; 4]; 256];

struct Target {
    texture: wgpu::Texture,
    bind_group: wgpu::BindGroup,
    size: [u32; 2],
}

/// GPU resources for the framebuffer blit
pub struct Scaler {
    pipeline: wgpu::RenderPipeline,
    bind_group_layout: wgpu::BindGroupLayout,
    params: wgpu::Buffer,
    palette: wgpu::Buffer,
    target: Option<Target>,
}

impl Scaler {
    pub fn new(render_state: &RenderState) -> Self {
        let device = &render_state.device;

        let module = device.create_shader_module(wgpu::include_wgsl!("sharp_bilinear.wgsl"));

        let bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("sharp_bilinear_bind_group_layout"),
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Buffer {
                        ty: wgpu::BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: NonZeroU64::new(size_of::<Params>() as u64),
                    },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        sample_type: wgpu::TextureSampleType::Uint,
                        view_dimension: wgpu::TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 2,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Buffer {
                        ty: wgpu::BufferBindingType::Uniform,
                        has_dynamic_offset: false,
                        min_binding_size: NonZeroU64::new(size_of::<PaletteData>() as u64),
                    },
                    count: None,
                },
            ],
        });

        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("sharp_bilinear_pipeline_layout"),
            bind_group_layouts: &[&bind_group_layout],
            push_constant_ranges: &[],
        });

        let target_format = render_state.target_format;
        let pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("sharp_bilinear_pipeline"),
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                module: &module,
                entry_point: Some("vs_main"),
                buffers: &[],
                compilation_options: Default::default(),
            },
            primitive: wgpu::PrimitiveState::default(),
            depth_stencil: None,
            multisample: wgpu::MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            fragment: Some(wgpu::FragmentState {
                module: &module,
                entry_point: Some(if target_format.is_srgb() {
                    "fs_srgb_target"
                } else {
                    "fs_gamma_target"
                }),
                targets: &[Some(wgpu::ColorTargetState {
                    format: target_format,
                    blend: None,
                    write_mask: wgpu::ColorWrites::ALL,
                })],
                compilation_options: Default::default(),
            }),
            multiview: None,
            cache: None,
        });

        let params = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("sharp_bilinear_params"),
            size: size_of::<Params>() as u64,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        let palette = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("sharp_bilinear_palette"),
            size: size_of::<PaletteData>() as u64,
            usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });

        Self {
            pipeline,
            bind_group_layout,
            params,
            palette,
            target: None,
        }
    }

    pub fn upload_palette(&self, render_state: &RenderState, palette: &PaletteData) {
        render_state
            .queue
            .write_buffer(&self.palette, 0, bytemuck::cast_slice(palette));
    }

    /// Copies this frame's palette indices into the framebuffer texture, reallocating it only when the game changes resolution.
    pub fn upload(&mut self, render_state: &RenderState, indices: &[u8], size: [usize; 2]) {
        let Some(indices) = indices.get(..size[0] * size[1]) else {
            tracing::warn!(
                "framebuffer is {} bytes, expected {}x{}",
                indices.len(),
                size[0],
                size[1]
            );
            return;
        };
        let size = [size[0] as u32, size[1] as u32];
        if size[0] == 0 || size[1] == 0 {
            return;
        }

        if self
            .target
            .as_ref()
            .is_none_or(|target| target.size != size)
        {
            self.target = Some(self.create_target(&render_state.device, size));
        }
        let Some(target) = &self.target else {
            return;
        };

        render_state.queue.write_texture(
            target.texture.as_image_copy(),
            indices,
            wgpu::TexelCopyBufferLayout {
                offset: 0,
                bytes_per_row: Some(size[0]),
                rows_per_image: Some(size[1]),
            },
            wgpu::Extent3d {
                width: size[0],
                height: size[1],
                depth_or_array_layers: 1,
            },
        );
    }

    fn create_target(&self, device: &wgpu::Device, size: [u32; 2]) -> Target {
        let texture = device.create_texture(&wgpu::TextureDescriptor {
            label: Some("game_framebuffer"),
            size: wgpu::Extent3d {
                width: size[0],
                height: size[1],
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::R8Uint,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            view_formats: &[],
        });

        let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("sharp_bilinear_bind_group"),
            layout: &self.bind_group_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: self.params.as_entire_binding(),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::TextureView(&view),
                },
                wgpu::BindGroupEntry {
                    binding: 2,
                    resource: self.palette.as_entire_binding(),
                },
            ],
        });

        Target {
            texture,
            bind_group,
            size,
        }
    }
}

/// Paint callback that blits the framebuffer over the rect egui allocated for it
pub struct SharpBilinear {
    /// Framebuffer size in texels
    pub source_size: [f32; 2],
    /// Size of the on-screen rect, in physical pixels
    pub output_size: [f32; 2],
    pub mode: ScalingMode,
}

impl CallbackTrait for SharpBilinear {
    fn prepare(
        &self,
        _device: &wgpu::Device,
        queue: &wgpu::Queue,
        _screen_descriptor: &ScreenDescriptor,
        _egui_encoder: &mut wgpu::CommandEncoder,
        callback_resources: &mut CallbackResources,
    ) -> Vec<wgpu::CommandBuffer> {
        if let Some(scaler) = callback_resources.get::<Scaler>() {
            let params = Params {
                sizes: [
                    self.source_size[0],
                    self.source_size[1],
                    self.output_size[0],
                    self.output_size[1],
                ],
                opts: [self.mode.prescale_override(), 0.0, 0.0, 0.0],
            };
            queue.write_buffer(&scaler.params, 0, bytemuck::bytes_of(&params));
        }
        Vec::new()
    }

    fn paint(
        &self,
        _info: PaintCallbackInfo,
        render_pass: &mut wgpu::RenderPass<'static>,
        callback_resources: &CallbackResources,
    ) {
        let Some(scaler) = callback_resources.get::<Scaler>() else {
            return;
        };
        let Some(target) = &scaler.target else {
            return;
        };

        render_pass.set_pipeline(&scaler.pipeline);
        render_pass.set_bind_group(0, &target.bind_group, &[]);
        render_pass.draw(0..3, 0..1);
    }
}
