/* VFX3D, Miles Design VFX's polygon fillers (3rdparty/vfx/VFX3D.ASM), for builds with other
   compilers (COMPAT_MODE): portable C, tested against the assembly by tests/asmequiv. The VC++ 4.1
   build assembles VFX3D.ASM with MASM 6.11 instead; both DLLs compile this file in its place.

   The fillers take a polygon as an array of six-dword vertices: x and y, then the vertex's color
   (16.16) or texture coordinates (16.16, at +0x0c and +0x10). They walk its left and right edges
   down from the top vertex, one scan line at a time, in 16.16 with a half added for rounding, and
   fill the span between them, clipped to the pane. The assembly keeps its working variables in
   its data; none outlives a call except the lookaside table VFX_map_lookaside copies for
   VFX_map_polygon, so the C keeps the others in locals.

   The C does the assembly's arithmetic: 32-bit sums that wrap, slopes from a 64-bit idiv, steps
   that only carry within the 24 bits a color byte and its fraction take. Out of domain, as the
   game never draws them: a polygon whose divisions fault (edges and spans 0x8000 pixels long or
   more overflow them), and one whose vertices all lie below y 0x7fff in a view taller than that
   (the assembly then starts from the top vertex a previous call found). In the game, polygons are convex: then the
   edges stay between their vertices, and so do the spans and the texture coordinates. */
#include "vfx3d.h"

#include "compat.h"
#include "decomp.h"
#include "pane.h"
#include "portable.h"
#include "types.h"
#include "window.h"

#include <string.h>

#define VERTEX_WORDS 6
#define MAX_VALUES 2

// VFX_map_lookaside's table (VFX3D.ASM's lookaside), for VFX_map_polygon's translated modes.
static MechU8 g_lookaside[0x100];

// The vertex words.
enum VertexWord {
	c_vertexX,
	c_vertexY,
	c_vertexColor,
	c_vertexU,
	c_vertexV
};

// The modes of VFX_map_polygon: the assembly's span routines, in __map_logic's order.
enum TextureMode {
	c_textureCopy,            // M_write
	c_textureLuma,            // MX_write: through the luma table
	c_textureTransparent,     // MT_write: texel 0xff isn't drawn
	c_textureLumaTransparent, // MTX_write: through the luma table, which maps to 0xff for none
	c_textureModeCount
};

// One side of the polygon: an edge, and where the walk along it has got to.
typedef struct PolyEdge {
	const MechU32* m_start;           // the edge's top vertex
	const MechU32* m_end;             // the next one along the walk
	MechU32 m_count;                  // the scan lines left on the edge
	MechU32 m_x;                      // 16.16
	MechU32 m_xStep;                  // per scan line
	MechU32 m_values[MAX_VALUES];     // the vertex words interpolated, 16.16
	MechU32 m_valueSteps[MAX_VALUES]; // per scan line
} PolyEdge;

typedef struct Polygon {
	MechU8* m_pixels;
	MechU32 m_pitch;
	MechS32 m_maxX; // the view's width and height minus one, clipped to the buffer
	MechS32 m_maxY;
	const MechU32* m_vertices;
	const MechU32* m_verticesEnd;
	const MechS32* m_words; // the vertex words the edges interpolate
	MechS32 m_wordCount;
	MechS32 m_clipX;        // whether a vertex lies left or right of the view
	MechU32 m_clippedLines; // the scan lines above the view
	MechU32 m_lines;        // the scan lines left after the current one
	MechU32 m_row;          // the current scan line's start, from m_pixels
	PolyEdge m_left;        // walks the vertices backwards
	PolyEdge m_right;       // forwards
} Polygon;

// A span between the two edges: their ends ordered by x.
typedef struct PolySpan {
	const PolyEdge* m_low;
	const PolyEdge* m_high;
	MechS32 m_left; // pixels, from the view's left
	MechS32 m_right;
} PolySpan;

static const MechS32 g_colorWords[] = {c_vertexColor};
static const MechS32 g_textureWords[] = {c_vertexU, c_vertexV};

// `sar 16`: a 16.16 value's integer part.
static MechS32 Integer(MechU32 p_value)
{
	return PortableSar32(PortableS32(p_value), 16);
}

