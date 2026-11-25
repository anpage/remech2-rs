#include "screenscale.h"

#include "clock.h"
#include "decomp.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "fixedtrig.h"
#include "gaugequadrant.h"
#include "keyboard.h"
#include "menu.h"
#include "point.h"
#include "polydraw.h"
#include "rect.h"
#include "refreshmode.h"
#include "render.h"
#include "simmain.h"
#include "targeting.h"
#include "types.h"
#include "window.h"

#include <string.h>

// Rectangles and points in 16.16 fractions of the screen (or of a frame rectangle), and in the
// 320x200 coordinates the HUD tables were authored in.

// The margins FitRectToText leaves around a text block, in 16.16 fractions.
Point g_textMargins = {0x28f, 0x28f};

// DrawPulsingFrame's state: the frame's inset (1 or 2, alternating each call), its color (0xf0
// to 0xf7 and back), when the color last stepped and its step.
int32_t g_pulseInset = 0;

int32_t g_pulseColor = 0xf0;

int32_t g_pulseTime = 0;

int32_t g_pulseStep = 0;

// ScaleRectToScreen, ScaleBoundsToScreen, ScalePointToScreen and CenterRectOnScreen are
// implemented on the Rust side (src/sim/window.rs). In widescreen they place the HUD in a 4:3 box
// centred in the frame.

// Maps 16.16 fractions of p_frame onto pixels.
PANE* ScaleRectToFrame(PANE* p_frame, PANE* p_src, PANE* p_dst)
{
	int32_t width;
	int32_t height;

	width = p_frame->m_x1 - p_frame->m_x0;
	height = p_frame->m_y1 - p_frame->m_y0;
	p_dst->m_x0 = FixedMul16(p_src->m_x0, width);
	p_dst->m_y0 = FixedMul16(p_src->m_y0, height);
	p_dst->m_x1 = FixedMul16(p_src->m_x1, width);
	p_dst->m_y1 = FixedMul16(p_src->m_y1, height);
	p_dst->m_x0 += p_frame->m_x0;
	p_dst->m_y0 += p_frame->m_y0;
	p_dst->m_x1 += p_frame->m_x0;
	p_dst->m_y1 += p_frame->m_y0;
	return p_dst;
}

// Maps 16.16 fractions of p_frame's size onto pixels, relative to the frame (unlike
// ScaleRectToFrame, it doesn't add the frame's origin).
Rect* ScaleBoundsToFrame(PANE* p_frame, Rect* p_src, Rect* p_dst)
{
	int32_t width;
	int32_t height;

	width = p_frame->m_x1 - p_frame->m_x0;
	height = p_frame->m_y1 - p_frame->m_y0;
	p_dst->m_left = FixedMul16(width, p_src->m_left);
	p_dst->m_top = FixedMul16(height, p_src->m_top);
	p_dst->m_right = FixedMul16(width, p_src->m_right);
	p_dst->m_bottom = FixedMul16(height, p_src->m_bottom);
	return p_dst;
}

// Maps a point in 16.16 fractions of p_frame's size onto pixels, relative to the frame.
Point* ScalePointToFrame(PANE* p_frame, Point* p_src, Point* p_dst)
{
	int32_t width;
	int32_t height;

	width = p_frame->m_x1 - p_frame->m_x0;
	height = p_frame->m_y1 - p_frame->m_y0;
	p_dst->m_x = FixedMul16(width, p_src->m_x);
	p_dst->m_y = FixedMul16(height, p_src->m_y);
	return p_dst;
}

// Scales 16.16 fractions to 320x200 (low resolution) coordinates, correcting the height for the aspect ratio
// p_aspect (16.16, 0xd555 = 5:6 for none).
PANE* ScaleRectToLowRes(PANE* p_rect, int32_t p_aspect)
{
	int32_t scale;

	scale = FixedDiv16(p_aspect, 0xd555);
	p_rect->m_x0 = FixedMul16(p_rect->m_x0, 319);
	p_rect->m_y0 = FixedMul16(p_rect->m_y0, 199);
	p_rect->m_y0 = FixedMul16(p_rect->m_y0, scale);
	p_rect->m_x1 = FixedMul16(p_rect->m_x1, 319);
	p_rect->m_y1 = FixedMul16(p_rect->m_y1, 199);
	p_rect->m_y1 = FixedMul16(p_rect->m_y1, scale);
	return p_rect;
}

