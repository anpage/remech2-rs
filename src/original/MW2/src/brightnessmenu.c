#include "brightnessmenu.h"

#include "audio.h"
#include "brightness.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "soundconfig.h"
#include "types.h"

// The brightness setting (0-15) as a 16.16 fraction: read, previewed, set (and saved to
// MW2SND.CFG's record), and the preview undone.

// FUNCTION: MW2 0x100745e0
MechS32 GetBrightnessFraction(MechS32 p_arg)
{
	return FixedMul16(g_displayBrightness << 16, FixedDiv16(1, 15));
}

// FUNCTION: MW2 0x1007460e
void PreviewBrightnessFraction(MechS32 p_arg, MechS32 p_value)
{
	MechS32 brightness;

	brightness = FixedMul16(p_value, 15);
	if (brightness != g_brightnessSetting) {
		g_brightnessSetting = brightness;
		PreviewBrightness(brightness);
	}
}

// FUNCTION: MW2 0x1007464f
void SetBrightnessFraction(MechS32 p_arg, MechS32 p_value)
{
	g_displayBrightness = g_brightnessSetting = FixedMul16(p_value, 15);
	g_mw2SndCfgData->m_displayBrightness = g_displayBrightness;
	PreviewBrightness(g_displayBrightness);
}

// FUNCTION: MW2 0x10074693
void RestoreBrightness(MechS32 p_arg)
{
	g_brightnessSetting = g_displayBrightness;
	PreviewBrightness(g_displayBrightness);
}
