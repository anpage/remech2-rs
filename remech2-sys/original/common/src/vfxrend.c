/* VFXREND, Miles Design VFX's polygon renderer (3rdparty/vfx/VFXREND.ASM), in portable C, which
   both DLLs compile in place of the assembly.

   VFXREND builds its primitives from one macro, MAKE_POLY, for each set of operation flags
   RENDOPTS.INC lists (MW2's 25, g_primitives), and dispatches on the flags through a table. The C
   has one renderer instead, whose passes the flags select as MAKE_POLY does: a background fill
   (solid, Gouraud or indirect Gouraud), a texture map (affine or perspective, through a flat
   lookaside table, sampled at every pixel or every other, tiled or not, with transparency), and a
   pass over the pixels already there (flat or Gouraud lookaside). Operations that RENDOPTS.INC
   doesn't build have no entry in the table, and draw nothing.

   VFX_polygon_render doesn't clip: the polygon must lie in the window. It walks the polygon's left
   and right edges down from its top vertex, one scan line at a time, in 16.16 with a half added to
   x, and draws the span between them. The C does the assembly's arithmetic: 32-bit sums that
   wrap, slopes from a 64-bit idiv, Gouraud shades that only carry within the 24 bits a color byte
   and its fraction take, texture coordinates in 32.32 (the integer in edx or ecx, the fraction in
   esi or ebp), perspective in segments of 16 pixels with a divide at each end.

   The primitives patch their own operands: the addresses of the texture's row table and of the
   lookaside tables, which each call compares with the ones it is passed and rewrites if they
   differ. They always use the current call's, so the patching can't be observed (but in the code
   itself), and the C reads the arguments instead. GetCodeBlock returns the range a caller must
   make writable for it, which the C has no need of: it returns its working vertices'.

   What a call leaves for later ones stays global: the dither levels, and the working vertices of
   the clipper. VFX_polygon_clip_XY_and_render clips the polygon in turn against the pane's right,
   bottom, left and top, alternating between the caller's vertex list and its own, and writes the
   clipped vertices' words only as far as the operation interpolates them: the others keep what the
   list held, and that leaks into the caller's list with the copies of later planes. The C keeps
   the working vertices for this. Out of domain: the divisions that fault (an edge or span of
   0x10000 lines or pixels, a perspective divisor of 0 or a quotient that overflows), an operation
   outside the table, an empty vertex list, a clipped polygon of more than MAX_WORK_VERTICES
   vertices, and a Gouraud fill through p_cueing by a color above 0xffff (beyond the table). */
#include "vfxrend.h"

#include "decomp.h"
#include "pane.h"
#include "portable.h"
#include "types.h"
#include "window.h"

#include <stddef.h>
#include <string.h>

#define VERTEX_WORDS 6
#define MAX_WORK_VERTICES 256
#define OPERATION_COUNT 0x800
#define TRANSPARENT_TEXEL 0xff
#define SEGMENT_SHIFT 4 // perspective segments of 16 pixels
#define SEGMENT_PIXELS (1 << SEGMENT_SHIFT)

// The vertex words (VFX_VERTEX).
enum VertexWord {
	c_vertexX,
	c_vertexY,
	c_vertexColor, // 16.16
	c_vertexU,     // 16.16, times w for perspective
	c_vertexV,
	c_vertexW // 2.30
};

// The operation flags, VFXREND.ASM's (VFXREND.H swaps the two Gouraud transparencies).
enum OperationFlag {
	c_mapIllum = 0x000,       // the pixels already there, shaded
	c_mapSolid = 0x200,       // a fill
	c_mapAffine = 0x400,      // a texture, interpolated linearly
	c_mapPerspective = 0x600, // a texture, perspective-correct
	c_mapMask = 0x600,
	c_shadeFlat = 0x080,     // through a 256-byte lookaside table
	c_shadeGouraud = 0x040,  // through a 256 by 256 table, by the Gouraud shade
	c_shadeIndirect = 0x0c0, // a Gouraud fill through the translucency table
	c_shadeRange = 0x100,    // a Gouraud fill of the shades themselves
	c_shadeMask = 0x1c0,
	c_sampleCoarse = 0x020, // a texel for every other pixel
	c_tileLog = 0x010,      // texture coordinates masked by the texture's size less one
	c_tileLinear = 0x008,   // texture coordinates modulo its size
	c_tileMask = 0x018,
	c_transparentSkip = 0x001,     // texels 0xff aren't drawn
	c_transparentRange = 0x007,    // nor are they, over a range Gouraud fill
	c_transparentIndirect = 0x004, // nor are they, over an indirect Gouraud fill
	c_transparentMask = 0x007
};

// The operations RENDOPTS.INC builds, in its order.
static const MechU16 g_primitives[] = {
	0x691, 0x611, 0x491, 0x411, 0x6a0, 0x680, 0x697, 0x689, 0x6b1, 0x690, 0x600, 0x610, 0x650,
	0x497, 0x490, 0x400, 0x410, 0x408, 0x450, 0x080, 0x280, 0x040, 0x300, 0x2c0, 0x240,
};