// Maps 320x200 coordinates to 16.16 fractions of the screen.
PANE* ScaleRectFromLowRes(PANE* p_src, PANE* p_dst)
{
	p_dst->m_x0 = FixedDiv16(p_src->m_x0, 319);
	p_dst->m_y0 = FixedDiv16(p_src->m_y0, 199);
	p_dst->m_x1 = FixedDiv16(p_src->m_x1, 319);
	p_dst->m_y1 = FixedDiv16(p_src->m_y1, 199);
	return p_dst;
}

Rect* ScaleBoundsFromLowRes(Rect* p_src, Rect* p_dst)
{
	p_dst->m_left = FixedDiv16(p_src->m_left, 319);
	p_dst->m_top = FixedDiv16(p_src->m_top, 199);
	p_dst->m_right = FixedDiv16(p_src->m_right, 319);
	p_dst->m_bottom = FixedDiv16(p_src->m_bottom, 199);
	return p_dst;
}

Point* ScalePointFromLowRes(Point* p_src, Point* p_dst)
{
	p_dst->m_x = FixedDiv16(p_src->m_x, 319);
	p_dst->m_y = FixedDiv16(p_src->m_y, 199);
	return p_dst;
}

// Scales p_src about its center by p_scale (16.16).
PANE* ScaleRectAboutCenter(PANE* p_src, PANE* p_dst, Point p_scale)
{
	int32_t centerX;
	int32_t centerY;

	centerX = ((p_src->m_x1 - p_src->m_x0 + 1) >> 1) + p_src->m_x0;
	centerY = ((p_src->m_y1 - p_src->m_y0 + 1) >> 1) + p_src->m_y0;
	p_dst->m_x0 = p_src->m_x0 - centerX;
	p_dst->m_y0 = p_src->m_y0 - centerY;
	p_dst->m_x1 = p_src->m_x1 - centerX;
	p_dst->m_y1 = p_src->m_y1 - centerY;
	p_dst->m_x0 = FixedMul16(p_dst->m_x0, p_scale.m_x);
	p_dst->m_y0 = FixedMul16(p_dst->m_y0, p_scale.m_y);
	p_dst->m_x1 = FixedMul16(p_dst->m_x1, p_scale.m_x);
	p_dst->m_y1 = FixedMul16(p_dst->m_y1, p_scale.m_y);
	p_dst->m_x0 += centerX;
	p_dst->m_y0 += centerY;
	p_dst->m_x1 += centerX;
	p_dst->m_y1 += centerY;
	return p_dst;
}

// Scales p_src about its center to the size of a shape frame.
PANE* FitRectToShape(PANE* p_src, PANE* p_dst, void* p_shape, int32_t p_frame)
{
	int32_t size;
	Point scale;

	size = VFX_shape_bounds(p_shape, p_frame);
	scale.m_x = size >> 16;
	scale.m_y = size & 0xffff;
	scale.m_x = FixedDiv16(scale.m_x, p_src->m_x1 - p_src->m_x0 + 1);
	scale.m_y = FixedDiv16(scale.m_y, p_src->m_y1 - p_src->m_y0 + 1);
	ScaleRectAboutCenter(p_src, p_dst, scale);
	return p_dst;
}

// Scales p_src about its center to the size of a shape.
PANE* FitRectToGif(PANE* p_src, PANE* p_dst, void* p_shape)
{
	int32_t size;
	Point scale;

	size = VFX_GIF_resolution(p_shape);
	scale.m_x = size >> 16;
	scale.m_y = size & 0xffff;
	scale.m_x = FixedDiv16(scale.m_x, p_src->m_x1 - p_src->m_x0 + 1);
	scale.m_y = FixedDiv16(scale.m_y, p_src->m_y1 - p_src->m_y0 + 1);
	ScaleRectAboutCenter(p_src, p_dst, scale);
	return p_dst;
}

