/* VFXREND, Miles Design VFX's polygon renderer, reimplemented in C, which both DLLs compile.

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
static const uint16_t g_primitives[] = {
	0x691, 0x611, 0x491, 0x411, 0x6a0, 0x680, 0x697, 0x689, 0x6b1, 0x690, 0x600, 0x610, 0x650,
	0x497, 0x490, 0x400, 0x410, 0x408, 0x450, 0x080, 0x280, 0x040, 0x300, 0x2c0, 0x240,
};

// VFX_set_Gouraud_dither_level's levels (dfactor_1, dfactor_2), which each Gouraud pass adds to
// the shades of a scan line's even and odd pixels, swapping them every line.
static uint32_t g_ditherLevels[2] = {0x8000, 0};

// The clipper's working vertices (work_pts).
static uint32_t g_workPoints[MAX_WORK_VERTICES * VERTEX_WORDS];

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
	const uint32_t* m_next;          // the vertex at the edge's end
	uint32_t m_count;                // the scan lines left on the edge
	uint32_t m_values[VERTEX_WORDS]; // by vertex word (x in 16.16, with a half)
	uint32_t m_steps[VERTEX_WORDS];  // per scan line
} RenderEdge;

typedef struct Render {
	uint8_t* m_buffer;
	uint32_t m_pitch;
	uint32_t m_row; // the scan line's offset in m_buffer
	const uint32_t* m_vertices;
	const uint32_t* m_verticesEnd;
	int32_t m_operation;
	uint32_t m_color;
	const VFX_TEXTURE* m_texture;
	const uint8_t* m_cueing;
	const uint8_t* m_translucency;
	uint32_t m_dither[2];
	int32_t m_passes[3];
	int32_t m_words[4]; // the vertex words the edges interpolate besides x
	int32_t m_wordCount;
	RenderEdge m_left;  // walks the vertices backwards
	RenderEdge m_right; // forwards
} Render;

// `sar 16`: a 16.16 value's integer part.
static int32_t Integer(uint32_t p_value)
{
	return PortableSar32(PortableS32(p_value), 16);
}

// FIXDIV and FPDIV: a 16.16 value over a count, as p_delta shifted into a 64-bit dividend over
// p_count shifted into a 32-bit divisor (which a count of 0x10000 makes 0).
static uint32_t Slope(uint32_t p_delta, uint32_t p_count)
{
	return (uint32_t) PortableIdiv((int64_t) PortableS32(p_delta) * 0x10000, PortableS32(p_count << 16));
}

// FPDIVONE: a perspective coordinate (16.16, times w) over w (2.30).
static uint32_t DivideOne(uint32_t p_value, uint32_t p_w)
{
	return (uint32_t) PortableIdiv((int64_t) PortableS32(p_value) * 0x40000000, PortableS32(p_w));
}

// The quotient of two magnitudes, in 32 bits, rounded half up: the remainder carries when it is
// at least half the divisor (rounded up).
static uint32_t DivideRounded(uint64_t p_dividend, uint32_t p_divisor)
{
	uint32_t quotient = PortableDiv(p_dividend, p_divisor);
	uint32_t remainder = (uint32_t) (p_dividend - (uint64_t) quotient * p_divisor);

	if ((p_divisor >> 1) + (p_divisor & 1) - 1 < remainder) {
		quotient++;
	}

	return quotient;
}

static uint32_t Magnitude(uint32_t p_value, int32_t* p_negative, int32_t p_sign)
{
	if (PortableS32(p_value) < 0) {
		*p_negative += p_sign;
		return 0 - p_value;
	}

	return p_value;
}

// F16_div_to_F30: the magnitudes' quotient with the dividend shifted up by 30 (`sar 2` into the
// high word, so a dividend of 0x80000000 faults), negated if the signs differ.
static uint32_t DivideToF30(uint32_t p_dividend, uint32_t p_divisor)
{
	int32_t negative = 0;
	uint32_t dividend = Magnitude(p_dividend, &negative, 1);
	uint32_t divisor = Magnitude(p_divisor, &negative, -1);
	uint64_t wide = (uint64_t) (uint32_t) PortableSar32(PortableS32(dividend), 2) << 32 | (uint32_t) (dividend << 30);
	uint32_t quotient = DivideRounded(wide, divisor);

	return negative ? 0 - quotient : quotient;
}

// MMUL_F30: the product of p_value and the 2.30 p_factor, rounded, shifted down by 30.
static uint32_t MultiplyF30(uint32_t p_value, uint32_t p_factor)
{
	uint64_t product = (uint64_t) ((int64_t) PortableS32(p_value) * PortableS32(p_factor));

	return (uint32_t) ((product + 0x20000000) >> 30);
}

// --- The entry points ---

void VFX_set_Gouraud_dither_level(int32_t p_dither1, int32_t p_dither2)
{
	g_ditherLevels[0] = (uint32_t) p_dither1;
	g_ditherLevels[1] = (uint32_t) p_dither2;
}

// The assembly returns its code segment's selector, and the range of its primitives and its size;
// the C patches no code, and returns its working vertices instead, which the caller can make
// writable all the same. The start is a 32-bit address, as the game's caller takes it.
int32_t GetCodeBlock(uint32_t* p_start, uint32_t* p_selector)
{
	*p_selector = 0;
	*p_start = (uint32_t) (size_t) g_workPoints;
	return (int32_t) sizeof(g_workPoints);
}

int32_t F16_div_to_F30(int32_t p_dividend, int32_t p_divisor)
{
	return PortableS32(DivideToF30((uint32_t) p_dividend, (uint32_t) p_divisor));
}

// 1.0 (16.16) over p_value: 0x4000 in the dividend's high word.
int32_t F30_reciprocal(int32_t p_value)
{
	int32_t negative = 0;
	uint32_t divisor = Magnitude((uint32_t) p_value, &negative, -1);
	uint32_t quotient = DivideRounded((uint64_t) 0x4000 << 32, divisor);

	return PortableS32(negative ? 0 - quotient : quotient);
}

int32_t mul_F30(int32_t p_m1, int32_t p_m2)
{
	return PortableS32(MultiplyF30((uint32_t) p_m1, (uint32_t) p_m2));
}

// --- Scan conversion ---

static int32_t IsBuilt(int32_t p_operation)
{
	int32_t i;

	for (i = 0; i < (int32_t) (sizeof(g_primitives) / sizeof(g_primitives[0])); i++) {
		if (g_primitives[i] == p_operation) {
			return 1;
		}
	}

	return 0;
}

static void AddWord(Render* p_render, int32_t p_word)
{
	p_render->m_words[p_render->m_wordCount++] = p_word;
}

// The passes and the edges' words of an operation, as MAKE_POLY selects them.
static void SelectPasses(Render* p_render)
{
	int32_t map = p_render->m_operation & c_mapMask;
	int32_t shade = p_render->m_operation & c_shadeMask;
	int32_t transparency = p_render->m_operation & c_transparentMask;
	int32_t useColor = 0;

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

static const uint32_t* PreviousVertex(const Render* p_render, const uint32_t* p_vertex)
{
	if (p_vertex == p_render->m_vertices) {
		p_vertex = p_render->m_verticesEnd;
	}

	return p_vertex - VERTEX_WORDS;
}

static const uint32_t* NextVertex(const Render* p_render, const uint32_t* p_vertex)
{
	p_vertex += VERTEX_WORDS;
	if (p_vertex == p_render->m_verticesEnd) {
		p_vertex = p_render->m_vertices;
	}

	return p_vertex;
}

// LEFT_DELTAS and RIGHT_DELTAS: starts an edge at its top vertex, towards m_next.
static void StartEdge(const Render* p_render, RenderEdge* p_edge, const uint32_t* p_start, uint32_t p_count)
{
	const uint32_t* end = p_edge->m_next;
	int32_t i;

	p_edge->m_count = p_count;
	p_edge->m_steps[c_vertexX] = Slope((end[c_vertexX] - p_start[c_vertexX]) << 16, p_count);
	p_edge->m_values[c_vertexX] = (p_start[c_vertexX] << 16) + 0x8000;
	for (i = 0; i < p_render->m_wordCount; i++) {
		int32_t word = p_render->m_words[i];

		p_edge->m_steps[word] = Slope(end[word] - p_start[word], p_count);
		p_edge->m_values[word] = p_start[word];
	}
}

// Moves an edge to the one after it along its walk; returns its top vertex.
static const uint32_t* AdvanceEdge(const Render* p_render, RenderEdge* p_edge)
{
	const uint32_t* start = p_edge->m_next;

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
	const uint32_t* start;
	uint32_t count;

	do {
		start = AdvanceEdge(p_render, p_edge);
		count = p_edge->m_next[c_vertexY] - start[c_vertexY];
	} while (!count);

	StartEdge(p_render, p_edge, start, count);
}

// The next edge, when the scan lines reach the end of one: a horizontal one for a line.
static void StartNextEdge(const Render* p_render, RenderEdge* p_edge)
{
	const uint32_t* start = AdvanceEdge(p_render, p_edge);
	uint32_t count = p_edge->m_next[c_vertexY] - start[c_vertexY];

	StartEdge(p_render, p_edge, start, count ? count : 1);
}

// LEFT_TRACE and RIGHT_TRACE.
static void StepEdge(const Render* p_render, RenderEdge* p_edge)
{
	int32_t i;

	p_edge->m_values[c_vertexX] += p_edge->m_steps[c_vertexX];
	for (i = 0; i < p_render->m_wordCount; i++) {
		int32_t word = p_render->m_words[i];

		p_edge->m_values[word] += p_edge->m_steps[word];
	}
}

// --- Spans ---

// The edges at a span's ends, ordered by x: the assembly swaps them unless the right edge's x is
// the greater.
typedef struct RenderSpan {
	const RenderEdge* m_low;
	const RenderEdge* m_high;
	int32_t m_left;    // the integer x of the ends
	uint32_t m_length; // the pixels after the first
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
	p_span->m_length = (uint32_t) Integer(p_span->m_high->m_values[c_vertexX]) - (uint32_t) p_span->m_left;
}

static uint8_t* SpanPixels(const Render* p_render, const RenderSpan* p_span)
{
	return p_render->m_buffer + PortableS32(p_render->m_row + (uint32_t) p_span->m_left);
}

// A span's Gouraud shades: the low end's color plus one dither level on its even pixels and plus
// the other on its odd ones, each stepping every other pixel by twice the slope (the color over
// half the span's length, at least 1). The assembly writes them in pairs, starting with a single
// pixel at an odd address for some passes, which puts the same shades on the same pixels.
typedef struct Shades {
	uint32_t m_even;
	uint32_t m_odd;
	uint32_t m_step;
} Shades;

static void StartShades(const Render* p_render, const RenderSpan* p_span, Shades* p_shades)
{
	uint32_t color = p_span->m_low->m_values[c_vertexColor];

	p_shades->m_step = 0;
	if (p_span->m_length) {
		uint32_t halves = p_span->m_length >> 1;

		p_shades->m_step = Slope(p_span->m_high->m_values[c_vertexColor] - color, halves ? halves : 1);
	}

	p_shades->m_even = color + p_render->m_dither[0];
	p_shades->m_odd = color + p_render->m_dither[1];
}

static uint8_t Shade(const Shades* p_shades, uint32_t p_pixel)
{
	uint32_t shade = (p_pixel & 1 ? p_shades->m_odd : p_shades->m_even) + (p_pixel >> 1) * p_shades->m_step;

	return (uint8_t) (shade >> 16);
}

static void SwapDither(Render* p_render)
{
	uint32_t level = p_render->m_dither[0];

	p_render->m_dither[0] = p_render->m_dither[1];
	p_render->m_dither[1] = level;
}

// The Gouraud passes: a fill of the shades (R_GOURAUD_SOLID), of the translucency table's entries
// for them (I_GOURAUD_SOLID), of p_cueing's for the polygon's color (M_GOURAUD_SOLID), or the
// pixels there translated through p_cueing's row for each shade (GOURAUD_CUE).
static void DrawShades(Render* p_render, int32_t p_pass)
{
	RenderSpan span;
	Shades shades;
	uint8_t* pixels;
	uint32_t i;

	StartSpan(p_render, &span);
	StartShades(p_render, &span, &shades);
	pixels = SpanPixels(p_render, &span);
	if (p_pass == c_passCueFill) {
		PORTABLE_ASSERT(!(p_render->m_color >> 16));
	}

	for (i = 0; i <= span.m_length; i++) {
		uint8_t shade = Shade(&shades, i);

		switch (p_pass) {
		case c_passRangeFill:
			pixels[i] = shade;
			break;
		case c_passIndirectFill:
			pixels[i] = p_render->m_translucency[shade];
			break;
		case c_passCueFill:
			pixels[i] = p_render->m_cueing[(p_render->m_color & ~(uint32_t) 0xff00) | (uint32_t) shade << 8];
			break;
		default:
			pixels[i] = p_render->m_cueing[(uint32_t) shade << 8 | pixels[i]];
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
	memset(SpanPixels(p_render, &span), (uint8_t) (p_render->m_color >> 16), span.m_length + 1);
}

// FLAT_CUE: the pixels there through p_cueing.
static void DrawFlatCue(const Render* p_render)
{
	RenderSpan span;
	uint8_t* pixels;
	uint32_t i;

	StartSpan(p_render, &span);
	pixels = SpanPixels(p_render, &span);
	for (i = 0; i <= span.m_length; i++) {
		pixels[i] = p_render->m_cueing[pixels[i]];
	}
}

// --- Texture mapping ---

// A run of pixels' walk through the texture: u and v in 32.32, the texel in the high word.
typedef struct TextureWalk {
	uint64_t m_u;
	uint64_t m_v;
	uint64_t m_uStep;
	uint64_t m_vStep;
} TextureWalk;

// A walk from 16.16 coordinates and steps.
static void StartWalk(TextureWalk* p_walk, uint32_t p_u, uint32_t p_uStep, uint32_t p_v, uint32_t p_vStep)
{
	p_walk->m_u = (uint64_t) p_u << 16;
	p_walk->m_v = (uint64_t) p_v << 16;
	p_walk->m_uStep = (uint64_t) ((int64_t) PortableS32(p_uStep) * 0x10000);
	p_walk->m_vStep = (uint64_t) ((int64_t) PortableS32(p_vStep) * 0x10000);
}

// TEXTURE_INDEX and TEXTURE_XP: the texel at the walk's position, through the flat lookaside
// table if the operation shades. Returns 0 for a transparent one.
static int32_t FetchTexel(const Render* p_render, const TextureWalk* p_walk, uint8_t* p_texel)
{
	const VFX_TEXTURE* texture = p_render->m_texture;
	uint32_t u = (uint32_t) (p_walk->m_u >> 32);
	uint32_t v = (uint32_t) (p_walk->m_v >> 32);
	uint8_t texel;

	switch (p_render->m_operation & c_tileMask) {
	case c_tileLog:
		texel = texture->m_vAddrs[v & ((uint32_t) texture->m_height - 1)][u & ((uint32_t) texture->m_width - 1)];
		break;
	case c_tileLinear:
		PORTABLE_ASSERT(texture->m_width && texture->m_height);
		texel = texture->m_vAddrs[v % (uint32_t) texture->m_height][u % (uint32_t) texture->m_width];
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
static void DrawTexels(const Render* p_render, uint8_t* p_pixels, uint32_t p_count, TextureWalk* p_walk)
{
	uint8_t texel;

	if (p_render->m_operation & c_sampleCoarse) {
		uint64_t uStep = p_walk->m_uStep * 2;
		uint64_t vStep = p_walk->m_vStep * 2;

		while (p_count >= 8) {
			int32_t i;

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
	uint32_t count;
	uint32_t u;
	uint32_t v;

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
	uint8_t* p_pixels,
	uint32_t p_count,
	uint32_t* p_u,
	uint32_t p_uStep,
	uint32_t* p_v,
	uint32_t p_vStep
)
{
	TextureWalk walk;

	StartWalk(&walk, *p_u, p_uStep, *p_v, p_vStep);
	DrawTexels(p_render, p_pixels, p_count, &walk);
	*p_u = (uint32_t) (walk.m_u >> 16);
	*p_v = (uint32_t) (walk.m_v >> 16);
}

// PERSPECTIVE_MAP: u, v and w linear across the span, and the texture coordinates u / w and
// v / w, rounded to the texel's centre, at each 16 pixels' end, linear in between. A segment's
// start is where the previous one's steps took it; the last segment, of the pixels left, ends
// at the span's high end.
static void DrawPerspective(const Render* p_render)
{
	RenderSpan span;
	uint32_t u;
	uint32_t v;
	uint32_t w;
	uint32_t uStep;
	uint32_t vStep;
	uint32_t wStep;
	uint32_t uTexel;
	uint32_t vTexel;
	uint32_t count;
	uint32_t rest;
	uint32_t segments;
	uint8_t* pixels;

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
		uint32_t vEnd;
		uint32_t uEnd;

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
			(uint32_t) PortableSar32(PortableS32(uEnd - uTexel), SEGMENT_SHIFT),
			&vTexel,
			(uint32_t) PortableSar32(PortableS32(vEnd - vTexel), SEGMENT_SHIFT)
		);
		pixels += SEGMENT_PIXELS;
	}

	rest = count & (SEGMENT_PIXELS - 1);
	if (rest) {
		const RenderEdge* high = span.m_high;
		uint32_t uEnd = DivideOne(high->m_values[c_vertexU], high->m_values[c_vertexW]) + 0x8000;
		uint32_t vEnd = DivideOne(high->m_values[c_vertexV], high->m_values[c_vertexW]) + 0x8000;

		DrawSegment(p_render, pixels, rest, &uTexel, Slope(uEnd - uTexel, rest), &vTexel, Slope(vEnd - vTexel, rest));
	}
}

// --- Polygons ---

static void DrawLine(Render* p_render)
{
	int32_t i;

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
	const uint32_t* vertex;
	const uint32_t* top = NULL;
	int32_t topY = 0x7fffffff;
	int32_t bottomY = -0x7fffffff - 1;
	uint32_t lines;

	for (vertex = p_render->m_vertices; vertex != p_render->m_verticesEnd; vertex += VERTEX_WORDS) {
		int32_t y = PortableS32(vertex[c_vertexY]);

		if (y <= topY) {
			topY = y;
			top = vertex;
		}

		if (y >= bottomY) {
			bottomY = y;
		}
	}

	lines = (uint32_t) bottomY - (uint32_t) topY;
	if (!lines) {
		return;
	}

	p_render->m_left.m_next = top;
	p_render->m_right.m_next = top;
	StartFirstEdge(p_render, &p_render->m_left);
	StartFirstEdge(p_render, &p_render->m_right);
	p_render->m_row = (uint32_t) topY * p_render->m_pitch;

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
	const uint32_t* p_vlist,
	int32_t p_nvertices,
	int32_t p_operation,
	uint32_t p_color,
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
	render.m_pitch = (uint32_t) p_pane->m_window->m_xMax + 1;
	render.m_vertices = p_vlist;
	render.m_verticesEnd = p_vlist + p_nvertices * VERTEX_WORDS;
	render.m_operation = p_operation;
	render.m_color = p_color;
	render.m_texture = p_texture;
	render.m_cueing = (const uint8_t*) p_cueing;
	render.m_translucency = (const uint8_t*) p_translucency;
	render.m_dither[0] = g_ditherLevels[0];
	render.m_dither[1] = g_ditherLevels[1];
	SelectPasses(&render);
	ScanConvert(&render);
}

void VFX_polygon_render(
	PANE* p_pane,
	uint32_t* p_vlist,
	int32_t p_nvertices,
	int32_t p_operation,
	uint32_t p_color,
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

static const uint32_t g_clipBits[] = {8, 1, 4, 2};

// The words a clipped vertex interpolates besides x and y, by operation (UV_clip, W_clip,
// C_clip).
typedef struct ClipWords {
	int32_t m_words[4];
	int32_t m_count;
} ClipWords;

static void SelectClipWords(ClipWords* p_words, int32_t p_operation)
{
	int32_t shade = p_operation & c_shadeMask;
	int32_t transparency = p_operation & c_transparentMask;

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

static int32_t IsInside(int32_t p_side, int32_t p_value, int32_t p_clip)
{
	return p_side == c_clipRight || p_side == c_clipTop ? p_value <= p_clip : p_value >= p_clip;
}

// CLIP_X_LINE and CLIP_Y_LINE: the point of the edge from p_second to p_first on the side, as
// p_second plus the edge times the fraction of it inside. Writes x, y and p_words only.
static void ClipEdge(
	const uint32_t* p_first,
	const uint32_t* p_second,
	uint32_t* p_out,
	int32_t p_side,
	int32_t p_clip,
	const ClipWords* p_words
)
{
	int32_t across = p_side == c_clipRight || p_side == c_clipLeft ? c_vertexX : c_vertexY;
	int32_t along = across == c_vertexX ? c_vertexY : c_vertexX;
	uint32_t fraction = DivideToF30((uint32_t) p_clip - p_second[across], p_first[across] - p_second[across]);
	int32_t i;

	p_out[across] = (uint32_t) p_clip;
	p_out[along] = p_second[along] + MultiplyF30(p_first[along] - p_second[along], fraction);
	for (i = 0; i < p_words->m_count; i++) {
		int32_t word = p_words->m_words[i];

		p_out[word] = p_second[word] + MultiplyF30(p_first[word] - p_second[word], fraction);
	}
}

// One side: each edge from p_first to p_second copies p_first if it's inside, and adds the edge's
// crossing if one end is outside. Returns the vertices written.
static uint32_t ClipSide(
	const uint32_t* p_source,
	uint32_t p_count,
	uint32_t* p_destination,
	int32_t p_side,
	int32_t p_clip,
	const ClipWords* p_words
)
{
	int32_t across = p_side == c_clipRight || p_side == c_clipLeft ? c_vertexX : c_vertexY;
	const uint32_t* first = p_source + (p_count - 1) * VERTEX_WORDS;
	const uint32_t* second = p_source;
	uint32_t out = 0;
	uint32_t i;

	for (i = 0; i < p_count; i++) {
		int32_t firstInside = IsInside(p_side, PortableS32(first[across]), p_clip);
		int32_t secondInside = IsInside(p_side, PortableS32(second[across]), p_clip);

		if (firstInside) {
			PORTABLE_ASSERT(p_destination != g_workPoints || out < MAX_WORK_VERTICES);
			memcpy(p_destination + out * VERTEX_WORDS, first, VERTEX_WORDS * sizeof(uint32_t));
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
	uint32_t* p_vlist,
	int32_t p_nvertices,
	int32_t p_operation,
	uint32_t p_color,
	VFX_TEXTURE* p_texture,
	void* p_cueing,
	void* p_translucency
)
{
	int32_t clips[4];
	uint32_t needed = 0;
	uint32_t count = (uint32_t) p_nvertices;
	uint32_t* source = p_vlist;
	uint32_t* destination = g_workPoints;
	uint32_t i;

	clips[c_clipRight] = p_pane->m_x1;
	clips[c_clipBottom] = p_pane->m_y0;
	clips[c_clipLeft] = p_pane->m_x0;
	clips[c_clipTop] = p_pane->m_y1;

	// The outcodes: the signs of each vertex's distances inside the sides
	PORTABLE_ASSERT(p_nvertices > 0);
	for (i = 0; i < count; i++) {
		const uint32_t* vertex = p_vlist + i * VERTEX_WORDS;
		uint32_t x = vertex[c_vertexX];
		uint32_t y = vertex[c_vertexY];

		needed |= ((uint32_t) clips[c_clipRight] - x) >> 31 << 3;
		needed |= (x - (uint32_t) clips[c_clipLeft]) >> 31 << 2;
		needed |= ((uint32_t) clips[c_clipTop] - y) >> 31 << 1;
		needed |= (y - (uint32_t) clips[c_clipBottom]) >> 31;
	}

	if (needed) {
		ClipWords words;
		int32_t side;

		SelectClipWords(&words, p_operation);
		for (side = c_clipRight; side <= c_clipTop; side++) {
			uint32_t* swap;

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

	RenderPolygon(p_pane, source, (int32_t) count, p_operation, p_color, p_texture, p_cueing, p_translucency);
}