// VFX_set_Gouraud_dither_level's levels (dfactor_1, dfactor_2), which each Gouraud pass adds to
// the shades of a scan line's even and odd pixels, swapping them every line.
static MechU32 g_ditherLevels[2] = {0x8000, 0};

// The clipper's working vertices (work_pts).
static MechU32 g_workPoints[MAX_WORK_VERTICES * VERTEX_WORDS];

// The passes MAKE_POLY assembles for a scan line, in order.
enum Pass {
	c_passNone,
	c_passRangeFill,    // R_GOURAUD_SOLID
	c_passIndirectFill, // I_GOURAUD_SOLID
	c_passCueFill,      // M_GOURAUD_SOLID: the polygon's color, Gouraud shaded through p_cueing
	c_passSolidFill,    // FLAT_SOLID
	c_passTexture,      // AFFINE_MAP, PERSPECTIVE_MAP
	c_passGouraudCue,   // GOURAUD_CUE
	c_passFlatCue       // FLAT_CUE
};

// One side of the polygon: an edge, and where the walk along it has got to.
typedef struct RenderEdge {
	const MechU32* m_next;          // the vertex at the edge's end
	MechU32 m_count;                // the scan lines left on the edge
	MechU32 m_values[VERTEX_WORDS]; // by vertex word (x in 16.16, with a half)
	MechU32 m_steps[VERTEX_WORDS];  // per scan line
} RenderEdge;

typedef struct Render {
	MechU8* m_buffer;
	MechU32 m_pitch;
	MechU32 m_row; // the scan line's offset in m_buffer
	const MechU32* m_vertices;
	const MechU32* m_verticesEnd;
	MechS32 m_operation;
	MechU32 m_color;
	const VFX_TEXTURE* m_texture;
	const MechU8* m_cueing;
	const MechU8* m_translucency;
	MechU32 m_dither[2];
	MechS32 m_passes[3];
	MechS32 m_words[4]; // the vertex words the edges interpolate besides x
	MechS32 m_wordCount;
	RenderEdge m_left;  // walks the vertices backwards
	RenderEdge m_right; // forwards
} Render;

// `sar 16`: a 16.16 value's integer part.
static MechS32 Integer(MechU32 p_value)
{
	return PortableSar32(PortableS32(p_value), 16);
}

// FIXDIV and FPDIV: a 16.16 value over a count, as p_delta shifted into a 64-bit dividend over
// p_count shifted into a 32-bit divisor (which a count of 0x10000 makes 0).
static MechU32 Slope(MechU32 p_delta, MechU32 p_count)
{
	return (MechU32) PortableIdiv((MechS64) PortableS32(p_delta) * 0x10000, PortableS32(p_count << 16));
}

// FPDIVONE: a perspective coordinate (16.16, times w) over w (2.30).
static MechU32 DivideOne(MechU32 p_value, MechU32 p_w)
{
	return (MechU32) PortableIdiv((MechS64) PortableS32(p_value) * 0x40000000, PortableS32(p_w));
}

// The quotient of two magnitudes, in 32 bits, rounded half up: the remainder carries when it is
// at least half the divisor (rounded up).
static MechU32 DivideRounded(MechU64 p_dividend, MechU32 p_divisor)
{
	MechU32 quotient = PortableDiv(p_dividend, p_divisor);
	MechU32 remainder = (MechU32) (p_dividend - (MechU64) quotient * p_divisor);

	if ((p_divisor >> 1) + (p_divisor & 1) - 1 < remainder) {
		quotient++;
	}

	return quotient;
}

static MechU32 Magnitude(MechU32 p_value, MechS32* p_negative, MechS32 p_sign)
{
	if (PortableS32(p_value) < 0) {
		*p_negative += p_sign;
		return 0 - p_value;
	}

	return p_value;
}

// F16_div_to_F30: the magnitudes' quotient with the dividend shifted up by 30 (`sar 2` into the
// high word, so a dividend of 0x80000000 faults), negated if the signs differ.
static MechU32 DivideToF30(MechU32 p_dividend, MechU32 p_divisor)
{
	MechS32 negative = 0;
	MechU32 dividend = Magnitude(p_dividend, &negative, 1);
	MechU32 divisor = Magnitude(p_divisor, &negative, -1);
	MechU64 wide = (MechU64) (MechU32) PortableSar32(PortableS32(dividend), 2) << 32 | (MechU32) (dividend << 30);
	MechU32 quotient = DivideRounded(wide, divisor);

	return negative ? 0 - quotient : quotient;
}

// MMUL_F30: the product of p_value and the 2.30 p_factor, rounded, shifted down by 30.
static MechU32 MultiplyF30(MechU32 p_value, MechU32 p_factor)
{
	MechU64 product = (MechU64) ((MechS64) PortableS32(p_value) * PortableS32(p_factor));

	return (MechU32) ((product + 0x20000000) >> 30);
}

// --- The entry points ---

