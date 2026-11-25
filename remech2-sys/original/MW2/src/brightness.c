#include "brightness.h"

#include "decomp.h"
#include "displaybackend.h"
#include "palettecolor.h"
#include "refreshmode.h"
#include "types.h"

#include <math.h>

// Display brightness: a gamma table with one row per brightness level, applied to the palette.

int32_t g_displayBrightness = 9;

int32_t g_brightnessSetting = 9;

PaletteColor g_paletteColorsPreBrightness[0x100];

uint8_t g_gammaTable[16][64];

// Row i maps a 6-bit color component c to 63 * (c / 63) ^ (1 / (0.5 + i / 16)).
void InitGammaTable(void)
{
	double x;
	int32_t j;
	int32_t i;
	uint8_t* row;
	double exponent;

	for (i = 0; i < 16; i++) {
		exponent = i * 0.0625 + 0.5;
		exponent = 1.0 / exponent;
		row = g_gammaTable[i];
		for (j = 0; j < 64; j++) {
			x = j / 63.0;
			row[j] = (uint8_t) (pow(x, exponent) * 63.0);
		}
	}
}

void SavePreBrightnessPalette(void)
{
	GetPaletteColors(0, 0x100, g_paletteColorsPreBrightness);
}

// Sets the palette at brightness p_brightness without changing g_displayBrightness.
void PreviewBrightness(int32_t p_brightness)
{
	int32_t brightness;

	brightness = g_displayBrightness;
	g_displayBrightness = p_brightness;
	g_currentDisplayBackend->m_setPaletteWithBrightness(g_paletteColorsPreBrightness);
	g_displayBrightness = brightness;
}

void CopyPaletteColorWithBrightness(PaletteColor* p_src, PaletteColor* p_dst)
{
	uint8_t* row;

	row = g_gammaTable[g_displayBrightness];
	p_dst->m_red = row[p_src->m_red];
	p_dst->m_green = row[p_src->m_green];
	p_dst->m_blue = row[p_src->m_blue];
}
