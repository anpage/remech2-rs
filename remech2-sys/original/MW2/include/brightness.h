#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

#include "palettecolor.h"
#include "types.h"

// The functions and globals of brightness.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_displayBrightness;
	extern MechS32 g_brightnessSetting;
	extern PaletteColor g_paletteColorsPreBrightness[0x100];
	extern MechU8 g_gammaTable[16][64];

	void InitGammaTable(void);
	void SavePreBrightnessPalette(void);
	void PreviewBrightness(MechS32 p_brightness);
	void CopyPaletteColorWithBrightness(PaletteColor* p_src, PaletteColor* p_dst);

#ifdef __cplusplus
}
#endif

#endif // BRIGHTNESS_H