void VFX_set_Gouraud_dither_level(MechS32 p_dither1, MechS32 p_dither2)
{
	g_ditherLevels[0] = (MechU32) p_dither1;
	g_ditherLevels[1] = (MechU32) p_dither2;
}

// The assembly returns its code segment's selector, and the range of its primitives and its size;
// the C patches no code, and returns its working vertices instead, which the caller can make
// writable all the same. The start is a 32-bit address, as the game's caller takes it.
MechS32 GetCodeBlock(undefined4* p_start, undefined4* p_selector)
{
	*p_selector = 0;
	*p_start = (undefined4) (size_t) g_workPoints;
	return (MechS32) sizeof(g_workPoints);
}

MechS32 F16_div_to_F30(MechS32 p_dividend, MechS32 p_divisor)
{
	return PortableS32(DivideToF30((MechU32) p_dividend, (MechU32) p_divisor));
}

// 1.0 (16.16) over p_value: 0x4000 in the dividend's high word.
MechS32 F30_reciprocal(MechS32 p_value)
{
	MechS32 negative = 0;
	MechU32 divisor = Magnitude((MechU32) p_value, &negative, -1);
	MechU32 quotient = DivideRounded((MechU64) 0x4000 << 32, divisor);

	return PortableS32(negative ? 0 - quotient : quotient);
}

MechS32 mul_F30(MechS32 p_m1, MechS32 p_m2)
{
	return PortableS32(MultiplyF30((MechU32) p_m1, (MechU32) p_m2));
}

// --- Scan conversion ---

static MechS32 IsBuilt(MechS32 p_operation)
{
	MechS32 i;

	for (i = 0; i < (MechS32) (sizeof(g_primitives) / sizeof(g_primitives[0])); i++) {
		if (g_primitives[i] == p_operation) {
			return 1;
		}
	}

	return 0;
}

static void AddWord(Render* p_render, MechS32 p_word)
{
	p_render->m_words[p_render->m_wordCount++] = p_word;
}

// The passes and the edges' words of an operation, as MAKE_POLY selects them.
static void SelectPasses(Render* p_render)
{
	MechS32 map = p_render->m_operation & c_mapMask;
	MechS32 shade = p_render->m_operation & c_shadeMask;
	MechS32 transparency = p_render->m_operation & c_transparentMask;
	MechS32 useColor = 0;

	p_render->m_passes[0] = c_passNone;
	p_render->m_passes[1] = c_passNone;
	p_render->m_passes[2] = c_passNone;
	p_render->m_wordCount = 0;
	if (map == c_mapAffine || map == c_mapPerspective) {
		p_render->m_passes[1] = c_passTexture;
		AddWord(p_render, c_vertexU);
		AddWord(p_render, c_vertexV);
		if (map == c_mapPerspective) {
			AddWord(p_render, c_vertexW);
		}
	}

	if (transparency == c_transparentRange) {
		p_render->m_passes[0] = c_passRangeFill;
		useColor = 1;
	}
	else if (transparency == c_transparentIndirect) {
		p_render->m_passes[0] = c_passIndirectFill;
		useColor = 1;
	}

	if (map == c_mapSolid) {
		if (shade == c_shadeGouraud) {
			p_render->m_passes[0] = c_passCueFill;
			useColor = 1;
		}
		else if (shade == c_shadeIndirect) {
			p_render->m_passes[0] = c_passIndirectFill;
			useColor = 1;
		}
		else if (shade == c_shadeRange) {
			p_render->m_passes[0] = c_passRangeFill;
			useColor = 1;
		}
		else if (shade == c_shadeFlat) {
			p_render->m_passes[0] = c_passSolidFill;
		}
	}
	else if (shade == c_shadeGouraud) {
		p_render->m_passes[2] = c_passGouraudCue;
		useColor = 1;
	}
	else if (shade == c_shadeFlat && map == c_mapIllum) {
		p_render->m_passes[2] = c_passFlatCue;
	}

	if (useColor) {
		AddWord(p_render, c_vertexColor);
	}
}

static const MechU32* PreviousVertex(const Render* p_render, const MechU32* p_vertex)
{
	if (p_vertex == p_render->m_vertices) {
		p_vertex = p_render->m_verticesEnd;
	}

	return p_vertex - VERTEX_WORDS;
}

static const MechU32* NextVertex(const Render* p_render, const MechU32* p_vertex)
{
	p_vertex += VERTEX_WORDS;
	if (p_vertex == p_render->m_verticesEnd) {
		p_vertex = p_render->m_vertices;
	}

	return p_vertex;
}

// LEFT_DELTAS and RIGHT_DELTAS: starts an edge at its top vertex, towards m_next.
static void StartEdge(const Render* p_render, RenderEdge* p_edge, const MechU32* p_start, MechU32 p_count)
{
	const MechU32* end = p_edge->m_next;
	MechS32 i;

	p_edge->m_count = p_count;
	p_edge->m_steps[c_vertexX] = Slope((end[c_vertexX] - p_start[c_vertexX]) << 16, p_count);
	p_edge->m_values[c_vertexX] = (p_start[c_vertexX] << 16) + 0x8000;
	for (i = 0; i < p_render->m_wordCount; i++) {
		MechS32 word = p_render->m_words[i];

		p_edge->m_steps[word] = Slope(end[word] - p_start[word], p_count);
		p_edge->m_values[word] = p_start[word];
	}
}

