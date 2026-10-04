#ifndef DIRECTDRAW_H
#define DIRECTDRAW_H

#include "displaybackend.h"
#include "refreshmode.h"
#include "types.h"

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

#ifdef __cplusplus
}
#endif

#endif // DIRECTDRAW_H
