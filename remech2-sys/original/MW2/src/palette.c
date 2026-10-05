#include "palette.h"

#include "clock.h"
#include "decomp.h"
#include "fixeddiv.h"
#include "loadres.h"
#include "mw2prj.h"
#include "palcycle.h"
#include "palfade.h"
#include "polydraw.h"
#include "refreshmode.h"
#include "render.h"
#include "simmain.h"
#include "targeting.h"
#include "ticks.h"
#include "types.h"

#include <string.h>

// GLOBAL: MW2 0x100a00cc
MechS32 g_paneIndex = -1;

// GLOBAL: MW2 0x100a00d0
MechS32 g_currentPalette = 0x10;

// GLOBAL: MW2 0x100a00d4
MechS32 g_palettePending = 0;

// GLOBAL: MW2 0x100a00d8
MechS32 g_basePalette = 0;

// GLOBAL: MW2 0x100a00dc
MechS32 g_settledPalette = 0x10;

// GLOBAL: MW2 0x100a00e0
MechS32 g_paletteFadeTarget = -1;

// GLOBAL: MW2 0x100a00e4
MechS32 g_paletteFadeBack = -1;

// GLOBAL: MW2 0x100a00e8
MechS32 g_paletteFadeBackSteps = 0;

// GLOBAL: MW2 0x100a00ec
MechS32 g_paletteFadeSteps = 0;

// GLOBAL: MW2 0x100a00f0
MechS32 g_paletteCycling = 0;

// GLOBAL: MW2 0x100a00f4
MechS32 g_paletteCycleResource = -1;

// GLOBAL: MW2 0x100bcd20
PaletteFade g_paletteFade;

// GLOBAL: MW2 0x100bcd40
PaletteCycle g_paletteCycle;

// GLOBAL: MW2 0x10181a60
PANE g_panes[11];

// GLOBAL: MW2 0x10181b40
MechS32 g_paletteResourceIds[20];

// FUNCTION: MW2 0x100023c0
void InitPanes(PANE* p_target)
{
	MechS32 i;

	for (i = 0; i < 11; i++) {
		g_panes[i] = *p_target;
	}

	for (i = 0; i < 20; i++) {
		g_paletteResourceIds[i] = 0;
	}
}

// SelectPane is implemented on the Rust side (src/sim/window.rs). In widescreen the 3D views
// cover the whole frame.

// FUNCTION: MW2 0x100024f0
void GetViewCenter(Eyepoint* p_eyepoint, MechS32* p_x, MechS32* p_y)
{
	MechS32 x;
	MechS32 y;

	x = p_eyepoint->m_offsetX + (p_eyepoint->m_viewLeft + p_eyepoint->m_viewRight) / 2;
	y = p_eyepoint->m_offsetY + (p_eyepoint->m_viewTop + p_eyepoint->m_viewBottom) / 2;
	*p_x = x;
	*p_y = y;
}

// FUNCTION: MW2 0x10002546
void ApplyPendingPalette(void)
{
	if (g_palettePending && g_paletteFadeSteps <= 0) {
		g_palettePending = 0;
		ApplyPaletteResource(g_settledPalette);
		g_currentPalette = g_settledPalette;
	}
}

// FUNCTION: MW2 0x1000258d
void ApplyPaletteResource(MechS32 p_slot)
{
	MechU8* palette;
	MechS32* id;

	id = &g_paletteResourceIds[p_slot];
	if (*id > 0) {
		palette = LoadCachedResource(g_mw2PrjHandle, *id, g_resourceTypeTags[c_resTagPal], 0);
		if (palette) {
			g_currentDisplayBackend->m_setPaletteWithBrightness((PaletteColor*) palette);
			UnlockCachedResource(*id, g_resourceTypeTags[c_resTagPal]);
		}
	}
}

