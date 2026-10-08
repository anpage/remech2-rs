#include "supanim.h"

#include "debugprint.h"
#include "decomp.h"
#include "displaybackend.h"
#include "files.h"
#include "network.h"
#include "palettecolor.h"
#include "refreshmode.h"
#include "render.h"
#include "simmain.h"
#include "targeting.h"
#include "types.h"
#include "vfxa.h"
#include "window.h"

#include <stdio.h>

// The dropship of the loading screen ("sup anim"). The original animated it from an AIL timer's
// thread while the mission loaded; for now it's one frame.

// GLOBAL: MW2 0x100a0154
void* g_supAnimBackdrop = NULL;

// GLOBAL: MW2 0x100a0158
void* g_supAnimShape = NULL;

// GLOBAL: MW2 0x100a015c
MechS32 g_supAnimFrameCount = 0;

// The command line's overrides of the backdrop and dropship file names (ProcessCmdLineArgs).

// GLOBAL: MW2 0x100a0160
MechChar* g_supAnimBackdropName = NULL;

// GLOBAL: MW2 0x100a0164
MechChar* g_supAnimShapeName = NULL;

// The first 16 palette entries in single player.
// GLOBAL: MW2 0x100a0168
PaletteColor g_supAnimPalette[16] = {
	{0x00, 0x00, 0x00},
	{0x33, 0x34, 0x36},
	{0x27, 0x28, 0x2b},
	{0x20, 0x20, 0x23},
	{0x1b, 0x1c, 0x1e},
	{0x17, 0x19, 0x1b},
	{0x15, 0x15, 0x17},
	{0x12, 0x14, 0x16},
	{0x11, 0x12, 0x14},
	{0x10, 0x10, 0x12},
	{0x0e, 0x10, 0x12},
	{0x0d, 0x0e, 0x10},
	{0x0c, 0x0c, 0x0e},
	{0x09, 0x09, 0x0a},
	{0x04, 0x04, 0x05},
	{0x01, 0x01, 0x02},
};

// GLOBAL: MW2 0x100bcd50
MechS32 g_supAnimY;

// GLOBAL: MW2 0x100bcd54
MechS32 g_supAnimX;

// GLOBAL: MW2 0x100bcd58
PANE g_supAnimTarget;

// GLOBAL: MW2 0x100bcd70
WINDOW g_supAnimBuffer;

// Starts the dropship loading screen: loads the backdrop (launch\\supanm6.shp, netmech6.shp
// in a network game, or the command line's override), draws it and fades its palette in (slowly with p_slowFade), then
// loads the dropship (launch6.shp) and draws its first frame. Stack-slot permutation: backdropPath, shapePath and
// palette. FUNCTION: MW2 0x10003a70
void StartSupAnim(MechS32 p_slowFade)
{
	MechChar backdropPath[256];
	MechChar shapePath[256];
	PaletteColor* palette;

	if (g_supAnimBackdropName == NULL || *g_supAnimBackdropName == '\0') {
		sprintf(backdropPath, "%s\\%s6.%s", "launch", !g_isNetworkGame ? "supanm" : "netmech", "shp");
	}
	else {
		sprintf(backdropPath, "%s\\%s6.%s", "launch", !g_isNetworkGame ? g_supAnimBackdropName : "netmech", "shp");
	}

	if (g_supAnimShapeName == NULL || *g_supAnimShapeName == '\0') {
		sprintf(shapePath, "%s\\%s6.%s", "launch", "launch", "shp");
	}
	else {
		sprintf(shapePath, "%s\\%s6.%s", "launch", g_supAnimShapeName, "shp");
	}

	g_supAnimBackdrop = MechReadFile(g_primaryHeap, backdropPath);
	if (g_supAnimBackdrop == NULL) {
		return;
	}

	palette = MechHeapAllocZeroed(g_primaryHeap, 0x100 * sizeof(PaletteColor));
	if (palette == NULL) {
		return;
	}

	g_currentDisplayBackend->m_setPalette(0, 0x100, palette, 1);
	g_supAnimBuffer = g_mainPixelBuffer;
	g_supAnimBuffer.m_xMax = g_refreshModeWidth - 1;
	g_supAnimBuffer.m_yMax = g_refreshModeHeight - 1;
	g_supAnimTarget.m_window = &g_supAnimBuffer;
	g_supAnimTarget.m_x1 = g_refreshModeWidth - 1;
	g_supAnimTarget.m_y1 = g_refreshModeHeight - 1;
	if ((g_windowActive ? g_currentDisplayBackend->m_acquireFramebuffer() : -1) != 0) {
		DebugPrint("StartSupanim: LockDisplayBuffer failed.\n");
		return;
	}

	g_supAnimBuffer.m_buffer = g_mainPixelBuffer.m_buffer;
	VFX_pane_wipe(&g_supAnimTarget, 0);
	VFX_shape_draw(&g_supAnimTarget, g_supAnimBackdrop, 0, 0, 0);
	if (g_windowActive) {
		g_currentRefreshMode->m_flip();
	}

	VFX_shape_palette(g_supAnimBackdrop, 0, (MechU8*) palette);
	if (p_slowFade) {
		g_currentDisplayBackend->m_blendPalettes(palette, 30);
	}
	else {
		g_currentDisplayBackend->m_blendPalettes(palette, 2);
	}

	if (!g_isNetworkGame) {
		g_currentDisplayBackend->m_setPalette(0, 16, g_supAnimPalette, 1);
	}

	MechHeapFree(g_primaryHeap, palette);
	g_supAnimShape = MechReadFile(g_primaryHeap, shapePath);
	if (g_supAnimShape == NULL) {
		return;
	}

	g_supAnimFrameCount = VFX_shape_count(g_supAnimShape);
	g_supAnimX = (g_supAnimTarget.m_x1 - g_supAnimTarget.m_x0) * 0.55;
	g_supAnimY = 0;
	if (!g_isNetworkGame) {
		if ((g_windowActive ? g_currentDisplayBackend->m_acquireFramebuffer() : -1) == 0) {
			VFX_shape_draw(&g_supAnimTarget, g_supAnimBackdrop, 0, 0, 0);
			VFX_shape_draw(&g_supAnimTarget, g_supAnimShape, 0, g_supAnimX, g_supAnimY);
			if (g_windowActive) {
				g_currentRefreshMode->m_flip();
			}
		}
	}
}

// Frees the backdrop and the dropship.
// FUNCTION: MW2 0x10004031
void StopSupAnim(void)
{
	if (g_supAnimBackdrop) {
		MechHeapFree(g_primaryHeap, g_supAnimBackdrop);
		g_supAnimBackdrop = NULL;
	}

	if (g_supAnimShape) {
		MechHeapFree(g_primaryHeap, g_supAnimShape);
		g_supAnimShape = NULL;
		g_supAnimFrameCount = 0;
	}
}