// The step that takes a value (16.16) by p_delta over p_count steps: the assembly shifts both in
// their registers before it divides, p_delta into a 64-bit dividend, p_count into a 32-bit
// divisor (which a count of 0x8000 or more overflows).
static MechU32 Slope(MechU32 p_delta, MechU32 p_count)
{
	return (MechU32) PortableIdiv((MechS64) PortableS32(p_delta) * 0x10000, PortableS32(p_count << 16));
}

// p_count steps of p_step at once: `imul` by the count shifted to 16.16, then `shrd 16`.
static MechU32 Advance(MechU32 p_step, MechU32 p_count)
{
	MechS64 product = (MechS64) PortableS32(p_step) * PortableS32(p_count << 16);

	return (MechU32) ((MechU64) product >> 16);
}

static const MechU32* PreviousVertex(const Polygon* p_polygon, const MechU32* p_vertex)
{
	if (p_vertex == p_polygon->m_vertices) {
		p_vertex = p_polygon->m_verticesEnd;
	}

	return p_vertex - VERTEX_WORDS;
}

static const MechU32* NextVertex(const Polygon* p_polygon, const MechU32* p_vertex)
{
	p_vertex += VERTEX_WORDS;
	if (p_vertex == p_polygon->m_verticesEnd) {
		p_vertex = p_polygon->m_vertices;
	}

	return p_vertex;
}

// Moves an edge to the one after it along its walk, without starting it.
static void AdvanceEdge(const Polygon* p_polygon, PolyEdge* p_edge)
{
	p_edge->m_start = p_edge->m_end;
	if (p_edge == &p_polygon->m_left) {
		p_edge->m_end = PreviousVertex(p_polygon, p_edge->m_start);
	}
	else {
		p_edge->m_end = NextVertex(p_polygon, p_edge->m_start);
	}
}

static void StartEdge(const Polygon* p_polygon, PolyEdge* p_edge, MechU32 p_count)
{
	const MechU32* start = p_edge->m_start;
	const MechU32* end = p_edge->m_end;
	MechS32 i;

	p_edge->m_count = p_count;
	p_edge->m_xStep = Slope((end[c_vertexX] - start[c_vertexX]) << 16, p_count);
	p_edge->m_x = (start[c_vertexX] << 16) + 0x8000;
	for (i = 0; i < p_polygon->m_wordCount; i++) {
		MechS32 word = p_polygon->m_words[i];

		p_edge->m_valueSteps[i] = Slope(end[word] - start[word], p_count);
		p_edge->m_values[i] = start[word] + 0x8000;
	}
}

// The first edge on a side that reaches below the view's top and isn't horizontal.
static void StartFirstEdge(const Polygon* p_polygon, PolyEdge* p_edge)
{
	for (;;) {
		MechS32 startY;
		MechS32 endY;

		AdvanceEdge(p_polygon, p_edge);
		startY = PortableS32(p_edge->m_start[c_vertexY]);
		endY = PortableS32(p_edge->m_end[c_vertexY]);
		if ((startY >= 0 || endY > 0) && endY != startY) {
			break;
		}
	}

	StartEdge(p_polygon, p_edge, p_edge->m_end[c_vertexY] - p_edge->m_start[c_vertexY]);
}

// The next edge, when the scan lines reach the end of one: any, a horizontal one for a line.
static void StartNextEdge(const Polygon* p_polygon, PolyEdge* p_edge)
{
	MechU32 count;

	AdvanceEdge(p_polygon, p_edge);
	count = p_edge->m_end[c_vertexY] - p_edge->m_start[c_vertexY];
	StartEdge(p_polygon, p_edge, count ? count : 1);
}

static void StepEdge(const Polygon* p_polygon, PolyEdge* p_edge)
{
	MechS32 i;

	p_edge->m_x += p_edge->m_xStep;
	for (i = 0; i < p_polygon->m_wordCount; i++) {
		p_edge->m_values[i] += p_edge->m_valueSteps[i];
	}
}

// Moves an edge down to the view's top.
static void ClipEdge(const Polygon* p_polygon, PolyEdge* p_edge)
{
	MechU32 lines = 0 - p_edge->m_start[c_vertexY];
	MechS32 i;

	p_edge->m_count -= lines;
	p_edge->m_x += Advance(p_edge->m_xStep, lines);
	for (i = 0; i < p_polygon->m_wordCount; i++) {
		p_edge->m_values[i] += Advance(p_edge->m_valueSteps[i], lines);
	}
}

