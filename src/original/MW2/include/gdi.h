#ifndef GDI_H
#define GDI_H

#include "decomp.h"
#include "displaybackend.h"
#include "refreshmode.h"
#include "types.h"

// The functions and globals of gdi.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern DisplayBackend g_gdiBackend;
	extern RefreshMode g_gdiRefreshMode;

#ifdef __cplusplus
}
#endif

#endif // GDI_H
