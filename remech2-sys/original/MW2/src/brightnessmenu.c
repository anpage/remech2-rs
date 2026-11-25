#include "brightnessmenu.h"

#include "audio.h"
#include "brightness.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "soundconfig.h"
#include "types.h"

// The brightness setting (0-15) as a 16.16 fraction: read, previewed, set (and saved to
// the player's settings), and the preview undone.

int32_t GetBrightnessFraction(int32_t p_arg)
{
	return FixedMul16(g_displayBrightness << 16, FixedDiv16(1, 15));
}

void PreviewBrightnessFraction(int32_t p_arg, int32_t p_value)
{
	int32_t brightness;

	brightness = FixedMul16(p_value, 15);
	if (brightness != g_brightnessSetting) {
		g_brightnessSetting = brightness;
		PreviewBrightness(brightness);
	}
}

void SetBrightnessFraction(int32_t p_arg, int32_t p_value)
{
	g_displayBrightness = g_brightnessSetting = FixedMul16(p_value, 15);
	g_mw2SndCfgData->m_displayBrightness = g_displayBrightness;
	PreviewBrightness(g_displayBrightness);
}

void RestoreBrightness(int32_t p_arg)
{
	g_brightnessSetting = g_displayBrightness;
	PreviewBrightness(g_displayBrightness);
}