// Clips the view to its buffer, finds the polygon's top and bottom, and starts its edges at the
// first scan line inside the view. Returns 0 when nothing of it can be visible.
static MechS32 StartPolygon(
	Polygon* p_polygon,
	PANE* p_view,
	MechS32 p_count,
	const MechU32* p_vertices,
	const MechS32* p_words,
	MechS32 p_wordCount
)
{
	WINDOW* buffer = p_view->m_window;
	const MechU32* vertex;
	const MechU32* top = NULL;
	MechS32 right;
	MechS32 left;
	MechS32 bottom;
	MechS32 viewTop;
	MechS32 minY;
	MechS32 maxY;
	MechU32 inside;

	PORTABLE_ASSERT(p_count > 0);
	p_polygon->m_pixels = buffer->m_buffer;
	p_polygon->m_pitch = (MechU32) buffer->m_xMax + 1;
	right = buffer->m_xMax < p_view->m_x1 ? buffer->m_xMax : p_view->m_x1;
	left = p_view->m_x0 > 0 ? p_view->m_x0 : 0;
	if (right < left) {
		return 0;
	}

	bottom = buffer->m_yMax < p_view->m_y1 ? buffer->m_yMax : p_view->m_y1;
	viewTop = p_view->m_y0 > 0 ? p_view->m_y0 : 0;
	if (bottom < viewTop) {
		return 0;
	}

	p_polygon->m_maxX = PortableS32((MechU32) right - (MechU32) left);
	p_polygon->m_maxY = PortableS32((MechU32) bottom - (MechU32) viewTop);
	p_polygon->m_row = (MechU32) viewTop * p_polygon->m_pitch + (MechU32) left;
	p_polygon->m_vertices = p_vertices;
	p_polygon->m_verticesEnd = p_vertices + p_count * VERTEX_WORDS;
	p_polygon->m_words = p_words;
	p_polygon->m_wordCount = p_wordCount;

	// The sides of the view each vertex lies beyond: a side all of them lie beyond hides the
	// polygon. The top is the last vertex of the least y.
	p_polygon->m_clipX = 0;
	inside = 0xf;
	minY = 0x7fff;
	maxY = -0x8000;
	vertex = p_vertices;
	do {
		MechU32 x = vertex[c_vertexX];
		MechU32 y = vertex[c_vertexY];
		MechU32 sides = (x >> 31) << 1 | ((MechU32) p_polygon->m_maxX - x) >> 31;

		p_polygon->m_clipX |= sides;
		sides = sides << 2 | (y >> 31) << 1 | ((MechU32) p_polygon->m_maxY - y) >> 31;
		if (PortableS32(y) <= minY) {
			minY = PortableS32(y);
			top = vertex;
		}

		if (PortableS32(y) >= maxY) {
			maxY = PortableS32(y);
		}

		inside &= sides;
		vertex += VERTEX_WORDS;
	} while (vertex != p_polygon->m_verticesEnd);

	if (inside) {
		return 0;
	}

	PORTABLE_ASSERT(top != NULL);
	if (maxY == minY) {
		return 0;
	}

	p_polygon->m_left.m_end = top;
	p_polygon->m_right.m_end = top;
	StartFirstEdge(p_polygon, &p_polygon->m_left);
	StartFirstEdge(p_polygon, &p_polygon->m_right);

	if (maxY > p_polygon->m_maxY) {
		p_polygon->m_lines = (MechU32) p_polygon->m_maxY - (MechU32) minY;
	}
	else {
		p_polygon->m_lines = (MechU32) maxY - (MechU32) minY;
	}

	p_polygon->m_clippedLines = 0;
	if (minY < 0) {
		p_polygon->m_clippedLines = 0 - (MechU32) minY;
		p_polygon->m_lines -= p_polygon->m_clippedLines;
		minY = 0;
		ClipEdge(p_polygon, &p_polygon->m_left);
		ClipEdge(p_polygon, &p_polygon->m_right);
	}

	p_polygon->m_row += (MechU32) minY * p_polygon->m_pitch;
	return 1;
}

