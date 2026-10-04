#include "polydraw.h"

#include "animation.h"
#include "bandpoly.h"
#include "collision.h"
#include "decomp.h"
#include "eyepoint.h"
#include "fixeddivu.h"
#include "fixedmul.h"
#include "fixedmul30.h"
#include "fixedtrig.h"
#include "horizon.h"
#include "render.h"
#include "rendersettings.h"
#include "targeting.h"
#include "transform.h"
#include "types.h"
#include "vfx3d.h"
#include "vfxa.h"

// GLOBAL: MW2 0x100a6be0
Eyepoint g_mainEyepoint = {0, 0,   0, 0,   0,    0,       0x10000, 1000, 10000, -1000, 1,  0x48,
						   0, 319, 0, 199, 0x40, 0x249f0, 0,       0,    0,     0,     {0}};

// GLOBAL: MW2 0x100a6cc0
Eyepoint* g_eyepoint = &g_mainEyepoint;

// GLOBAL: MW2 0x100a6cc8
RenderSettings g_renderSettings = {0, 1, 1,       1,       1, 1, 1,    1,    1,    1, {0xe0, 0xef}, 1, 0, 0,
								   0, 0, 0x186a0, 0x10000, 0, 0, NULL, NULL, NULL, 0, NULL};

// GLOBAL: MW2 0x100a6d30
MechS32 g_horizonBandHeight = 0x24;

// Polygon drawing: a polygon is p_count points of 6 dwords each (x, y, a shade, two texture
// coordinates and a depth), and the mode in bits 12 to 14 of p_flags picks how it is drawn.

// The end of the last line DrawLineTo drew.
// GLOBAL: MW2 0x100be5d4
MechS32 g_lineEndX;

// GLOBAL: MW2 0x100be5d8
MechS32 g_lineEndY;

// Stack-slot permutation: luma, saved, mode, index, scale, shade and fraction.
// FUNCTION: MW2 0x10042e00
void DrawScenePolygon(MechS32 p_count, MechU32* p_points, MechU32 p_flags)
{
	MechS32 luma;
	MechS32 saved;
	MechU32 mode;
	MechS32 i;
	MechU32* point;
	MechS32 index;
	MechS32 scale;
	MechS32 shade;
	MechS32 fraction;

	mode = p_flags & 0x7000;
	point = p_points;
	switch (mode) {
	case 0:
	case 0x1000:
		p_flags &= 0xff;
		for (i = 0; i < p_count; i++) {
			point[2] = p_flags << 16;
			point += 6;
		}

		VFX_flat_polygon(&g_currentPane, p_count, p_points);
		break;
	case 0x2000:
		p_flags &= 0xff;
		g_lineEndX = p_points[0];
		g_lineEndY = p_points[1];
		for (i = 1; i < p_count; i++) {
			DrawLineTo(p_points[i * 6], p_points[i * 6 + 1], p_flags);
		}

		DrawLineTo(p_points[0], p_points[1], p_flags);
		break;
	case 0x4000:
		p_flags &= 0xff;
		for (i = 0; i < p_count; i++) {
			if ((MechS32) point[3] < 0x300000) {
				point[2] = point[3];
			}
			else {
				shade = point[3];
				fraction = shade & 0xf0000;
				scale = ((p_flags & 0xf) + 1) << 12;
				fraction = FixedMul16(fraction, scale);
				point[2] = (shade & 0xfff00000) + fraction;
			}

			point += 6;
		}

		if (g_renderSettings.m_gouraud) {
			VFX_dithered_Gouraud_polygon(&g_currentPane, 0x7fff, p_count, p_points);
		}
		else {
			VFX_flat_polygon(&g_currentPane, p_count, p_points);
		}
		break;
	case 0x3000:
		DrawBandPolygon(p_flags, p_count, p_points, -1);
		break;
	case 0x5000:
		if (!g_renderSettings.m_textures) {
			break;
		}

		luma = (p_flags & 0xf00) >> 8;
		index = p_flags & 0xff;
		if (!g_renderSettings.m_affineTextures) {
			for (i = 0; i < p_count; i++) {
				point[5] = FixedDivU16(g_eyepoint->m_projectScaleX, point[5]);
				point[3] = FixedMul30(point[3], point[5]);
				point[4] = FixedMul30(point[4], point[5]);
				point += 6;
			}
		}

		DrawAnimatedPolygon(index, p_count, p_points, luma, 0, 1);
		break;
	case 0x6000:
		if (!g_renderSettings.m_textures) {
			break;
		}

		luma = (p_flags & 0xf00) >> 8;
		index = p_flags & 0xff;
		saved = g_renderSettings.m_affineTextures;
		g_renderSettings.m_affineTextures = TRUE;
		DrawAnimatedPolygon(index, p_count, p_points, luma, 0, 1);
		g_renderSettings.m_affineTextures = saved;
		break;
	case 0x7000:
		if (!g_renderSettings.m_textures) {
			break;
		}

		luma = (p_flags & 0xf00) >> 8;
		index = p_flags & 0xff;
		DrawAnimatedPolygon(index, p_count, p_points, luma, 0, 0);
		break;
	default:
		break;
	}
}

