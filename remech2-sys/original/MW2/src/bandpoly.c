#include "bandpoly.h"

#include "animation.h"
#include "decomp.h"
#include "palette.h"
#include "polydraw.h"
#include "types.h"

#include <stdlib.h>

// The texture coordinates of a band's corners (DrawBand).
Point g_bandTexCoords[4] = {{0, 0}, {0x10000, 0}, {0x10000, 0x10000}, {0, 0x10000}};

// Draws a polygon of p_count points in the mode in p_flags' low four bits, in the color in bits
// 4-11: modes 0 and 3 draw it filled (DrawBand) when the render settings allow.
void DrawBandPolygon(uint32_t p_flags, int32_t p_count, uint32_t* p_points, int32_t p_unk0x0c)
{
	uint32_t color;
	uint32_t mode;

	mode = p_flags & 0xf;
	color = (p_flags & 0xff0) >> 4;
	switch (mode) {
	case 0:
	case 3:
		if (g_renderSettings.m_flags & 1) {
			DrawBand(color, p_count, (int32_t*) p_points, p_unk0x0c, 0);
		}
		break;
	case 1:
		break;
	default:
		break;
	}
}

// Draws a band as a textured quad (DrawAnimatedPolygon) from the first three points of a polygon: the
// two whose m_unk0x0c/m_unk0x10 mark its ends (mode 0), or a square about the second one (mode
// 1). A negative mark flips the texture. Returns half the band's width, or 0 without both ends.
int32_t DrawBand(uint32_t p_color, int32_t p_count, int32_t* p_points, int32_t p_luma, int32_t p_mode)
{
	int32_t x0;
	int32_t y0;
	int32_t x1;
	int32_t step;
	int32_t flip;
	int32_t i;
	int32_t y1;
	int32_t haveStart;
	int32_t dy;
	int32_t dx;
	int32_t haveEnd;
	uint32_t quad[4][6];

	haveStart = FALSE;
	haveEnd = FALSE;
	flip = FALSE;
	if (p_count < 2) {
		return 0;
	}

	for (i = 0; i < 3; i++) {
		if (p_points[3] == 0) {
			if (p_points[4]) {
				x1 = p_points[0];
				y1 = p_points[1];
				haveEnd = TRUE;
				if (p_points[4] < 0) {
					flip = TRUE;
				}
			}
			else if (p_mode == 0) {
				x0 = p_points[0];
				y0 = p_points[1];
				haveStart = TRUE;
			}
		}
		else {
			if (p_mode == 1) {
				x0 = p_points[0];
				y0 = p_points[1];
				haveStart = TRUE;
			}

			if (p_points[3] < 0) {
				flip = TRUE;
			}
		}

		p_points += 6;
	}

	if (!haveStart || !haveEnd) {
		return 0;
	}

	dx = (y1 - y0) >> 1;
	dy = (x1 - x0) >> 1;
	if (p_mode == 0) {
		quad[0][0] = x0 - dx;
		quad[0][1] = y0 + dy;
		quad[1][0] = x0 + dx;
		quad[1][1] = y0 - dy;
		quad[2][0] = x1 + dx;
		quad[2][1] = y1 - dy;
		quad[3][0] = x1 - dx;
		quad[3][1] = y1 + dy;
	}
	else {
		dy = abs(dy);
		quad[0][0] = x1 - dy;
		quad[0][1] = y1 - dy;
		quad[1][0] = x1 + dy;
		quad[1][1] = y1 - dy;
		quad[2][0] = x1 + dy;
		quad[2][1] = y1 + dy;
		quad[3][0] = x1 - dy;
		quad[3][1] = y1 + dy;
	}

	for (i = 0; i < 4; i++) {
		quad[i][4] = g_bandTexCoords[i].m_y;
		if (flip) {
			if (i % 2) {
				step = -1;
			}
			else {
				step = 1;
			}

			quad[i][3] = g_bandTexCoords[step + i].m_x;
		}
		else {
			quad[i][3] = g_bandTexCoords[i].m_x;
		}
	}

	DrawAnimatedPolygon(p_color, 4, quad[0], p_luma, 1, 0);
	return dx;
}

int32_t DrawBandWithFlash(undefined4 p_unk0x00, int32_t p_unk0x04, undefined4* p_unk0x08)
{
	int32_t result;

	result = DrawBand(p_unk0x00, p_unk0x04, (int32_t*) p_unk0x08, 15, 0);
	if (result > 60) {
		StartPaletteFade(17, 181, 1);
	}

	return result;
}
