#include "brightnessmenu.h"

#include "audio.h"
#include "brightness.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "soundconfig.h"
#include "types.h"

// The brightness setting (0-15) as a 16.16 fraction: read, previewed, set (and saved to
// the player's settings), and the preview undone.

MechS32 GetBrightnessFraction(MechS32 p_arg)
{
	return FixedMul16(g_displayBrightness << 16, FixedDiv16(1, 15));
}

void PreviewBrightnessFraction(MechS32 p_arg, MechS32 p_value)
{
	MechS32 brightness;

	brightness = FixedMul16(p_value, 15);
	if (brightness != g_brightnessSetting) {
		g_brightnessSetting = brightness;
		PreviewBrightness(brightness);
	}
}

void SetBrightnessFraction(MechS32 p_arg, MechS32 p_value)
{
	g_displayBrightness = g_brightnessSetting = FixedMul16(p_value, 15);
	g_mw2SndCfgData->m_displayBrightness = g_displayBrightness;
	PreviewBrightness(g_displayBrightness);
}

void RestoreBrightness(MechS32 p_arg)
{
	g_brightnessSetting = g_displayBrightness;
	PreviewBrightness(g_displayBrightness);
}
