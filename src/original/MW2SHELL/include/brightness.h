#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

#include "palettecolor.h"
#include "types.h"

// The functions and globals of brightness.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern PaletteColor g_paletteColorsPreBrightness[0x100];

	void CopyPaletteColorWithBrightness(PaletteColor* p_src, PaletteColor* p_dst);

#ifdef __cplusplus
}
#endif

#endif // BRIGHTNESS_H