// Moves an edge to the one after it along its walk; returns its top vertex.
static const MechU32* AdvanceEdge(const Render* p_render, RenderEdge* p_edge)
{
	const MechU32* start = p_edge->m_next;

	if (p_edge == &p_render->m_left) {
		p_edge->m_next = PreviousVertex(p_render, start);
	}
	else {
		p_edge->m_next = NextVertex(p_render, start);
	}

	return start;
}

// The first edge on a side that isn't horizontal.
static void StartFirstEdge(const Render* p_render, RenderEdge* p_edge)
{
	const MechU32* start;
	MechU32 count;

	do {
		start = AdvanceEdge(p_render, p_edge);
		count = p_edge->m_next[c_vertexY] - start[c_vertexY];
	} while (!count);

	StartEdge(p_render, p_edge, start, count);
}

// The next edge, when the scan lines reach the end of one: a horizontal one for a line.
static void StartNextEdge(const Render* p_render, RenderEdge* p_edge)
{
	const MechU32* start = AdvanceEdge(p_render, p_edge);
	MechU32 count = p_edge->m_next[c_vertexY] - start[c_vertexY];

	StartEdge(p_render, p_edge, start, count ? count : 1);
}

// LEFT_TRACE and RIGHT_TRACE.
static void StepEdge(const Render* p_render, RenderEdge* p_edge)
{
	MechS32 i;

	p_edge->m_values[c_vertexX] += p_edge->m_steps[c_vertexX];
	for (i = 0; i < p_render->m_wordCount; i++) {
		MechS32 word = p_render->m_words[i];

		p_edge->m_values[word] += p_edge->m_steps[word];
	}
}

// --- Spans ---

// The edges at a span's ends, ordered by x: the assembly swaps them unless the right edge's x is
// the greater.
typedef struct RenderSpan {
	const RenderEdge* m_low;
	const RenderEdge* m_high;
	MechS32 m_left;   // the integer x of the ends
	MechU32 m_length; // the pixels after the first
} RenderSpan;

static void StartSpan(const Render* p_render, RenderSpan* p_span)
{
	if (PortableS32(p_render->m_right.m_values[c_vertexX]) > PortableS32(p_render->m_left.m_values[c_vertexX])) {
		p_span->m_low = &p_render->m_left;
		p_span->m_high = &p_render->m_right;
	}
	else {
		p_span->m_low = &p_render->m_right;
		p_span->m_high = &p_render->m_left;
	}

	p_span->m_left = Integer(p_span->m_low->m_values[c_vertexX]);
	p_span->m_length = (MechU32) Integer(p_span->m_high->m_values[c_vertexX]) - (MechU32) p_span->m_left;
}

static MechU8* SpanPixels(const Render* p_render, const RenderSpan* p_span)
{
	return p_render->m_buffer + PortableS32(p_render->m_row + (MechU32) p_span->m_left);
}

// A span's Gouraud shades: the low end's color plus one dither level on its even pixels and plus
// the other on its odd ones, each stepping every other pixel by twice the slope (the color over
// half the span's length, at least 1). The assembly writes them in pairs, starting with a single
// pixel at an odd address for some passes, which puts the same shades on the same pixels.
typedef struct Shades {
	MechU32 m_even;
	MechU32 m_odd;
	MechU32 m_step;
} Shades;

static void StartShades(const Render* p_render, const RenderSpan* p_span, Shades* p_shades)
{
	MechU32 color = p_span->m_low->m_values[c_vertexColor];

	p_shades->m_step = 0;
	if (p_span->m_length) {
		MechU32 halves = p_span->m_length >> 1;

		p_shades->m_step = Slope(p_span->m_high->m_values[c_vertexColor] - color, halves ? halves : 1);
	}

	p_shades->m_even = color + p_render->m_dither[0];
	p_shades->m_odd = color + p_render->m_dither[1];
}

static MechU8 Shade(const Shades* p_shades, MechU32 p_pixel)
{
	MechU32 shade = (p_pixel & 1 ? p_shades->m_odd : p_shades->m_even) + (p_pixel >> 1) * p_shades->m_step;

	return (MechU8) (shade >> 16);
}

static void SwapDither(Render* p_render)
{
	MechU32 level = p_render->m_dither[0];

	p_render->m_dither[0] = p_render->m_dither[1];
	p_render->m_dither[1] = level;
}

