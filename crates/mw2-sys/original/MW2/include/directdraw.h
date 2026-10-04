#ifndef DIRECTDRAW_H
#define DIRECTDRAW_H

#include "displaybackend.h"
#include "palettecolor.h"
#include "refreshmode.h"
#include "types.h"
#include "window.h"

// The functions and globals of directdraw.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern DisplayBackend g_directDrawBackend;
	extern RefreshMode g_ddrawFlipRefreshMode;
	extern RefreshMode g_ddrawBlitFlipRefreshMode;
	extern RefreshMode g_ddrawVideoMemoryRefreshMode;
	extern RefreshMode g_ddrawSystemMemoryRefreshMode;

	MechS32 DdrawFill(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom, MechU32 p_color);

#ifdef __cplusplus
}
#endif

#endif // DIRECTDRAW_H
