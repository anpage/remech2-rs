#include "setres.h"

#include "cockpit.h"
#include "config.h"
#include "decomp.h"
#include "eyepoint.h"
#include "hud.h"
#include "muldiv.h"
#include "overlay.h"
#include "palette.h"
#include "point.h"
#include "polydraw.h"
#include "render.h"
#include "screenscale.h"
#include "targeting.h"
#include "timedoverlays.h"
#include "types.h"
#include "view.h"

#include <stdlib.h>

// The file name suffixes of the art resolutions (ShowBanner).
// GLOBAL: MW2 0x100aa710
MechChar g_artResolutionSuffixes[4][2] = {"", "6", "k", ""};

// The largest coordinates of the three resolutions the art comes in.
// GLOBAL: MW2 0x100aa718
Point g_artResolutionSizes[3] = {{319, 199}, {639, 479}, {1023, 767}};

// GLOBAL: MW2 0x100e9610
MechS32 g_pixelAspect;

// GLOBAL: MW2 0x100e9614
MechS32 g_artResolution;

// FUNCTION: MW2 0x1005d410
void SetPixelAspect(GameWindowGeometry* p_geometry)
{
	g_eyepoint->m_pixelAspect = g_pixelAspect =
		MulDiv64(p_geometry->m_height << 16, 0x15555, p_geometry->m_width << 16);
}

// ChooseArtResolution is implemented on the Rust side (src/sim/window.rs). The original only
// took a resolution within 7 of the window's size, which 320x240 never is.

// Rescales the tables authored in 320x200 coordinates to the screen, and sets up what depends on
// the resolution.
// FUNCTION: MW2 0x1005d4d3
void SetRes(void)
{
	MechS32 i;
	Point point;

	for (i = 0; i < 8; i++) {
		ScaleRectFromLowRes(&g_panes[i], &g_panes[i]);
		ScaleRectToScreen(&g_mainPixelBuffer, &g_panes[i], &g_panes[i]);
	}

	for (i = 0; i < 5; i++) {
		ScaleRectFromLowRes(&g_cockpitGaugePanes[i], &g_cockpitGaugePanes[i]);
		ScaleRectToScreen(&g_mainPixelBuffer, &g_cockpitGaugePanes[i], &g_cockpitGaugePanes[i]);
	}

	ScalePointFromLowRes(g_unk0x100a5bb8[3], g_unk0x100a5bb8[3]);
	ScalePointToScreen(&g_mainPixelBuffer, g_unk0x100a5bb8[3], g_unk0x100a5bb8[3]);
	for (i = 0; i < 6; i++) {
		ScalePointFromLowRes(&g_hudGaugePositions[i], &g_hudGaugePositions[i]);
		ScalePointToScreen(&g_mainPixelBuffer, &g_hudGaugePositions[i], &g_hudGaugePositions[i]);
	}

	SelectPane(0);
	UpdateProjection(g_eyepoint);
	UpdateViewMatrix(g_eyepoint);
	g_projectionDirty = 0;
	point.m_x = g_horizonBandHeight;
	point.m_y = 0;
	ScalePointFromLowRes(&point, &point);
	ScalePointToScreen(&g_mainPixelBuffer, &point, &point);
	g_horizonBandHeight = point.m_x;
	ScaleCockpitLayout();
	LayoutMessageBoxes();
	ScaleOverlayPositions();
}