// The Gouraud passes: a fill of the shades (R_GOURAUD_SOLID), of the translucency table's entries
// for them (I_GOURAUD_SOLID), of p_cueing's for the polygon's color (M_GOURAUD_SOLID), or the
// pixels there translated through p_cueing's row for each shade (GOURAUD_CUE).
static void DrawShades(Render* p_render, MechS32 p_pass)
{
	RenderSpan span;
	Shades shades;
	MechU8* pixels;
	MechU32 i;

	StartSpan(p_render, &span);
	StartShades(p_render, &span, &shades);
	pixels = SpanPixels(p_render, &span);
	if (p_pass == c_passCueFill) {
		PORTABLE_ASSERT(!(p_render->m_color >> 16));
	}

	for (i = 0; i <= span.m_length; i++) {
		MechU8 shade = Shade(&shades, i);

		switch (p_pass) {
		case c_passRangeFill:
			pixels[i] = shade;
			break;
		case c_passIndirectFill:
			pixels[i] = p_render->m_translucency[shade];
			break;
		case c_passCueFill:
			pixels[i] = p_render->m_cueing[(p_render->m_color & ~(MechU32) 0xff00) | (MechU32) shade << 8];
			break;
		default:
			pixels[i] = p_render->m_cueing[(MechU32) shade << 8 | pixels[i]];
			break;
		}
	}

	SwapDither(p_render);
}

// FLAT_SOLID: the color's integer part.
static void DrawSolid(const Render* p_render)
{
	RenderSpan span;

	StartSpan(p_render, &span);
	memset(SpanPixels(p_render, &span), (MechU8) (p_render->m_color >> 16), span.m_length + 1);
}

// FLAT_CUE: the pixels there through p_cueing.
static void DrawFlatCue(const Render* p_render)
{
	RenderSpan span;
	MechU8* pixels;
	MechU32 i;

	StartSpan(p_render, &span);
	pixels = SpanPixels(p_render, &span);
	for (i = 0; i <= span.m_length; i++) {
		pixels[i] = p_render->m_cueing[pixels[i]];
	}
}

// --- Texture mapping ---

// A run of pixels' walk through the texture: u and v in 32.32, the texel in the high word.
typedef struct TextureWalk {
	MechU64 m_u;
	MechU64 m_v;
	MechU64 m_uStep;
	MechU64 m_vStep;
} TextureWalk;

// A walk from 16.16 coordinates and steps.
static void StartWalk(TextureWalk* p_walk, MechU32 p_u, MechU32 p_uStep, MechU32 p_v, MechU32 p_vStep)
{
	p_walk->m_u = (MechU64) p_u << 16;
	p_walk->m_v = (MechU64) p_v << 16;
	p_walk->m_uStep = (MechU64) ((MechS64) PortableS32(p_uStep) * 0x10000);
	p_walk->m_vStep = (MechU64) ((MechS64) PortableS32(p_vStep) * 0x10000);
}

// TEXTURE_INDEX and TEXTURE_XP: the texel at the walk's position, through the flat lookaside
// table if the operation shades. Returns 0 for a transparent one.
static MechS32 FetchTexel(const Render* p_render, const TextureWalk* p_walk, MechU8* p_texel)
{
	const VFX_TEXTURE* texture = p_render->m_texture;
	MechU32 u = (MechU32) (p_walk->m_u >> 32);
	MechU32 v = (MechU32) (p_walk->m_v >> 32);
	MechU8 texel;

	switch (p_render->m_operation & c_tileMask) {
	case c_tileLog:
		texel = texture->m_vAddrs[v & ((MechU32) texture->m_height - 1)][u & ((MechU32) texture->m_width - 1)];
		break;
	case c_tileLinear:
		PORTABLE_ASSERT(texture->m_width && texture->m_height);
		texel = texture->m_vAddrs[v % (MechU32) texture->m_height][u % (MechU32) texture->m_width];
		break;
	default:
		texel = texture->m_vAddrs[PortableS32(v)][PortableS32(u)];
		break;
	}

	if ((p_render->m_operation & c_shadeMask) == c_shadeFlat) {
		texel = p_render->m_cueing[texel];
	}

	*p_texel = texel;
	switch (p_render->m_operation & c_transparentMask) {
	case c_transparentSkip:
	case c_transparentRange:
	case c_transparentIndirect:
		return texel != TRANSPARENT_TEXEL;
	default:
		return 1;
	}
}

// The texels of p_count pixels. Subsampled, each texel takes two pixels, with steps twice as long,
// while 8 pixels or more are left; the rest take one each.
static void DrawTexels(const Render* p_render, MechU8* p_pixels, MechU32 p_count, TextureWalk* p_walk)
{
	MechU8 texel;

	if (p_render->m_operation & c_sampleCoarse) {
		MechU64 uStep = p_walk->m_uStep * 2;
		MechU64 vStep = p_walk->m_vStep * 2;

		while (p_count >= 8) {
			MechS32 i;

			for (i = 0; i < 4; i++) {
				if (FetchTexel(p_render, p_walk, &texel)) {
					p_pixels[0] = texel;
					p_pixels[1] = texel;
				}

				p_pixels += 2;
				p_walk->m_u += uStep;
				p_walk->m_v += vStep;
			}

			p_count -= 8;
		}
	}

	while (p_count) {
		if (FetchTexel(p_render, p_walk, &texel)) {
			*p_pixels = texel;
		}

		p_pixels++;
		p_walk->m_u += p_walk->m_uStep;
		p_walk->m_v += p_walk->m_vStep;
		p_count--;
	}
}