// Steps both edges to the next scan line. Returns 0 after the last.
static MechS32 NextLine(Polygon* p_polygon)
{
	p_polygon->m_row += p_polygon->m_pitch;
	p_polygon->m_lines--;
	if (PortableS32(p_polygon->m_lines) < 0) {
		return 0;
	}

	// The last line steps both edges once more, whatever their counts
	if (p_polygon->m_lines == 0) {
		StepEdge(p_polygon, &p_polygon->m_left);
		StepEdge(p_polygon, &p_polygon->m_right);
		return 1;
	}

	if (--p_polygon->m_left.m_count == 0) {
		StartNextEdge(p_polygon, &p_polygon->m_left);
	}
	else {
		StepEdge(p_polygon, &p_polygon->m_left);
	}

	if (--p_polygon->m_right.m_count == 0) {
		StartNextEdge(p_polygon, &p_polygon->m_right);
	}
	else {
		StepEdge(p_polygon, &p_polygon->m_right);
	}

	return 1;
}

// The scan line's span between the edges, unclipped. Returns 0 when it lies beyond the view
// (always tested when p_clip is set: only then can it).
static MechS32 StartSpan(const Polygon* p_polygon, PolySpan* p_span, MechS32 p_clip)
{
	if (PortableS32(p_polygon->m_right.m_x) > PortableS32(p_polygon->m_left.m_x)) {
		p_span->m_low = &p_polygon->m_left;
		p_span->m_high = &p_polygon->m_right;
	}
	else {
		p_span->m_low = &p_polygon->m_right;
		p_span->m_high = &p_polygon->m_left;
	}

	p_span->m_left = Integer(p_span->m_low->m_x);
	p_span->m_right = Integer(p_span->m_high->m_x);
	return !p_clip || (p_span->m_left <= p_polygon->m_maxX && p_span->m_right >= 0);
}

static MechU8* SpanPixels(const Polygon* p_polygon, MechS32 p_left)
{
	return p_polygon->m_pixels + (MechU32) (p_polygon->m_row + (MechU32) p_left);
}

// --- VFX_flat_polygon ---

// Fills a polygon with the first vertex's color, rounded.
void VFX_flat_polygon(PANE* p_pane, MechS32 p_vcnt, MechU32* p_vlist)
{
	Polygon polygon;
	MechU8 color;

	if (!StartPolygon(&polygon, p_pane, p_vcnt, p_vlist, NULL, 0)) {
		return;
	}

	color = (MechU8) ((p_vlist[c_vertexColor] + 0x8000) >> 16);
	do {
		PolySpan span;
		MechU32 count;

		if (StartSpan(&polygon, &span, polygon.m_clipX)) {
			count = (MechU32) span.m_right - (MechU32) span.m_left + 1;
			if (polygon.m_clipX) {
				if (span.m_left < 0) {
					count += (MechU32) span.m_left;
					span.m_left = 0;
				}

				if (span.m_right > polygon.m_maxX) {
					count -= (MechU32) span.m_right - (MechU32) polygon.m_maxX;
				}
			}

			memset(SpanPixels(&polygon, span.m_left), color, count);
		}
	} while (NextLine(&polygon));
}

// --- The shaded fillers ---

// A shaded span, clipped: its pixels, the pixels clipped on its left, and its color (16.16) at
// the first pixel drawn and its step. The assembly keeps a color as a byte and a 16-bit fraction,
// so only the low 24 bits of the sums count.
typedef struct ShadedSpan {
	PolySpan m_span;
	MechU32 m_count;
	MechU32 m_skipped;
	MechU32 m_color;
	MechU32 m_step;
} ShadedSpan;

// p_dithered: the step is over half the span, for each of the two dithered colors. Returns 0 when
// the span lies beyond the view.
static MechS32 StartShadedSpan(const Polygon* p_polygon, ShadedSpan* p_span, MechS32 p_dithered)
{
	PolySpan* span = &p_span->m_span;
	MechU32 width;

	if (!StartSpan(p_polygon, span, p_polygon->m_clipX)) {
		return 0;
	}

	width = (MechU32) span->m_right - (MechU32) span->m_left;
	p_span->m_count = width + 1;
	p_span->m_skipped = 0;
	p_span->m_color = span->m_low->m_values[0];
	p_span->m_step = 0; // unused by a span of one pixel, which the assembly doesn't divide for
	if (width) {
		MechU32 delta = span->m_high->m_values[0] - span->m_low->m_values[0];

		if (p_dithered) {
			width = (MechU32) PortableSar32(PortableS32(width), 1);
			p_span->m_step = Slope(delta, width ? width : 1);
		}
		else {
			p_span->m_step = Slope(delta, width);
		}
	}

	if (p_polygon->m_clipX) {
		if (span->m_left < 0) {
			p_span->m_skipped = 0 - (MechU32) span->m_left;
			p_span->m_count -= p_span->m_skipped;
			span->m_left = 0;
			if (p_dithered) {
				p_span->m_color += Advance(p_span->m_step, p_span->m_skipped >> 1);
			}
			else {
				p_span->m_color += Advance(p_span->m_step, p_span->m_skipped);
			}
		}

		if (span->m_right > p_polygon->m_maxX) {
			p_span->m_count -= (MechU32) span->m_right - (MechU32) p_polygon->m_maxX;
		}
	}

	return 1;
}