// Draws the sky and the ground of the view from p_eyepoint: the horizon, rolled with the view,
// splits the view rectangle (m_viewLeft-m_viewBottom). Each corner's side of it (IsAboveHorizon) makes
// one bit of the case; the parts are filled in g_skyColor (the sky, with m_drawSky) and
// g_groundColor (the ground, with m_drawGround), and with m_horizonBand a shaded band over the
// horizon blends the sky into the ground.
// Stack-slot permutation; the original calls IsAboveHorizon for the corners in the order of the
// terms, (x0, y1) first (commutative operand order).
// FUNCTION: MW2 0x1004320b
void DrawSkyAndGround(Eyepoint* p_eyepoint)
{
	MechS32 y1;
	MechS32 x1;
	MechS32 yLeft;
	MechS32 corners;
	MechS32 y0;
	MechS32 dx;
	PANE rect;
	MechU32 band[4 * 6];
	MechS32 dy;
	MechS32 dz;
	MechS32 xBottom;
	MechS32 yRight;
	MechS32 xTop;
	MechS32 gradient;
	MechS32 x0;
	Matrix roll;

	x0 = p_eyepoint->m_viewLeft;
	x1 = p_eyepoint->m_viewRight;
	y0 = p_eyepoint->m_viewTop;
	y1 = p_eyepoint->m_viewBottom;
	gradient = FALSE;
	rect = g_currentPane;
	dx = dz = 0;
	dy = g_horizonBandHeight;
	SetIdentityMatrix(&roll);
	roll.m_rows[0][0] = roll.m_rows[1][1] = FixedCos(p_eyepoint->m_roll);
	roll.m_rows[1][0] = FixedSin(p_eyepoint->m_roll);
	roll.m_rows[0][1] = -roll.m_rows[1][0];
	roll.m_rows[3][0] = roll.m_rows[3][1] = roll.m_rows[3][2] = 0;
	TransformPoint(&roll, &dx, &dy, &dz);
	corners = IsAboveHorizon(x0, y1, p_eyepoint) * 4 + IsAboveHorizon(x1, y1, p_eyepoint) * 8 +
			  IsAboveHorizon(x1, y0, p_eyepoint) * 2 + IsAboveHorizon(x0, y0, p_eyepoint);
	band[2] = band[8] = g_skyColor << 16;
	band[14] = band[20] = (g_groundColor - 1) << 16;
	switch (corners) {
	case 0:
		if (g_renderSettings.m_drawGround) {
			rect.m_x0 += x0;
			rect.m_y0 += y0;
			rect.m_x1 = rect.m_x0 + x1 - x0;
			rect.m_y1 = rect.m_y0 + y1 - y0;
			VFX_pane_wipe(&rect, g_groundColor);
		}
		break;
	case 15:
		if (g_renderSettings.m_drawSky) {
			if (g_renderSettings.m_horizonBand && !IsAboveHorizon(x0, dy + y1, p_eyepoint)) {
				yLeft = HorizonYAtX(x0, p_eyepoint);
				yRight = HorizonYAtX(x1, p_eyepoint);
				if (yRight - dy < y1 || yLeft - dy < y1) {
					band[0] = band[18] = x1;
					band[6] = band[12] = x0;
					band[1] = yRight - dy;
					band[7] = yLeft - dy;
					band[13] = yLeft;
					band[19] = yRight;
					gradient = TRUE;
				}
			}

			rect.m_x0 += x0;
			rect.m_y0 += y0;
			rect.m_x1 = rect.m_x0 + x1 - x0;
			rect.m_y1 = rect.m_y0 + y1 - y0;
			VFX_pane_wipe(&rect, g_skyColor);
		}
		break;
	case 3:
		yLeft = HorizonYAtX(x0, p_eyepoint);
		yRight = HorizonYAtX(x1, p_eyepoint);
		if (g_renderSettings.m_horizonBand && g_renderSettings.m_drawSky) {
			band[0] = band[18] = x1;
			band[6] = band[12] = x0;
			band[1] = yRight - dy;
			band[7] = yLeft - dy;
			band[13] = yLeft;
			band[19] = yRight;
			gradient = TRUE;
		}

		if (yRight == yLeft) {
			if (g_renderSettings.m_drawSky) {
				rect.m_x0 += x0;
				rect.m_y0 += y0;
				rect.m_x1 = rect.m_x0 + x1 - x0;
				rect.m_y1 = rect.m_y0 + yLeft - y0;
				VFX_pane_wipe(&rect, g_skyColor);
			}

			if (g_renderSettings.m_drawGround) {
				rect = g_currentPane;
				rect.m_x0 += x0;
				rect.m_y0 += yLeft;
				rect.m_x1 = rect.m_x0 + x1 - x0;
				rect.m_y1 = rect.m_y0 + y1 - yLeft;
				VFX_pane_wipe(&rect, g_groundColor);
			}
		}
		else {
			if (g_renderSettings.m_drawSky) {
				DrawQuad(x1, y0, x0, y0, x0, yLeft, x1, yRight, g_skyColor);
			}

			if (g_renderSettings.m_drawGround) {
				DrawQuad(x0, y1, x1, y1, x1, yRight, x0, yLeft, g_groundColor);
			}
		}
		break;
	case 12:
		yLeft = HorizonYAtX(x0, p_eyepoint);
		yRight = HorizonYAtX(x1, p_eyepoint);
		if (g_renderSettings.m_horizonBand && g_renderSettings.m_drawSky) {
			band[12] = band[6] = x1 - dx;
			band[18] = band[0] = dx + x0;
			band[13] = yRight;
			band[19] = yLeft;
			band[1] = yLeft - dy;
			band[7] = yRight - dy;
			gradient = TRUE;
		}

		if (yRight == yLeft) {
			if (g_renderSettings.m_drawGround) {
				rect.m_x0 += x0;
				rect.m_y0 += y0;
				rect.m_x1 = rect.m_x0 + x1 - x0;
				rect.m_y1 = rect.m_y0 + yLeft - y0;
				VFX_pane_wipe(&rect, g_groundColor);
			}

			if (g_renderSettings.m_drawSky) {
				rect = g_currentPane;
				rect.m_x0 += x0;
				rect.m_y0 += yLeft;
				rect.m_x1 = rect.m_x0 + x1 - x0;
				rect.m_y1 = rect.m_y0 + y1 - yLeft;
				VFX_pane_wipe(&rect, g_skyColor);
			}
		}
		else {
			if (g_renderSettings.m_drawGround) {
				DrawQuad(x1, y0, x0, y0, x0, yLeft, x1, yRight, g_groundColor);
			}

			if (g_renderSettings.m_drawSky) {
				DrawQuad(x0, y1, x1, y1, x1, yRight, x0, yLeft, g_skyColor);
			}
		}
		break;
	case 5:
		xTop = HorizonXAtY(y0, p_eyepoint);
		xBottom = HorizonXAtY(y1, p_eyepoint);
		if (xBottom == xTop) {
			if (g_renderSettings.m_drawSky) {
				rect.m_x0 += x0;
				rect.m_y0 += y0;
				rect.m_x1 = rect.m_x0 + xTop - x0 - 1;
				rect.m_y1 = rect.m_y0 + y1 - y0 - 1;
				VFX_pane_wipe(&rect, g_skyColor);
			}

			if (g_renderSettings.m_drawGround) {
				rect = g_currentPane;
				rect.m_x0 += xTop - x0 + 1;
				rect.m_y0 += y0;
				rect.m_x1 = rect.m_x0 + x1 - xTop - 1;
				rect.m_y1 = rect.m_y0 + y1 - y0 - 1;
				VFX_pane_wipe(&rect, g_groundColor);
			}
		}
		else {
			if (g_renderSettings.m_drawSky) {
				DrawQuad(x0, y0, x0, y1, xBottom, y1, xTop, y0, g_skyColor);
			}

			if (g_renderSettings.m_drawGround) {
				DrawQuad(x1, y1, x1, y0, xTop, y0, xBottom, y1, g_groundColor);
			}
		}

		if (g_renderSettings.m_horizonBand && g_renderSettings.m_drawSky) {
			band[0] = xTop - dx;
			band[6] = xBottom - dx;
			band[12] = xBottom;
			band[18] = xTop;
			band[1] = y0;
			band[19] = y0;
			band[7] = y1;
			band[13] = y1;
			gradient = TRUE;
		}
		break;
	case 10:
		xTop = HorizonXAtY(y0, p_eyepoint);
		xBottom = HorizonXAtY(y1, p_eyepoint);
		if (xBottom == xTop) {
			if (g_renderSettings.m_drawGround) {
				rect.m_x0 += x0;
				rect.m_y0 += y0;
				rect.m_x1 = rect.m_x0 + xTop - x0 - 1;
				rect.m_y1 = rect.m_y0 + y1 - y0 - 1;
				VFX_pane_wipe(&rect, g_groundColor);
			}

			if (g_renderSettings.m_drawSky) {
				rect = g_currentPane;
				rect.m_x0 += xTop - x0 + 1;
				rect.m_y0 += y0;
				rect.m_x1 = rect.m_x0 + x1 - xTop - 1;
				rect.m_y1 = rect.m_y0 + y1 - y0 - 1;
				VFX_pane_wipe(&rect, g_skyColor);
			}
		}
		else {
			if (g_renderSettings.m_drawGround) {
				DrawQuad(x0, y0, x0, y1, xBottom, y1, xTop, y0, g_groundColor);
			}

			if (g_renderSettings.m_drawSky) {
				DrawQuad(x1, y1, x1, y0, xTop, y0, xBottom, y1, g_skyColor);
			}
		}

		if (g_renderSettings.m_horizonBand && g_renderSettings.m_drawSky) {
			band[0] = xTop - dx;
			band[6] = xBottom - dx;
			band[12] = xBottom;
			band[18] = xTop;
			band[1] = y0;
			band[19] = y0;
			band[7] = y1;
			band[13] = y1;
			gradient = TRUE;
		}
		break;
	case 1:
		xTop = HorizonXAtY(y0, p_eyepoint);
		yLeft = HorizonYAtX(x0, p_eyepoint);
		if (g_renderSettings.m_drawSky) {
			DrawTriangle(x0, y0, x0, yLeft, xTop, y0, g_skyColor);
		}

		if (g_renderSettings.m_drawGround) {
			DrawPentagon(x0, y1, x1, y1, x1, y0, xTop, y0, x0, yLeft, g_groundColor);
		}
		break;
	case 14:
		xTop = HorizonXAtY(y0, p_eyepoint);
		yLeft = HorizonYAtX(x0, p_eyepoint);
		if (g_renderSettings.m_drawGround) {
			DrawTriangle(x0, y0, x0, yLeft, xTop, y0, g_groundColor);
		}

		if (g_renderSettings.m_drawSky) {
			DrawPentagon(x0, y1, x1, y1, x1, y0, xTop, y0, x0, yLeft, g_skyColor);
		}
		break;
	case 2:
		xTop = HorizonXAtY(y0, p_eyepoint);
		yRight = HorizonYAtX(x1, p_eyepoint);
		if (g_renderSettings.m_drawSky) {
			DrawTriangle(x1, y0, xTop, y0, x1, yRight, g_skyColor);
		}

		if (g_renderSettings.m_drawGround) {
			DrawPentagon(x0, y0, x0, y1, x1, y1, x1, yRight, xTop, y0, g_groundColor);
		}
		break;
	case 13:
		xTop = HorizonXAtY(y0, p_eyepoint);
		yRight = HorizonYAtX(x1, p_eyepoint);
		if (g_renderSettings.m_drawGround) {
			DrawTriangle(x1, y0, xTop, y0, x1, yRight, g_groundColor);
		}

		if (g_renderSettings.m_drawSky) {
			DrawPentagon(x0, y0, x0, y1, x1, y1, x1, yRight, xTop, y0, g_skyColor);
		}
		break;
	case 4:
		xBottom = HorizonXAtY(y1, p_eyepoint);
		yLeft = HorizonYAtX(x0, p_eyepoint);
		if (g_renderSettings.m_drawSky) {
			DrawTriangle(x0, yLeft, x0, y1, xBottom, y1, g_skyColor);
		}

		if (g_renderSettings.m_drawGround) {
			DrawPentagon(x1, y1, x1, y0, x0, y0, x0, yLeft, xBottom, y1, g_groundColor);
		}
		break;
	case 11:
		xBottom = HorizonXAtY(y1, p_eyepoint);
		yLeft = HorizonYAtX(x0, p_eyepoint);
		yRight = HorizonYAtX(x1, p_eyepoint);
		if (g_renderSettings.m_drawGround) {
			DrawTriangle(x0, yLeft, x0, y1, xBottom, y1, g_groundColor);
		}

		if (g_renderSettings.m_drawSky) {
			DrawPentagon(x1, y1, x1, y0, x0, y0, x0, yLeft, xBottom, y1, g_skyColor);
			if (g_renderSettings.m_horizonBand) {
				band[6] = band[12] = x0;
				band[18] = band[0] = x1;
				band[1] = yRight - dy;
				band[7] = yLeft - dy;
				band[13] = yLeft;
				band[19] = yRight;
				gradient = TRUE;
			}
		}
		break;
	case 8:
		xBottom = HorizonXAtY(y1, p_eyepoint);
		yRight = HorizonYAtX(x1, p_eyepoint);
		if (g_renderSettings.m_drawSky) {
			DrawTriangle(xBottom, y1, x1, y1, x1, yRight, g_skyColor);
		}

		if (g_renderSettings.m_drawGround) {
			DrawPentagon(x1, y0, x0, y0, x0, y1, xBottom, y1, x1, yRight, g_groundColor);
		}
		break;
	case 7:
		xBottom = HorizonXAtY(y1, p_eyepoint);
		yRight = HorizonYAtX(x1, p_eyepoint);
		yLeft = HorizonYAtX(x0, p_eyepoint);
		if (g_renderSettings.m_drawGround) {
			DrawTriangle(xBottom, y1, x1, y1, x1, yRight, g_groundColor);
		}

		if (g_renderSettings.m_drawSky) {
			DrawPentagon(x1, y0, x0, y0, x0, y1, xBottom, y1, x1, yRight, g_skyColor);
			if (g_renderSettings.m_horizonBand) {
				band[6] = band[12] = x0;
				band[18] = band[0] = x1;
				band[1] = yRight - dy;
				band[7] = yLeft - dy;
				band[13] = yLeft;
				band[19] = yRight;
				gradient = TRUE;
			}
		}
		break;
	}

	if (gradient && g_renderSettings.m_gouraud) {
		VFX_dithered_Gouraud_polygon(&g_currentPane, 0x8000, 4, band);
	}
}

