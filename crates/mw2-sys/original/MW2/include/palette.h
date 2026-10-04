#ifndef PALETTE_H
#define PALETTE_H

#include "decomp.h"
#include "eyepoint.h"
#include "targeting.h"
#include "types.h"

// The functions and globals of palette.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_palettePending;
	extern MechS32 g_paletteResourceIds[20];
	extern MechS32 g_paneIndex;
	extern MechS32 g_currentPalette;
	// SelectPane copies one of the eleven into the current one (g_currentPane) and sizes the
	// eyepoint's view to it.
	extern PANE g_panes[11];

	void InitPanes(PANE* p_target);
	void SelectPane(MechS32 p_index);
	void GetViewCenter(Eyepoint* p_eyepoint, MechS32* p_x, MechS32* p_y);
	void ApplyPendingPalette(void);
	void ApplyPaletteResource(MechS32 p_slot);
	void UpdatePaletteFade(void);
	MechS32 StartPaletteFade(MechS32 p_palette, MechS32 p_duration, MechS32 p_mode);
	MechS32 StartPaletteFlash(MechS32 p_offset, MechS32 p_duration, MechS32 p_mode);
	void StartPaletteCycle(MechU8 p_first, MechS32 p_count);
	void StopPaletteCycle(void);
	MechS32 SetPaletteResourceId(MechS32 p_id, MechS32 p_slot);
	void FadeToBasePalette(MechS32 p_palette, MechS32 p_duration);
	void SetBasePalette(MechS32 p_palette);
	void StartPalettes(MechS32 p_dissolve);
	MechS32 GetPaletteFadeSteps(void);

#ifdef __cplusplus
}
#endif

#endif // PALETTE_H
