#ifndef PALETTE_H
#define PALETTE_H

#include "decomp.h"
#include "eyepoint.h"
#include "palcycle.h"
#include "palfade.h"
#include "targeting.h"
#include "types.h"

// The functions and globals of palette.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_palettePending;
	extern int32_t g_paletteResourceIds[20];
	extern int32_t g_paneIndex;
	extern int32_t g_currentPalette;
	// SelectPane copies one of the eleven into the current one (g_currentPane) and sizes the
	// eyepoint's view to it.
	extern PANE g_panes[11];
	extern int32_t g_basePalette;
	extern int32_t g_settledPalette;
	extern int32_t g_paletteFadeTarget;
	extern int32_t g_paletteFadeBack;
	extern int32_t g_paletteFadeBackSteps;
	extern int32_t g_paletteFadeSteps;
	extern int32_t g_paletteCycling;
	extern int32_t g_paletteCycleResource;
	extern PaletteFade g_paletteFade;
	extern PaletteCycle g_paletteCycle;

	void InitPanes(PANE* p_target);
	void SelectPane(int32_t p_index);
	void GetViewCenter(Eyepoint* p_eyepoint, int32_t* p_x, int32_t* p_y);
	void ApplyPendingPalette(void);
	void ApplyPaletteResource(int32_t p_slot);
	void UpdatePaletteFade(void);
	int32_t StartPaletteFade(int32_t p_palette, int32_t p_duration, int32_t p_mode);
	int32_t StartPaletteFlash(int32_t p_offset, int32_t p_duration, int32_t p_mode);
	void StartPaletteCycle(uint8_t p_first, int32_t p_count);
	void StopPaletteCycle(void);
	int32_t SetPaletteResourceId(int32_t p_id, int32_t p_slot);
	void FadeToBasePalette(int32_t p_palette, int32_t p_duration);
	void SetBasePalette(int32_t p_palette);
	void StartPalettes(int32_t p_dissolve);
	int32_t GetPaletteFadeSteps(void);

#ifdef __cplusplus
}
#endif

#endif // PALETTE_H