// Draws a closed polygon of five points.
// FUNCTION: MW2 0x1004440d
void DrawPentagon(
	MechS32 p_x0,
	MechS32 p_y0,
	MechS32 p_x1,
	MechS32 p_y1,
	MechS32 p_x2,
	MechS32 p_y2,
	MechS32 p_x3,
	MechS32 p_y3,
	MechS32 p_x4,
	MechS32 p_y4,
	MechU32 p_flags
)
{
	MechU32 points[5 * 6];

	points[0] = points[3] = p_x0;
	points[1] = points[4] = p_y0;
	points[6] = points[9] = p_x1;
	points[7] = points[10] = p_y1;
	points[12] = points[15] = p_x2;
	points[13] = points[16] = p_y2;
	points[18] = points[21] = p_x3;
	points[19] = points[22] = p_y3;
	points[24] = points[27] = p_x4;
	points[25] = points[28] = p_y4;
	g_renderSettings.m_drawPolygon(5, points, p_flags);
}

// FUNCTION: MW2 0x100444a6
void DrawQuad(
	MechS32 p_x0,
	MechS32 p_y0,
	MechS32 p_x1,
	MechS32 p_y1,
	MechS32 p_x2,
	MechS32 p_y2,
	MechS32 p_x3,
	MechS32 p_y3,
	MechU32 p_flags
)
{
	MechU32 points[4 * 6];

	points[0] = points[3] = p_x0;
	points[1] = points[4] = p_y0;
	points[6] = points[9] = p_x1;
	points[7] = points[10] = p_y1;
	points[12] = points[15] = p_x2;
	points[13] = points[16] = p_y2;
	points[18] = points[21] = p_x3;
	points[19] = points[22] = p_y3;
	g_renderSettings.m_drawPolygon(4, points, p_flags);
}