// Outlines a pane.
void OutlinePane(PANE* p_target, int32_t p_color)
{
	int32_t width;
	int32_t height;

	width = p_target->m_x1 - p_target->m_x0;
	height = p_target->m_y1 - p_target->m_y0;
	VFX_line_draw(p_target, 0, 0, width, 0, 0, p_color);
	VFX_line_draw(p_target, width, 0, width, height, 0, p_color);
	VFX_line_draw(p_target, width, height, 0, height, 0, p_color);
	VFX_line_draw(p_target, 0, height, 0, 0, 0, p_color);
}

// Draws a line across the pane under a line of text at p_pos.
void DrawRuleUnderRow(PANE* p_target, Point p_pos, void* p_font, int32_t p_color)
{
	int32_t height;
	int32_t width;

	height = VFX_font_height(p_font);
	p_pos.m_x = 0;
	p_pos.m_y += height;
	width = p_target->m_x1 - p_target->m_x0 + 1;
	VFX_line_draw(p_target, p_pos.m_x, p_pos.m_y, width - 1, p_pos.m_y, 0, p_color);
}

// Underlines text drawn at p_pos.
void UnderlineText(PANE* p_target, char* p_text, Point p_pos, void* p_font, int32_t p_color)
{
	int32_t width;
	int32_t height;
	uint32_t i;

	width = 0;
	height = VFX_font_height(p_font);
	p_pos.m_y += height;
	for (i = 0; i < strlen(p_text); i++) {
		width += VFX_character_width(p_font, p_text[i]);
	}

	VFX_line_draw(p_target, p_pos.m_x, p_pos.m_y, p_pos.m_x + width - 1, p_pos.m_y, 0, p_color);
}

// Draws a box around text drawn at p_pos.m_x, p_pos.m_y.
void BoxText(PANE* p_target, char* p_text, Point p_pos, void* p_font, int32_t p_color)
{
	int32_t left;
	int32_t top;
	int32_t bottom;
	int32_t right;
	int32_t height;
	int32_t width;
	uint32_t i;

	width = 0;
	height = VFX_font_height(p_font);
	for (i = 0; i < strlen(p_text); i++) {
		width += VFX_character_width(p_font, p_text[i]);
	}

	left = p_pos.m_x;
	right = p_pos.m_x + width;
	bottom = p_pos.m_y + height + 1;
	top = p_pos.m_y - 1;
	VFX_line_draw(p_target, left, top, right, top, 0, p_color);
	VFX_line_draw(p_target, left, bottom, right, bottom, 0, p_color);
	VFX_line_draw(p_target, left, top, left, bottom, 0, p_color);
	VFX_line_draw(p_target, right, top, right, bottom, 0, p_color);
}

// Draws text word-wrapped into a pane, inside the margins, until it runs out of lines.
void DrawWrappedText(PANE* p_target, char* p_text, void* p_font)
{
	int32_t x;
	char* line;
	Point origin;
	char* end;
	int32_t length;
	char* breakAt;
	int32_t lineHeight;
	char* buffer;
	char* p;
	int32_t width;
	int32_t height;

	if (p_target == NULL || p_text == NULL || p_font == NULL) {
		return;
	}

	length = strlen(p_text);
	if (length == 0) {
		return;
	}

	buffer = p = MechHeapAlloc(g_primaryHeap, length + 1);
	end = buffer + length;
	strcpy(buffer, p_text);
	ScalePointToFrame(p_target, &g_textMargins, &origin);
	width = p_target->m_x1 - p_target->m_x0;
	height = p_target->m_y1 - p_target->m_y0;
	lineHeight = VFX_font_height(p_font);

	while (p <= end && origin.m_y < height) {
		line = p;
		x = origin.m_x;
		while (p <= end) {
			if (*p == '\n') {
				break;
			}

			x += VFX_character_width(p_font, *p);
			if (x > width) {
				break;
			}

			p++;
		}

		if (p <= end) {
			if (*p == '\n') {
				*p = '\0';
			}
			else {
				breakAt = p;
				while (p >= line && *p != ' ' && *p != '\t') {
					p--;
				}

				if (p < line) {
					p = breakAt;
					*p = '\0';
				}
				else {
					*p = '\0';
				}
			}
		}

		if (*line != '\0') {
			VFX_string_draw(p_target, origin.m_x, origin.m_y, p_font, line, g_textColors);
		}

		origin.m_y += lineHeight;
		p++;
	}

	MechHeapFree(g_primaryHeap, buffer);
}