// FUNCTION: MW2 0x10002600
void UpdatePaletteFade(void)
{
	if (g_paletteFadeSteps) {
		StepPaletteFade(&g_paletteFade);
		if (--g_paletteFadeSteps == 0) {
			g_currentPalette = g_paletteFadeTarget;
			if (g_paletteFadeBackSteps) {
				StartPaletteFade(g_paletteFadeBack, g_paletteFadeBackSteps, 0);
			}
			else {
				g_palettePending = 1;
			}
		}
	}

	if (g_paletteCycling) {
		RotatePaletteCycle(&g_paletteCycle);
	}
}

// Matches except for the stack slots of fromSlot, steps and to (a consistent permutation).
// FUNCTION: MW2 0x10002687
MechS32 StartPaletteFade(MechS32 p_palette, MechS32 p_duration, MechS32 p_mode)
{
	MechU8* to;
	MechU8* from;
	MechS32 steps;
	MechS32 fromSlot;
	MechS32 result;

	from = NULL;
	fromSlot = -1;
	result = 0;
	if (g_paletteFadeSteps > 0 && p_mode == 0) {
		from = g_paletteFade.m_palette;
	}
	else if (g_paletteFadeSteps <= 0) {
		fromSlot = g_currentPalette;
		from = LoadCachedResource(g_mw2PrjHandle, g_paletteResourceIds[fromSlot], g_resourceTypeTags[c_resTagPal], 0);
	}

	if (from) {
		to = LoadCachedResource(g_mw2PrjHandle, g_paletteResourceIds[p_palette], g_resourceTypeTags[c_resTagPal], 0);
		if (to) {
			g_paletteFadeTarget = p_palette;
			g_paletteFadeBack = fromSlot;
			if (g_deltaTime) {
				if (p_mode == 1) {
					p_duration >>= 1;
				}

				if (g_deltaTime > 0) {
					steps = FixedDiv16(p_duration, g_deltaTime);
				}
				else {
					steps = 10;
				}

				if (steps >= 0x10000) {
					steps >>= 16;
				}
				else {
					steps = 1;
				}
			}
			else {
				steps = 20;
			}

			if (p_mode == 2) {
				g_paletteFadeSteps = 1;
			}
			else {
				g_paletteFadeSteps = steps;
			}

			if (p_mode == 1 || p_mode == 2) {
				g_paletteFadeBackSteps = steps;
			}
			else {
				g_paletteFadeBackSteps = 0;
			}

			InitPaletteFade(&g_paletteFade, from, to, 0, 0x100, steps);
			result = 1;
			UnlockCachedResource(g_paletteResourceIds[p_palette], g_resourceTypeTags[c_resTagPal]);
		}

		if (fromSlot == -1) {
			MechHeapFree(g_primaryHeap, from);
		}
		else {
			UnlockCachedResource(g_paletteResourceIds[fromSlot], g_resourceTypeTags[c_resTagPal]);
		}
	}

	return result;
}

// FUNCTION: MW2 0x1000288e
MechS32 StartPaletteFlash(MechS32 p_offset, MechS32 p_duration, MechS32 p_mode)
{
	return StartPaletteFade(p_offset + g_basePalette, p_duration, p_mode);
}

// FUNCTION: MW2 0x100028b7
void StartPaletteCycle(MechU8 p_first, MechS32 p_count)
{
	MechU8* palette;

	if (g_paletteFadeSteps > 0 || g_paletteCycling) {
		return;
	}

	g_paletteCycling = 1;
	g_paletteCycleResource = g_paletteResourceIds[g_currentPalette];
	palette = LoadCachedResource(g_mw2PrjHandle, g_paletteCycleResource, g_resourceTypeTags[c_resTagPal], 0);
	if (palette == NULL) {
		return;
	}

	InitPaletteCycle(&g_paletteCycle, palette, p_first, p_count);
	memcpy(g_paletteCycle.m_working, palette, 0x300);
	g_paletteCycle.m_first = 0x80;
	g_paletteCycle.m_count = 0x10;
	g_paletteCycling = 1;
}