// VFX_Gouraud_polygon: a polygon shaded from its vertices' colors.
void VFX_Gouraud_polygon(PANE* p_pane, MechS32 p_vcnt, MechU32* p_vlist)
{
	Polygon polygon;

	if (!StartPolygon(&polygon, p_pane, p_vcnt, p_vlist, g_colorWords, 1)) {
		return;
	}

	do {
		ShadedSpan span;

		if (StartShadedSpan(&polygon, &span, 0)) {
			MechU8* pixels = SpanPixels(&polygon, span.m_span.m_left);
			MechU32 color = span.m_color;
			MechU32 i;

			for (i = 0; i < span.m_count; i++) {
				pixels[i] = (MechU8) (color >> 16);
				color += span.m_step;
			}
		}
	} while (NextLine(&polygon));
}

// A dithered span's colors: the span's color plus one offset on its even pixels and plus the
// other on its odd ones, counted from its unclipped start. The two take turns, each stepping
// every other pixel; the offsets swap every scan line.
typedef struct Dither {
	MechU32 m_current;
	MechU32 m_next;
	MechU32 m_step;
} Dither;

static MechU8 NextShade(Dither* p_dither)
{
	MechU32 current = p_dither->m_current;

	p_dither->m_current = p_dither->m_next;
	p_dither->m_next = current + p_dither->m_step;
	return (MechU8) (current >> 16);
}

static void StartDither(Dither* p_dither, const ShadedSpan* p_span, const MechU32* p_offsets)
{
	p_dither->m_current = p_span->m_color + p_offsets[0];
	p_dither->m_next = p_span->m_color + p_offsets[1];
	p_dither->m_step = p_span->m_step;
	if (p_span->m_skipped & 1) {
		NextShade(p_dither);
	}
}

static void StartDitherOffsets(const Polygon* p_polygon, MechU32* p_offsets, MechS32 p_dither)
{
	p_offsets[0] = (MechU32) p_dither;
	p_offsets[1] = 0;
	if (p_polygon->m_clippedLines & 1) {
		p_offsets[0] = 0;
		p_offsets[1] = (MechU32) p_dither;
	}
}

static void SwapDitherOffsets(MechU32* p_offsets)
{
	MechU32 offset = p_offsets[0];

	p_offsets[0] = p_offsets[1];
	p_offsets[1] = offset;
}

// VFX_dithered_Gouraud_polygon: VFX_Gouraud_polygon dithered, with p_dither added to every other pixel's color in a
// checkerboard (the game passes half a color, 0x7fff or 0x8000).
void VFX_dithered_Gouraud_polygon(PANE* p_pane, MechS32 p_ditherAmount, MechS32 p_vcnt, MechU32* p_vlist)
{
	Polygon polygon;
	MechU32 offsets[2];

	if (!StartPolygon(&polygon, p_pane, p_vcnt, p_vlist, g_colorWords, 1)) {
		return;
	}

	StartDitherOffsets(&polygon, offsets, p_ditherAmount);
	do {
		ShadedSpan span;

		if (StartShadedSpan(&polygon, &span, 1)) {
			MechU8* pixels = SpanPixels(&polygon, span.m_span.m_left);
			Dither dither;
			MechU32 i;

			StartDither(&dither, &span, offsets);
			for (i = 0; i < span.m_count; i++) {
				pixels[i] = NextShade(&dither);
			}
		}

		SwapDitherOffsets(offsets);
	} while (NextLine(&polygon));
}

// Adds a shade to a pixel.
static void AddShade(MechU8* p_pixel, Dither* p_dither)
{
	*p_pixel = (MechU8) (*p_pixel + NextShade(p_dither));
}