// Sizes p_rect to fit a block of text, lines separated by newlines, plus the margins.
PANE* FitRectToText(char* p_text, void* p_font, PANE* p_rect)
{
	int32_t lineWidth;
	int32_t lineHeight;
	char* c;
	int32_t maxWidth;
	int32_t height;
	char ch;

	maxWidth = 0;
	height = 0;
	lineWidth = 0;
	if (p_rect == NULL || p_text == NULL || p_font == NULL) {
		return NULL;
	}

	c = p_text;
	height = lineHeight = VFX_font_height(p_font);
	while ((ch = *c++) != '\0') {
		if (ch == '\n') {
			if (lineWidth > maxWidth) {
				maxWidth = lineWidth;
			}

			lineWidth = 0;
			height += lineHeight;
		}
		else {
			lineWidth += VFX_character_width(p_font, ch);
		}
	}

	if (lineWidth > maxWidth) {
		maxWidth = lineWidth;
	}

	p_rect->m_x0 = p_rect->m_y0 = 0;
	p_rect->m_x1 = FixedDiv16(maxWidth, 0x10000 - g_textMargins.m_x);
	p_rect->m_y1 = FixedDiv16(height, 0x10000 - g_textMargins.m_y);
	return p_rect;
}

// A pane and the colour PlotCenteredPixel draws into it.
typedef struct CenteredDraw {
	PANE* m_target;  // 0x00
	uint8_t m_color; // 0x04
} CenteredDraw;

// Plots a pixel of p_draw's colour at (p_width / 2, -p_height / 2) from the centre of its target
// (16.16 fractions of the target), shows the frame, and returns FALSE if Esc was pressed.
int32_t PlotCenteredPixel(CenteredDraw* p_draw, int32_t p_width, int32_t p_height)
{
	int32_t result;
	PANE* target;
	Point pos;

	result = TRUE;
	if (!p_draw) {
		return result;
	}

	target = p_draw->m_target;
	if (!target) {
		return result;
	}

	pos.m_x = p_width;
	pos.m_y = p_height;
	pos.m_x = pos.m_x / 2 + 0x8000;
	pos.m_y = -(pos.m_y / 2) + 0x8000;
	ScalePointToFrame(target, &pos, &pos);
	VFX_pixel_write(target, pos.m_x, pos.m_y, p_draw->m_color);
	g_currentRefreshMode->m_flip();

	if (KeyboardPollKeyCode() == 0x1b) {
		result = FALSE;
	}

	return result;
}

// Draws a pulsing frame just inside a pane.
void DrawPulsingFrame(PANE* p_target)
{
	Point corner;

	if (g_currentClock - g_pulseTime >= 0x5a) {
		g_pulseTime = g_currentClock;
		g_pulseColor += g_pulseStep;
		if (g_pulseColor == 0xf0) {
			g_pulseStep = 1;
		}
		else if (g_pulseColor == 0xf7) {
			g_pulseStep = g_pulseStep * -1;
		}
	}

	g_pulseInset %= 2;
	g_pulseInset++;
	corner.m_x = p_target->m_x1 - g_pulseInset;
	corner.m_y = p_target->m_y1 - g_pulseInset;
	VFX_rectangle_hash(p_target, g_pulseInset, g_pulseInset, corner.m_x, corner.m_y, g_pulseColor);
}

// Tiles a pane with a shape frame.
void TilePane(PANE* p_target, void* p_shape, int32_t p_frame)
{
	int32_t size;
	int32_t tileWidth;
	int32_t tileHeight;
	int32_t columns;
	int32_t rows;
	int32_t row;
	int32_t column;
	int32_t x;
	int32_t y;

	x = 0;
	y = 0;
	if (p_target == NULL || p_shape == NULL) {
		return;
	}

	size = VFX_shape_bounds(p_shape, p_frame);
	tileWidth = size >> 16;
	tileHeight = size & 0xffff;
	columns = (p_target->m_x1 - p_target->m_x0 + tileWidth) / tileWidth;
	rows = (p_target->m_y1 - p_target->m_y0 + tileHeight) / tileHeight;
	for (row = 0; row < rows; row++) {
		for (column = 0; column < columns; column++) {
			VFX_shape_draw(p_target, p_shape, p_frame, x, y);
			x += tileWidth;
		}

		x = 0;
		y += tileHeight;
	}
}