// AFFINE_MAP: u and v linear across the span, from its low end rounded to the texel's centre.
static void DrawAffine(const Render* p_render)
{
	RenderSpan span;
	TextureWalk walk;
	MechU32 count;
	MechU32 u;
	MechU32 v;

	StartSpan(p_render, &span);
	count = span.m_length + 1;
	u = span.m_low->m_values[c_vertexU];
	v = span.m_low->m_values[c_vertexV];
	StartWalk(
		&walk,
		u + 0x8000,
		Slope(span.m_high->m_values[c_vertexU] - u, count),
		v + 0x8000,
		Slope(span.m_high->m_values[c_vertexV] - v, count)
	);
	DrawTexels(p_render, SpanPixels(p_render, &span), count, &walk);
}

// A perspective segment: p_count pixels from (p_u, p_v), with steps of p_uStep and p_vStep. Moves
// the coordinates to the segment's end.
static void DrawSegment(
	const Render* p_render,
	MechU8* p_pixels,
	MechU32 p_count,
	MechU32* p_u,
	MechU32 p_uStep,
	MechU32* p_v,
	MechU32 p_vStep
)
{
	TextureWalk walk;

	StartWalk(&walk, *p_u, p_uStep, *p_v, p_vStep);
	DrawTexels(p_render, p_pixels, p_count, &walk);
	*p_u = (MechU32) (walk.m_u >> 16);
	*p_v = (MechU32) (walk.m_v >> 16);
}

// PERSPECTIVE_MAP: u, v and w linear across the span, and the texture coordinates u / w and
// v / w, rounded to the texel's centre, at each 16 pixels' end, linear in between. A segment's
// start is where the previous one's steps took it; the last segment, of the pixels left, ends
// at the span's high end.
static void DrawPerspective(const Render* p_render)
{
	RenderSpan span;
	MechU32 u;
	MechU32 v;
	MechU32 w;
	MechU32 uStep;
	MechU32 vStep;
	MechU32 wStep;
	MechU32 uTexel;
	MechU32 vTexel;
	MechU32 count;
	MechU32 rest;
	MechU32 segments;
	MechU8* pixels;

	StartSpan(p_render, &span);
	count = span.m_length + 1;
	u = span.m_low->m_values[c_vertexU];
	v = span.m_low->m_values[c_vertexV];
	w = span.m_low->m_values[c_vertexW];
	uStep = Slope(span.m_high->m_values[c_vertexU] - u, count) << SEGMENT_SHIFT;
	vStep = Slope(span.m_high->m_values[c_vertexV] - v, count) << SEGMENT_SHIFT;
	wStep = Slope(span.m_high->m_values[c_vertexW] - w, count) << SEGMENT_SHIFT;
	uTexel = DivideOne(u, w) + 0x8000;
	vTexel = DivideOne(v, w) + 0x8000;
	pixels = SpanPixels(p_render, &span);

	for (segments = count >> SEGMENT_SHIFT; segments; segments--) {
		MechU32 vEnd;
		MechU32 uEnd;

		u += uStep;
		v += vStep;
		w += wStep;
		vEnd = DivideOne(v, w) + 0x8000;
		uEnd = DivideOne(u, w) + 0x8000;
		DrawSegment(
			p_render,
			pixels,
			SEGMENT_PIXELS,
			&uTexel,
			(MechU32) PortableSar32(PortableS32(uEnd - uTexel), SEGMENT_SHIFT),
			&vTexel,
			(MechU32) PortableSar32(PortableS32(vEnd - vTexel), SEGMENT_SHIFT)
		);
		pixels += SEGMENT_PIXELS;
	}

	rest = count & (SEGMENT_PIXELS - 1);
	if (rest) {
		const RenderEdge* high = span.m_high;
		MechU32 uEnd = DivideOne(high->m_values[c_vertexU], high->m_values[c_vertexW]) + 0x8000;
		MechU32 vEnd = DivideOne(high->m_values[c_vertexV], high->m_values[c_vertexW]) + 0x8000;

		DrawSegment(p_render, pixels, rest, &uTexel, Slope(uEnd - uTexel, rest), &vTexel, Slope(vEnd - vTexel, rest));
	}
}

// --- Polygons ---

static void DrawLine(Render* p_render)
{
	MechS32 i;

	for (i = 0; i < 3; i++) {
		switch (p_render->m_passes[i]) {
		case c_passRangeFill:
		case c_passIndirectFill:
		case c_passCueFill:
		case c_passGouraudCue:
			DrawShades(p_render, p_render->m_passes[i]);
			break;
		case c_passSolidFill:
			DrawSolid(p_render);
			break;
		case c_passTexture:
			if ((p_render->m_operation & c_mapMask) == c_mapPerspective) {
				DrawPerspective(p_render);
			}
			else {
				DrawAffine(p_render);
			}
			break;
		case c_passFlatCue:
			DrawFlatCue(p_render);
			break;
		default:
			break;
		}
	}
}