// VFX_illuminate_polygon: VFX_dithered_Gouraud_polygon's dithered shades added to the pixels already there. The
// assembly adds them two pixels at a time, as a word, so the carry out of the first pixel goes into the second; the
// pairs start at an even address, or right after an odd number of pixels clipped on the left.
void VFX_illuminate_polygon(PANE* p_pane, MechS32 p_ditherAmount, MechS32 p_vcnt, MechU32* p_vlist)
{
	Polygon polygon;
	MechU32 offsets[2];

	if (!StartPolygon(&polygon, p_pane, p_vcnt, p_vlist, g_colorWords, 1)) {
		return;
	}

	StartDitherOffsets(&polygon, offsets, p_ditherAmount);
	do {
		ShadedSpan span;

		if (StartShadedSpan(&polygon, &span, 1)) {
			MechU8* pixels = SpanPixels(&polygon, span.m_span.m_left);
			MechU32 count = span.m_count;
			Dither dither;

			StartDither(&dither, &span, offsets);
			if (!(span.m_skipped & 1) && ((MechU32) (size_t) pixels & 1)) {
				AddShade(pixels++, &dither);
				count--;
			}

			while (count >= 2) {
				MechU32 sum = pixels[0] + (MechU32) NextShade(&dither);

				pixels[0] = (MechU8) sum;
				pixels[1] = (MechU8) (pixels[1] + NextShade(&dither) + (sum >> 8));
				pixels += 2;
				count -= 2;
			}

			if (count) {
				AddShade(pixels, &dither);
			}
		}

		SwapDitherOffsets(offsets);
	} while (NextLine(&polygon));
}

// --- VFX_translate_polygon ---

// Maps the pixels under the polygon through p_table, a 256-byte table (a shadow or a tint).
void VFX_translate_polygon(PANE* p_pane, MechS32 p_vcnt, MechU32* p_vlist, MechU8* p_lookaside)
{
	Polygon polygon;

	if (!StartPolygon(&polygon, p_pane, p_vcnt, p_vlist, NULL, 0)) {
		return;
	}

	do {
		PolySpan span;

		if (StartSpan(&polygon, &span, 1)) {
			MechU8* pixels;
			MechS32 i;

			if (span.m_right != span.m_left) {
				if (span.m_left < 0) {
					span.m_left = 0;
				}

				if (span.m_right > polygon.m_maxX) {
					span.m_right = polygon.m_maxX;
				}
			}

			pixels = SpanPixels(&polygon, span.m_left);
			for (i = 0; i <= span.m_right - span.m_left; i++) {
				pixels[i] = p_lookaside[pixels[i]];
			}
		}
	} while (NextLine(&polygon));
}

// --- VFX_map_polygon ---

void VFX_map_lookaside(MechU16* p_table)
{
	memcpy(g_lookaside, p_table, sizeof(g_lookaside));
}

// A span's walk through the texture: the texel offset, and the fractions of u and v, which carry
// into it. With a negative step, the assembly steps the complement of the fraction up instead,
// and the carry takes the texel back.
typedef struct TextureWalk {
	MechU32 m_texel;
	MechU32 m_u;
	MechU32 m_uStep;
	MechU32 m_v;
	MechU32 m_vStep;
	MechU32 m_offsets[4]; // the texel step, by the carries out of u's fraction and v's
} TextureWalk;

// A texture step's whole texels, rounded towards zero: the step's top half, plus one when it's
// negative with a fraction. The assembly sign-extends u's to 32 bits first; v's stays a word,
// which it multiplies by the pitch as one (0xffff plus one is 0x10000, a multiple of 0x10000).
static MechU32 WholeTexels(MechU32 p_step, MechS32 p_extend)
{
	MechU32 whole = (p_step >> 16) & 0xffff;

	if (PortableS32(p_step) < 0) {
		if (p_extend) {
			whole |= 0xffff0000;
		}

		if (p_step & 0xffff) {
			whole++;
		}
	}

	return whole;
}

static void StartFraction(MechU32 p_value, MechU32 p_step, MechU32* p_fraction, MechU32* p_fractionStep)
{
	if (PortableS32(p_step) < 0) {
		p_step = 0 - p_step;
		p_value = ~p_value;
	}

	*p_fraction = p_value << 16;
	*p_fractionStep = p_step << 16;
}