// Returns 1 if the line through (0, 0) and (p_dx, p_dy) is too steep for a 16.16 slope,
// otherwise 0 with the slope in p_slope.
int32_t GetLineSlope(int32_t p_dx, int32_t p_dy, int32_t* p_slope)
{
	int32_t steep;

	if (p_dx == 0 || p_dy / p_dx > 0x7fff || p_dy / p_dx < -0x8000) {
		steep = 1;
	}
	else {
		steep = 0;
	}

	if (!steep) {
		*p_slope = FixedDiv16(p_dy, p_dx);
	}

	return steep;
}

// Where the needle from the center of a gauge rectangle towards p_point leaves the rectangle.
Point* GetRectNeedleToward(PANE* p_target, Point* p_point, Point* p_out)
{
	GaugeQuadrant quadrant;
	int32_t slope;
	Point half;
	int32_t dx;
	int32_t dy;

	half.m_x = (p_target->m_x1 - p_target->m_x0 + 1) >> 1;
	half.m_y = (p_target->m_y1 - p_target->m_y0 + 1) >> 1;
	quadrant.m_value = 0;

	dx = half.m_x - p_point->m_x;
	if (dx >= 0) {
		quadrant.m_bits.m_left = 1;
	}

	dy = half.m_y - p_point->m_y;
	if (dy >= 0) {
		quadrant.m_bits.m_up = 1;
	}

	quadrant.m_bits.m_steep = GetLineSlope(dx, dy, &slope);
	GetRectEdgeAtSlope(&half, quadrant, slope, p_out);
	return p_out;
}

// Where the needle from the center of a gauge rectangle at the heading p_angle (16.16 degrees)
// leaves the rectangle.
Point* GetRectNeedleAt(PANE* p_target, int32_t p_angle, Point* p_out)
{
	GaugeQuadrant quadrant;
	int32_t slope;
	Point half;
	int32_t dx;
	int32_t dy;

	p_angle = (p_angle % 0x1680000 + 0x1680000) % 0x1680000;
	half.m_x = (p_target->m_x1 - p_target->m_x0 + 1) >> 1;
	half.m_y = (p_target->m_y1 - p_target->m_y0 + 1) >> 1;
	quadrant.m_value = 0;

	if (p_angle >= 0x5a0000 && p_angle <= 0x10e0000) {
		quadrant.m_bits.m_left = 1;
	}
	else {
		quadrant.m_bits.m_left = 0;
	}

	if (p_angle <= 0xb40000) {
		quadrant.m_bits.m_up = 1;
	}
	else {
		quadrant.m_bits.m_up = 0;
	}

	dx = FixedCos(p_angle) >> 13;
	dy = FixedSin(p_angle) >> 13;
	dy = FixedMul16(dy, g_eyepoint->m_pixelAspect);
	quadrant.m_bits.m_steep = GetLineSlope(dx, dy, &slope);
	slope = -slope;
	GetRectEdgeAtSlope(&half, quadrant, slope, p_out);
	return p_out;
}