// SCAN_CONVERT: the polygon from its top vertex (the last of the least y) down to its greatest y.
static void ScanConvert(Render* p_render)
{
	const MechU32* vertex;
	const MechU32* top = NULL;
	MechS32 topY = 0x7fffffff;
	MechS32 bottomY = -0x7fffffff - 1;
	MechU32 lines;

	for (vertex = p_render->m_vertices; vertex != p_render->m_verticesEnd; vertex += VERTEX_WORDS) {
		MechS32 y = PortableS32(vertex[c_vertexY]);

		if (y <= topY) {
			topY = y;
			top = vertex;
		}

		if (y >= bottomY) {
			bottomY = y;
		}
	}

	lines = (MechU32) bottomY - (MechU32) topY;
	if (!lines) {
		return;
	}

	p_render->m_left.m_next = top;
	p_render->m_right.m_next = top;
	StartFirstEdge(p_render, &p_render->m_left);
	StartFirstEdge(p_render, &p_render->m_right);
	p_render->m_row = (MechU32) topY * p_render->m_pitch;

	for (;;) {
		DrawLine(p_render);
		p_render->m_row += p_render->m_pitch;
		lines--;
		if (PortableS32(lines) < 0) {
			break;
		}

		// The last line steps both edges once more, whatever their counts
		if (!lines) {
			StepEdge(p_render, &p_render->m_left);
			StepEdge(p_render, &p_render->m_right);
			continue;
		}

		if (--p_render->m_left.m_count) {
			StepEdge(p_render, &p_render->m_left);
		}
		else {
			StartNextEdge(p_render, &p_render->m_left);
		}

		if (--p_render->m_right.m_count) {
			StepEdge(p_render, &p_render->m_right);
		}
		else {
			StartNextEdge(p_render, &p_render->m_right);
		}
	}
}

static void RenderPolygon(
	const PANE* p_pane,
	const MechU32* p_vlist,
	MechS32 p_nvertices,
	MechS32 p_operation,
	undefined4 p_color,
	VFX_TEXTURE* p_texture,
	void* p_cueing,
	void* p_translucency
)
{
	Render render;

	PORTABLE_ASSERT(p_operation >= 0 && p_operation < OPERATION_COUNT);
	if (!IsBuilt(p_operation)) {
		return;
	}

	PORTABLE_ASSERT(p_nvertices > 0);
	render.m_buffer = p_pane->m_window->m_buffer;
	render.m_pitch = (MechU32) p_pane->m_window->m_xMax + 1;
	render.m_vertices = p_vlist;
	render.m_verticesEnd = p_vlist + p_nvertices * VERTEX_WORDS;
	render.m_operation = p_operation;
	render.m_color = p_color;
	render.m_texture = p_texture;
	render.m_cueing = (const MechU8*) p_cueing;
	render.m_translucency = (const MechU8*) p_translucency;
	render.m_dither[0] = g_ditherLevels[0];
	render.m_dither[1] = g_ditherLevels[1];
	SelectPasses(&render);
	ScanConvert(&render);
}

void VFX_polygon_render(
	PANE* p_pane,
	MechU32* p_vlist,
	MechS32 p_nvertices,
	MechS32 p_operation,
	undefined4 p_color,
	VFX_TEXTURE* p_texture,
	void* p_cueing,
	void* p_translucency
)
{
	RenderPolygon(p_pane, p_vlist, p_nvertices, p_operation, p_color, p_texture, p_cueing, p_translucency);
}

// --- Clipping ---

// The pane's sides, in the order the clipper takes them, and the outcode bit of each.
enum ClipSide {
	c_clipRight,
	c_clipBottom, // the least y (VFXREND calls y0 the bottom)
	c_clipLeft,
	c_clipTop
};

static const MechU32 g_clipBits[] = {8, 1, 4, 2};

// The words a clipped vertex interpolates besides x and y, by operation (UV_clip, W_clip,
// C_clip).
typedef struct ClipWords {
	MechS32 m_words[4];
	MechS32 m_count;
} ClipWords;

static void SelectClipWords(ClipWords* p_words, MechS32 p_operation)
{
	MechS32 shade = p_operation & c_shadeMask;
	MechS32 transparency = p_operation & c_transparentMask;

	p_words->m_count = 0;
	if (p_operation & c_mapAffine) {
		p_words->m_words[p_words->m_count++] = c_vertexU;
		p_words->m_words[p_words->m_count++] = c_vertexV;
	}

	if ((p_operation & c_mapMask) == c_mapPerspective) {
		p_words->m_words[p_words->m_count++] = c_vertexW;
	}

	if (shade == c_shadeGouraud || shade == c_shadeIndirect || shade == c_shadeRange ||
		transparency == c_transparentIndirect || transparency == c_transparentRange) {
		p_words->m_words[p_words->m_count++] = c_vertexColor;
	}
}

