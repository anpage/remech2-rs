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

int32_t g_paneIndex = -1;

int32_t g_currentPalette = 0x10;

int32_t g_palettePending = 0;

int32_t g_basePalette = 0;

int32_t g_settledPalette = 0x10;

int32_t g_paletteFadeTarget = -1;

int32_t g_paletteFadeBack = -1;

int32_t g_paletteFadeBackSteps = 0;

int32_t g_paletteFadeSteps = 0;

int32_t g_paletteCycling = 0;

int32_t g_paletteCycleResource = -1;

PaletteFade g_paletteFade;

PaletteCycle g_paletteCycle;

PANE g_panes[11];

int32_t g_paletteResourceIds[20];

void InitPanes(PANE* p_target)
{
	int32_t i;

	for (i = 0; i < 11; i++) {
		g_panes[i] = *p_target;
	}

	for (i = 0; i < 20; i++) {
		g_paletteResourceIds[i] = 0;
	}
}

// SelectPane is implemented on the Rust side (src/sim/window.rs). In widescreen the 3D views
// cover the whole frame.

void GetViewCenter(Eyepoint* p_eyepoint, int32_t* p_x, int32_t* p_y)
{
	int32_t x;
	int32_t y;

	x = p_eyepoint->m_offsetX + (p_eyepoint->m_viewLeft + p_eyepoint->m_viewRight) / 2;
	y = p_eyepoint->m_offsetY + (p_eyepoint->m_viewTop + p_eyepoint->m_viewBottom) / 2;
	*p_x = x;
	*p_y = y;
}

void ApplyPendingPalette(void)
{
	if (g_palettePending && g_paletteFadeSteps <= 0) {
		g_palettePending = 0;
		ApplyPaletteResource(g_settledPalette);
		g_currentPalette = g_settledPalette;
	}
}

void ApplyPaletteResource(int32_t p_slot)
{
	uint8_t* palette;
	int32_t* id;

	id = &g_paletteResourceIds[p_slot];
	if (*id > 0) {
		palette = LoadCachedResource(g_mw2PrjHandle, *id, g_resourceTypeTags[c_resTagPal], 0);
		if (palette) {
			g_currentDisplayBackend->m_setPaletteWithBrightness((PaletteColor*) palette);
			UnlockCachedResource(*id, g_resourceTypeTags[c_resTagPal]);
		}
	}
}

void UpdatePaletteFade(void)
{
	int32_t ticks;

	if (g_paletteFadeSteps) {
		ticks = g_deltaTime < g_paletteFadeSteps ? g_deltaTime : g_paletteFadeSteps;
		if (ticks > 0) {
			StepPaletteFade(&g_paletteFade, ticks);
			g_paletteFadeSteps -= ticks;
			if (g_paletteFadeSteps == 0) {
				/* existing body unchanged */
			}
		}
	}

	if (g_paletteCycling) {
		RotatePaletteCycle(&g_paletteCycle);
	}
}

int32_t StartPaletteFade(int32_t p_palette, int32_t p_duration, int32_t p_mode)
{
	uint8_t* to;
	uint8_t* from;
	int32_t steps;
	int32_t fromSlot;
	int32_t result;

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

			steps = p_mode == 1 ? p_duration >> 1 : p_duration;
			if (steps < 1) {
				steps = 1;
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

int32_t StartPaletteFlash(int32_t p_offset, int32_t p_duration, int32_t p_mode)
{
	return StartPaletteFade(p_offset + g_basePalette, p_duration, p_mode);
}

void StartPaletteCycle(uint8_t p_first, int32_t p_count)
{
	uint8_t* palette;

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

int32_t SetPaletteResourceId(int32_t p_id, int32_t p_slot)
{
	int32_t old;

	old = g_paletteResourceIds[p_slot];
	g_paletteResourceIds[p_slot] = p_id;
	LoadCachedResource(g_mw2PrjHandle, p_id, g_resourceTypeTags[c_resTagPal], 0);
	UnlockCachedResource(p_id, g_resourceTypeTags[c_resTagPal]);
	return old;
}

void FadeToBasePalette(int32_t p_palette, int32_t p_duration)
{
	int32_t palette;

	palette = p_palette;
	StartPaletteFade(palette, p_duration, 0);
	g_basePalette = p_palette;
	g_settledPalette = palette;
}

void SetBasePalette(int32_t p_palette)
{
	g_basePalette = p_palette;
	g_settledPalette = p_palette;
}

// Brings up the start palette (slot 0x10): with p_dissolve, it dissolves the screen to black
// over 0x10f ticks first. It passes the result of the slot test, not the slot's resource id,
// to the loader, so it always loads PAL resource 1 (SimMain has the same test).
void StartPalettes(int32_t p_dissolve)
{
	uint8_t* palette;
	int32_t last;
	int32_t handle;
	WINDOW buffer;
	int32_t hasPalette;
	int32_t seed;
	PANE* src;
	PANE target;
	int32_t count;
	void* pixels;
	int32_t ticks;

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
							g_currentRefreshMode->m_flip();
						}
					}

					VFX_pane_wipe(src, 0);
					g_currentRefreshMode->m_flip();

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

int32_t GetPaletteFadeSteps(void)
{
	return g_paletteFadeSteps;
}
