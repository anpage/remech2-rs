// Scales the game's framebuffer to the window with a "sharp bilinear" filter.
// Adapted from RetroArch's `sharp-bilinear-simple.slang`

struct Params {
    sizes: vec4<f32>,
    opts: vec4<f32>,
};

struct Palette {
    colors: array<vec4<f32>, 256>,
};

@group(0) @binding(0) var<uniform> params: Params;
@group(0) @binding(1) var indexed: texture_2d<u32>;
@group(0) @binding(2) var<uniform> palette: Palette;

struct VsOut {
    @builtin(position) pos: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> VsOut {
    let uv = vec2<f32>(f32((vertex_index << 1u) & 2u), f32(vertex_index & 2u));
    var out: VsOut;
    out.uv = uv;
    out.pos = vec4<f32>(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    return out;
}

fn sharp_bilinear_uv(uv: vec2<f32>) -> vec2<f32> {
    let source_size = params.sizes.xy;
    let output_size = params.sizes.zw;

    var scale = max(floor(output_size / source_size), vec2<f32>(1.0));
    if params.opts.x > 0.0 {
        scale = vec2<f32>(params.opts.x);
    }

    let texel = uv * source_size;
    let s = fract(texel);

    let region_range = vec2<f32>(0.5) - 0.5 / scale;

    let center_dist = s - 0.5;
    let f = (center_dist - clamp(center_dist, -region_range, region_range)) * scale + 0.5;

    return (floor(texel) + f) / source_size;
}

fn tap(coord: vec2<i32>, max_coord: vec2<i32>) -> vec4<f32> {
    let index = textureLoad(indexed, clamp(coord, vec2<i32>(0), max_coord), 0).r;
    return palette.colors[index];
}

fn sample_framebuffer(uv: vec2<f32>) -> vec4<f32> {
    let source_size = params.sizes.xy;
    let max_coord = vec2<i32>(source_size) - vec2<i32>(1);

    let point = sharp_bilinear_uv(uv) * source_size - 0.5;
    let base = floor(point);
    let weight = point - base;
    let coord = vec2<i32>(base);

    let top_left = tap(coord, max_coord);
    let top_right = tap(coord + vec2<i32>(1, 0), max_coord);
    let bottom_left = tap(coord + vec2<i32>(0, 1), max_coord);
    let bottom_right = tap(coord + vec2<i32>(1, 1), max_coord);

    let top = mix(top_left, top_right, weight.x);
    let bottom = mix(bottom_left, bottom_right, weight.x);
    return mix(top, bottom, weight.y);
}

fn linear_from_gamma_rgb(srgb: vec3<f32>) -> vec3<f32> {
    let cutoff = srgb < vec3<f32>(0.04045);
    let lower = srgb / vec3<f32>(12.92);
    let higher = pow((srgb + vec3<f32>(0.055)) / vec3<f32>(1.055), vec3<f32>(2.4));
    return select(higher, lower, cutoff);
}

@fragment
fn fs_gamma_target(in: VsOut) -> @location(0) vec4<f32> {
    return sample_framebuffer(in.uv);
}

@fragment
fn fs_srgb_target(in: VsOut) -> @location(0) vec4<f32> {
    let color = sample_framebuffer(in.uv);
    return vec4<f32>(linear_from_gamma_rgb(color.rgb), color.a);
}