static MechS32 IsInside(MechS32 p_side, MechS32 p_value, MechS32 p_clip)
{
	return p_side == c_clipRight || p_side == c_clipTop ? p_value <= p_clip : p_value >= p_clip;
}

// CLIP_X_LINE and CLIP_Y_LINE: the point of the edge from p_second to p_first on the side, as
// p_second plus the edge times the fraction of it inside. Writes x, y and p_words only.
static void ClipEdge(
	const MechU32* p_first,
	const MechU32* p_second,
	MechU32* p_out,
	MechS32 p_side,
	MechS32 p_clip,
	const ClipWords* p_words
)
{
	MechS32 across = p_side == c_clipRight || p_side == c_clipLeft ? c_vertexX : c_vertexY;
	MechS32 along = across == c_vertexX ? c_vertexY : c_vertexX;
	MechU32 fraction = DivideToF30((MechU32) p_clip - p_second[across], p_first[across] - p_second[across]);
	MechS32 i;

	p_out[across] = (MechU32) p_clip;
	p_out[along] = p_second[along] + MultiplyF30(p_first[along] - p_second[along], fraction);
	for (i = 0; i < p_words->m_count; i++) {
		MechS32 word = p_words->m_words[i];

		p_out[word] = p_second[word] + MultiplyF30(p_first[word] - p_second[word], fraction);
	}
}

// One side: each edge from p_first to p_second copies p_first if it's inside, and adds the edge's
// crossing if one end is outside. Returns the vertices written.
static MechU32 ClipSide(
	const MechU32* p_source,
	MechU32 p_count,
	MechU32* p_destination,
	MechS32 p_side,
	MechS32 p_clip,
	const ClipWords* p_words
)
{
	MechS32 across = p_side == c_clipRight || p_side == c_clipLeft ? c_vertexX : c_vertexY;
	const MechU32* first = p_source + (p_count - 1) * VERTEX_WORDS;
	const MechU32* second = p_source;
	MechU32 out = 0;
	MechU32 i;

	for (i = 0; i < p_count; i++) {
		MechS32 firstInside = IsInside(p_side, PortableS32(first[across]), p_clip);
		MechS32 secondInside = IsInside(p_side, PortableS32(second[across]), p_clip);

		if (firstInside) {
			PORTABLE_ASSERT(p_destination != g_workPoints || out < MAX_WORK_VERTICES);
			memcpy(p_destination + out * VERTEX_WORDS, first, VERTEX_WORDS * sizeof(MechU32));
			out++;
		}

		if (firstInside != secondInside) {
			PORTABLE_ASSERT(p_destination != g_workPoints || out < MAX_WORK_VERTICES);
			ClipEdge(first, second, p_destination + out * VERTEX_WORDS, p_side, p_clip, p_words);
			out++;
		}

		first = second;
		second += VERTEX_WORDS;
	}

	return out;
}

void VFX_polygon_clip_XY_and_render(
	PANE* p_pane,
	MechU32* p_vlist,
	MechS32 p_nvertices,
	MechS32 p_operation,
	undefined4 p_color,
	VFX_TEXTURE* p_texture,
	void* p_cueing,
	void* p_translucency
)
{
	MechS32 clips[4];
	MechU32 needed = 0;
	MechU32 count = (MechU32) p_nvertices;
	MechU32* source = p_vlist;
	MechU32* destination = g_workPoints;
	MechU32 i;

	clips[c_clipRight] = p_pane->m_x1;
	clips[c_clipBottom] = p_pane->m_y0;
	clips[c_clipLeft] = p_pane->m_x0;
	clips[c_clipTop] = p_pane->m_y1;

	// The outcodes: the signs of each vertex's distances inside the sides
	PORTABLE_ASSERT(p_nvertices > 0);
	for (i = 0; i < count; i++) {
		const MechU32* vertex = p_vlist + i * VERTEX_WORDS;
		MechU32 x = vertex[c_vertexX];
		MechU32 y = vertex[c_vertexY];

		needed |= ((MechU32) clips[c_clipRight] - x) >> 31 << 3;
		needed |= (x - (MechU32) clips[c_clipLeft]) >> 31 << 2;
		needed |= ((MechU32) clips[c_clipTop] - y) >> 31 << 1;
		needed |= (y - (MechU32) clips[c_clipBottom]) >> 31;
	}

	if (needed) {
		ClipWords words;
		MechS32 side;

		SelectClipWords(&words, p_operation);
		for (side = c_clipRight; side <= c_clipTop; side++) {
			MechU32* swap;

			if (!(needed & g_clipBits[side])) {
				continue;
			}

			count = ClipSide(source, count, destination, side, clips[side], &words);
			if (!count) {
				return;
			}

			swap = source;
			source = destination;
			destination = swap;
		}
	}

	if (PortableS32(count) < 3) {
		return;
	}

	RenderPolygon(p_pane, source, (MechS32) count, p_operation, p_color, p_texture, p_cueing, p_translucency);
}