// Whether a + b carries out of 32 bits, and the sum.
static MechU32 AddCarry(MechU32* p_value, MechU32 p_step)
{
	MechU32 value = *p_value;

	*p_value = value + p_step;
	return *p_value < value;
}

static MechU8 Texel(const WINDOW* p_texture, TextureWalk* p_walk)
{
	return p_texture->m_buffer[p_walk->m_texel];
}

static void StepTexel(TextureWalk* p_walk)
{
	MechU32 carry = AddCarry(&p_walk->m_u, p_walk->m_uStep) << 1;

	carry |= AddCarry(&p_walk->m_v, p_walk->m_vStep);
	p_walk->m_texel += p_walk->m_offsets[carry];
}

// Maps a polygon with a texture, through p_mode's span routine (c_textureCopy...). The vertices'
// u and v are in texels, 16.16; the texture's rows are m_maxX + 1 texels apart. The luma table
// is VFX_map_lookaside's, from an earlier call.
void VFX_map_polygon(PANE* p_pane, MechS32 p_vcnt, MechU32* p_vlist, WINDOW* p_texture, MechS32 p_flags)
{
	const MechU8* luma = g_lookaside;
	MechU32 texturePitch = (MechU32) p_texture->m_xMax + 1;
	Polygon polygon;

	PORTABLE_ASSERT(p_flags >= 0 && p_flags < c_textureModeCount);
	if (!StartPolygon(&polygon, p_pane, p_vcnt, p_vlist, g_textureWords, 2)) {
		return;
	}

	do {
		PolySpan span;

		if (StartSpan(&polygon, &span, 1)) {
			MechU32 u = span.m_low->m_values[0];
			MechU32 v = span.m_low->m_values[1];
			MechU32 uStep = 0;
			MechU32 vStep = 0;
			TextureWalk walk;
			MechU8* pixels;
			MechS32 count;
			MechS32 i;

			// The walk's steps, which a span of one pixel doesn't take
			memset(&walk, 0, sizeof(walk));
			if (span.m_right != span.m_left) {
				MechU32 width = (MechU32) span.m_right - (MechU32) span.m_left;
				MechU32 uWhole;
				MechU32 vOffset;
				MechU32 vCarry;

				// The texel steps: the whole texels, and one more in the step's direction when
				// the fraction carries
				uStep = Slope(span.m_high->m_values[0] - u, width);
				uWhole = WholeTexels(uStep, 1);
				vStep = Slope(span.m_high->m_values[1] - v, width);
				vOffset = (MechU32) PortableS16((MechU16) (texturePitch * WholeTexels(vStep, 0)));
				vCarry = PortableS32(vStep) < 0 ? 0 - texturePitch : texturePitch;
				walk.m_offsets[0] = uWhole + vOffset;
				walk.m_offsets[1] = walk.m_offsets[0] + vCarry;
				walk.m_offsets[2] = uWhole + (PortableS32(uStep) < 0 ? 0xffffffff : 1) + vOffset;
				walk.m_offsets[3] = walk.m_offsets[2] + vCarry;

				if (span.m_left < 0) {
					MechU32 skipped = 0 - (MechU32) span.m_left;

					span.m_left = 0;
					u += Advance(uStep, skipped);
					v += Advance(vStep, skipped);
				}

				if (span.m_right > polygon.m_maxX) {
					span.m_right = polygon.m_maxX;
				}
			}

			walk.m_texel = (v >> 16) * texturePitch + (u >> 16);
			StartFraction(u, uStep, &walk.m_u, &walk.m_uStep);
			StartFraction(v, vStep, &walk.m_v, &walk.m_vStep);
			pixels = SpanPixels(&polygon, span.m_left);
			count = span.m_right - span.m_left + 1;
			for (i = 0; i < count; i++) {
				MechU8 texel = Texel(p_texture, &walk);

				switch (p_flags) {
				case c_textureCopy:
					pixels[i] = texel;
					break;
				case c_textureLuma:
					pixels[i] = luma[texel];
					break;
				case c_textureTransparent:
					if (texel != 0xff) {
						pixels[i] = texel;
					}
					break;
				default:
					if (luma[texel] != 0xff) {
						pixels[i] = luma[texel];
					}
					break;
				}

				if (i + 1 < count) {
					StepTexel(&walk);
				}
			}
		}
	} while (NextLine(&polygon));
}