// Where a line from the center of a rectangle (p_half: its half size) leaves it, for the
// slope p_slope and the direction bits of p_quadrant (GetRectNeedleToward).
Point* GetRectEdgeAtSlope(Point* p_half, GaugeQuadrant p_quadrant, int32_t p_slope, Point* p_out)
{
	int32_t height;
	int32_t width;
	int32_t ratio;
	int32_t negRatio;
	Point result;

	width = p_half->m_x * 2;
	height = p_half->m_y * 2;
	if (p_half->m_x == 0) {
		ratio = 0;
	}
	else {
		ratio = FixedDiv16(p_half->m_y, p_half->m_x);
	}
	negRatio = -ratio;

	switch (p_quadrant.m_value) {
	case 3:
		if (ratio < p_slope) {
			result.m_y = 0;
			result.m_x = p_half->m_x - FixedDiv16(p_half->m_y, p_slope);
		}
		else {
			result.m_x = 0;
			result.m_y = p_half->m_y - FixedMul16(p_half->m_x, p_slope);
		}
		break;
	case 2:
		if (p_slope < negRatio) {
			result.m_y = 0;
			result.m_x = p_half->m_x - FixedDiv16(p_half->m_y, p_slope);
		}
		else {
			result.m_x = width;
			result.m_y = p_half->m_y + FixedMul16(p_half->m_x, p_slope);
		}
		break;
	case 0:
		if (ratio < p_slope) {
			result.m_y = height;
			result.m_x = p_half->m_x + FixedDiv16(p_half->m_y, p_slope);
		}
		else {
			result.m_x = width;
			result.m_y = p_half->m_y + FixedMul16(p_half->m_x, p_slope);
		}
		break;
	case 1:
		if (p_slope < negRatio) {
			result.m_y = height;
			result.m_x = p_half->m_x + FixedDiv16(p_half->m_y, p_slope);
		}
		else {
			result.m_x = 0;
			result.m_y = p_half->m_y - FixedMul16(p_half->m_x, p_slope);
		}
		break;
	case 7:
		result.m_y = 0;
		result.m_x = p_half->m_x;
		break;
	case 5:
		result.m_y = height;
		result.m_x = p_half->m_x;
		break;
	default:
		result.m_y = 0;
		result.m_x = 0;
		break;
	}

	*p_out = result;
	return p_out;
}

// Draws the ellipse inscribed in a pane, corrected for the pixel aspect.
void DrawGaugeEllipse(PANE* p_target, int32_t p_color)
{
	int32_t centerX;
	int32_t centerY;
	int32_t radiusY;
	int32_t radius;
	int32_t aspect;

	aspect = g_eyepoint->m_pixelAspect;
	centerX = (p_target->m_x1 - p_target->m_x0 + 1) >> 1;
	centerY = (p_target->m_y1 - p_target->m_y0 + 1) >> 1;
	radius = (p_target->m_x1 - p_target->m_x0 + 1) / 2 - 1;
	radiusY = FixedMul16(radius, aspect);
	VFX_ellipse_draw(p_target, centerX, centerY, radius, radiusY, p_color);
}

// Fills the part of the ellipse inscribed in a pane that lies in p_rect.
void FillGaugeEllipse(PANE* p_target, Rect* p_rect, int32_t p_color)
{
	PANE clip;
	int32_t centerX;
	int32_t centerY;
	int32_t radiusY;
	int32_t radius;
	int32_t aspect;

	aspect = g_eyepoint->m_pixelAspect;
	centerX = (p_target->m_x1 - p_target->m_x0 + 1) >> 1;
	centerY = (p_target->m_y1 - p_target->m_y0 + 1) >> 1;
	radius = (p_target->m_x1 - p_target->m_x0 + 1) / 2 - 1;
	radiusY = FixedMul16(radius, aspect);
	clip.m_window = p_target->m_window;
	clip.m_x0 = p_target->m_x0 + p_rect->m_left;
	clip.m_y0 = p_target->m_y0 + p_rect->m_top;
	clip.m_x1 = p_target->m_x0 + p_rect->m_right;
	clip.m_y1 = p_target->m_y0 + p_rect->m_bottom;
	centerX -= p_rect->m_left;
	centerY -= p_rect->m_top;
	VFX_ellipse_fill(&clip, centerX, centerY, radius, radiusY, p_color);
}

// Tests whether a point lies inside the ellipse inscribed in a pane.
int32_t IsInsideGaugeEllipse(PANE* p_target, int32_t p_x, int32_t p_y)
{
	int32_t centerX;
	int32_t centerY;
	int32_t radiusSquared;
	int32_t dx;
	int32_t dy;
	int32_t distanceSquared;
	int32_t aspect;

	aspect = g_eyepoint->m_pixelAspect;
	centerX = (p_target->m_x1 - p_target->m_x0 + 1) >> 1;
	centerY = (p_target->m_y1 - p_target->m_y0 + 1) >> 1;
	radiusSquared = (p_target->m_x1 - p_target->m_x0 + 1) / 2 - 1;
	radiusSquared = radiusSquared * radiusSquared;
	dx = p_x - centerX;
	dy = FixedDiv16(p_y - centerY, aspect);
	distanceSquared = dy * dy + dx * dx;
	return distanceSquared <= radiusSquared;
}

