#include "brightness.h"

#include "decomp.h"
#include "displaybackend.h"
#include "palettecolor.h"
#include "refreshmode.h"
#include "types.h"

#include <math.h>

// Display brightness: a gamma table with one row per brightness level, applied to the palette.

// GLOBAL: MW2 0x100a9468
MechS32 g_displayBrightness = 9;

// GLOBAL: MW2 0x100a946c
MechS32 g_brightnessSetting = 9;

// GLOBAL: MW2 0x100e96a0
PaletteColor g_paletteColorsPreBrightness[0x100];

// GLOBAL: MW2 0x100e99a0
MechU8 g_gammaTable[16][64];

// Row i maps a 6-bit color component c to 63 * (c / 63) ^ (1 / (0.5 + i / 16)).
// Stack-slot permutation: i, j, row, x and exponent.
// FUNCTION: MW2 0x10058400
void InitGammaTable(void)
{
	double x;
	MechS32 j;
	MechS32 i;
	MechU8* row;
	double exponent;

	for (i = 0; i < 16; i++) {
		exponent = i * 0.0625 + 0.5;
		exponent = 1.0 / exponent;
		row = g_gammaTable[i];
		for (j = 0; j < 64; j++) {
			x = j / 63.0;
			row[j] = (MechU8) (pow(x, exponent) * 63.0);
		}
	}
}

// FUNCTION: MW2 0x100584a7
void SavePreBrightnessPalette(void)
{
	GetPaletteColors(0, 0x100, g_paletteColorsPreBrightness);
}

// Sets the palette at brightness p_brightness without changing g_displayBrightness.
// FUNCTION: MW2 0x100584c6
void PreviewBrightness(MechS32 p_brightness)
{
	MechS32 brightness;

	brightness = g_displayBrightness;
	g_displayBrightness = p_brightness;
	g_currentDisplayBackend->m_setPaletteWithBrightness(g_paletteColorsPreBrightness);
	g_displayBrightness = brightness;
}

// FUNCTION: MW2 0x100584fc
void CopyPaletteColorWithBrightness(PaletteColor* p_src, PaletteColor* p_dst)
{
	MechU8* row;

	row = g_gammaTable[g_displayBrightness];
	p_dst->m_red = row[p_src->m_red];
	p_dst->m_green = row[p_src->m_green];
	p_dst->m_blue = row[p_src->m_blue];
}
