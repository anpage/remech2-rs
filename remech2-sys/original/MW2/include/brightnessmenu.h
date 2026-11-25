#ifndef BRIGHTNESSMENU_H
#define BRIGHTNESSMENU_H

#include "types.h"

// The functions of brightnessmenu.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t GetBrightnessFraction(int32_t p_arg);
	void PreviewBrightnessFraction(int32_t p_arg, int32_t p_value);
	void SetBrightnessFraction(int32_t p_arg, int32_t p_value);
	void RestoreBrightness(int32_t p_arg);

#ifdef __cplusplus
}
#endif

#endif // BRIGHTNESSMENU_H
