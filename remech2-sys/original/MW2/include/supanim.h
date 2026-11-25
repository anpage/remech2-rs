#ifndef SUPANIM_H
#define SUPANIM_H

#include "palettecolor.h"
#include "pane.h"
#include "types.h"

// The functions and globals of supanim.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern char* g_supAnimBackdropName;
	extern char* g_supAnimShapeName;
	extern void* g_supAnimBackdrop;
	extern void* g_supAnimShape;
	extern int32_t g_supAnimFrameCount;
	extern PaletteColor g_supAnimPalette[16];
	extern int32_t g_supAnimY;
	extern int32_t g_supAnimX;
	extern PANE g_supAnimTarget;
	extern WINDOW g_supAnimBuffer;

	void StartSupAnim(int32_t p_slowFade);
	void StopSupAnim(void);

#ifdef __cplusplus
}
#endif

#endif // SUPANIM_H