// FUNCTION: MW2 0x10044527
void DrawTriangle(MechS32 p_x0, MechS32 p_y0, MechS32 p_x1, MechS32 p_y1, MechS32 p_x2, MechS32 p_y2, MechU32 p_flags)
{
	MechU32 points[3 * 6];

	points[0] = points[3] = p_x0;
	points[1] = points[4] = p_y0;
	points[6] = points[9] = p_x1;
	points[7] = points[10] = p_y1;
	points[12] = points[15] = p_x2;
	points[13] = points[16] = p_y2;
	g_renderSettings.m_drawPolygon(3, points, p_flags);
}

// Draws a line from the end of the last one to (p_x, p_y).
// FUNCTION: MW2 0x10044590
void DrawLineTo(MechS32 p_x, MechS32 p_y, MechU32 p_color)
{
	VFX_line_draw(&g_currentPane, p_x, p_y, g_lineEndX, g_lineEndY, 0, p_color);
	g_lineEndX = p_x;
	g_lineEndY = p_y;
}

// Draws a polygon: a point or a line with the pane's own routines where the settings
// allow, else through g_renderSettings's polygon callback, filled, outlined or both.
// FUNCTION: MW2 0x100445d2
void DrawPolygonOrLine(MechS32 p_count, MechU32* p_points, MechU32 p_flags)
{
	if (p_count == 1 && g_renderSettings.m_drawPixels) {
		if (p_flags == 0x1000) {
			return;
		}

		VFX_pixel_write(&g_currentPane, p_points[0], p_points[1], p_flags);
	}
	else if (p_count == 2 && g_renderSettings.m_drawLines) {
		VFX_line_draw(&g_currentPane, p_points[0], p_points[1], p_points[6], p_points[7], 0, p_flags);
	}
	else if (g_renderSettings.m_wireframe == 0) {
		g_renderSettings.m_drawPolygon(p_count, p_points, p_flags);
	}
	else if (g_renderSettings.m_wireframe == 1) {
		g_renderSettings.m_drawPolygon(p_count, p_points, 0);
		g_renderSettings.m_drawPolygon(p_count, p_points, p_flags | 0x2000);
	}
	else {
		g_renderSettings.m_drawPolygon(p_count, p_points, p_flags | 0x2000);
	}
}