// FUNCTION: MW2 0x10002971
void StopPaletteCycle(void)
{
	if (!g_paletteCycling) {
		return;
	}

	FreePaletteCycle(&g_paletteCycle);
	g_paletteCycling = 0;
	UnlockCachedResource(g_paletteCycleResource, g_resourceTypeTags[c_resTagPal]);
	g_paletteCycleResource = -1;
	ApplyPendingPalette();
}

// FUNCTION: MW2 0x100029c8
MechS32 SetPaletteResourceId(MechS32 p_id, MechS32 p_slot)
{
	MechS32 old;

	old = g_paletteResourceIds[p_slot];
	g_paletteResourceIds[p_slot] = p_id;
	LoadCachedResource(g_mw2PrjHandle, p_id, g_resourceTypeTags[c_resTagPal], 0);
	UnlockCachedResource(p_id, g_resourceTypeTags[c_resTagPal]);
	return old;
}

// FUNCTION: MW2 0x10002a24
void FadeToBasePalette(MechS32 p_palette, MechS32 p_duration)
{
	MechS32 palette;

	palette = p_palette;
	StartPaletteFade(palette, p_duration, 0);
	g_basePalette = p_palette;
	g_settledPalette = palette;
}

// FUNCTION: MW2 0x10002a5a
void SetBasePalette(MechS32 p_palette)
{
	g_basePalette = p_palette;
	g_settledPalette = p_palette;
}

// Brings up the start palette (slot 0x10): with p_dissolve, it dissolves the screen to black
// over 0x10f ticks first. It passes the result of the slot test, not the slot's resource id,
// to the loader, so it always loads PAL resource 1 (SimMain has the same test).
// Matches except for the stack slots of all its locals (a consistent permutation).
// FUNCTION: MW2 0x10002a75
void StartPalettes(MechS32 p_dissolve)
{
	MechU8* palette;
	MechS32 last;
	MechS32 handle;
	WINDOW buffer;
	MechS32 hasPalette;
	MechS32 seed;
	PANE* src;
	PANE target;
	MechS32 count;
	void* pixels;
	MechS32 ticks;

	seed = 0;
	last = -1;
	ticks = 0;
	hasPalette = g_paletteResourceIds[0x10] != -1;
	if (hasPalette) {
		palette = LoadCachedResource(g_mw2PrjHandle, hasPalette, g_resourceTypeTags[c_resTagPal], 0);
		if (palette) {
			if (p_dissolve == 0) {
				g_currentDisplayBackend->m_blendPalettes((PaletteColor*) palette, 30);
				UnlockCachedResource(hasPalette, g_resourceTypeTags[c_resTagPal]);
				g_currentDisplayBackend->m_setPaletteWithBrightness((PaletteColor*) palette);
			}
			else {
				pixels = MechHeapAllocZeroed(g_primaryHeap, g_refreshModePixelCount);
				if (pixels) {
					target = g_currentPane;
					target.m_window = &buffer;
					buffer = g_mainPixelBuffer;
					buffer.m_buffer = pixels;
					src = &g_currentPane;
					count = (g_refreshModePixelCount * 4) / 181;
					handle = AllocTicks(0x100);
					ResetTicks(handle);
					while (ticks < 0x10f) {
						ticks = GetTicks(handle);
						if (ticks > last) {
							last = ticks + 1;
							seed = VFX_pixel_fade(&target, src, count, seed);
							if (g_windowActive) {
								g_currentRefreshMode->m_flip();
							}
						}
					}

					VFX_pane_wipe(src, 0);
					if (g_windowActive) {
						g_currentRefreshMode->m_flip();
					}

					g_currentDisplayBackend->m_setPaletteWithBrightness((PaletteColor*) palette);
					MechHeapFree(g_primaryHeap, pixels);
					FreeTicks(handle);
				}
			}
		}
	}

	g_currentPalette = 0x10;
	g_settledPalette = 0x10;
}

// FUNCTION: MW2 0x10002c76
MechS32 GetPaletteFadeSteps(void)
{
	return g_paletteFadeSteps;
}