// Where the needle of the gauge ellipse towards p_point ends.
Point* GetEllipseNeedleToward(PANE* p_target, Point* p_point, Point* p_out)
{
	int32_t angle;
	GaugeQuadrant quadrant;
	Point half;
	int32_t dx;
	int32_t dy;

	half.m_x = (p_target->m_x1 - p_target->m_x0 + 1) >> 1;
	half.m_y = (p_target->m_y1 - p_target->m_y0 + 1) >> 1;
	quadrant.m_value = 0;

	dx = half.m_x - p_point->m_x;
	if (dx >= 0) {
		quadrant.m_bits.m_left = 1;
	}

	dy = half.m_y - p_point->m_y;
	if (dy >= 0) {
		quadrant.m_bits.m_up = 1;
	}

	if (dx == 0 || dy / dx > 0x7fff || dy / dx < -0x8000) {
		quadrant.m_bits.m_steep = 1;
	}
	else {
		quadrant.m_bits.m_steep = 0;
	}

	dy = FixedDiv16(dy, g_eyepoint->m_pixelAspect);
	angle = FixedAtan2(dy, dx);
	GetEllipseEdgeAtAngle(p_target, &half, quadrant, angle, p_out);
	return p_out;
}

// Where the needle of the gauge ellipse at the heading p_angle (16.16 degrees) ends.
Point* GetEllipseNeedleAt(PANE* p_target, int32_t p_angle, Point* p_out)
{
	int32_t angle;
	GaugeQuadrant quadrant;
	Point half;
	int32_t dx;
	int32_t dy;

	p_angle = (p_angle % 0x1680000 + 0x1680000) % 0x1680000;
	half.m_x = (p_target->m_x1 - p_target->m_x0 + 1) >> 1;
	half.m_y = (p_target->m_y1 - p_target->m_y0 + 1) >> 1;
	quadrant.m_value = 0;

	if (p_angle >= 0x5a0000 && p_angle <= 0x10e0000) {
		quadrant.m_bits.m_left = 1;
	}
	else {
		quadrant.m_bits.m_left = 0;
	}

	if (p_angle <= 0xb40000) {
		quadrant.m_bits.m_up = 1;
	}
	else {
		quadrant.m_bits.m_up = 0;
	}

	dx = FixedCos(p_angle) >> 13;
	dy = FixedSin(p_angle) >> 13;
	dy = FixedMul16(dy, g_eyepoint->m_pixelAspect);
	if (dx == 0 || dy / dx > 0x7fff || dy / dx < -0x8000) {
		quadrant.m_bits.m_steep = 1;
	}
	else {
		quadrant.m_bits.m_steep = 0;
	}

	angle = FixedAtan2(dy, -dx);
	GetEllipseEdgeAtAngle(p_target, &half, quadrant, angle, p_out);
	return p_out;
}

// Where a needle of the gauge ellipse at p_angle ends, from p_center.
Point* GetEllipseEdgeAtAngle(PANE* p_target, Point* p_center, GaugeQuadrant p_quadrant, int32_t p_angle, Point* p_out)
{
	int32_t radius;
	int32_t radiusY;
	Point result;
	Point center;

	center = *p_center;
	radius = (p_target->m_x1 - p_target->m_x0 + 1) / 2 - 1;
	radiusY = FixedMul16(radius, g_eyepoint->m_pixelAspect);
	radius--;
	radiusY--;

	switch (p_quadrant.m_value) {
	case 0:
	case 1:
	case 2:
	case 3:
		result.m_x = center.m_x - (FixedMul16(radius, FixedCos(p_angle)) >> 13);
		result.m_y = center.m_y - (FixedMul16(radiusY, FixedSin(p_angle)) >> 13);
		break;
	case 7:
		result.m_y = center.m_y - radiusY;
		result.m_x = center.m_x;
		break;
	case 5:
		result.m_y = center.m_y + radiusY;
		result.m_x = center.m_x;
		break;
	default:
		result.m_y = 0;
		result.m_x = 0;
		break;
	}

	*p_out = result;
	return p_out;
}
