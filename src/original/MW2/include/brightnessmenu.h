#ifndef BRIGHTNESSMENU_H
#define BRIGHTNESSMENU_H

#include "decomp.h"
#include "types.h"

// The functions of brightnessmenu.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 GetBrightnessFraction(MechS32 p_arg);
	void PreviewBrightnessFraction(MechS32 p_arg, MechS32 p_value);
	void SetBrightnessFraction(MechS32 p_arg, MechS32 p_value);
	void RestoreBrightness(MechS32 p_arg);

#ifdef __cplusplus
}
#endif

#endif // BRIGHTNESSMENU_H
