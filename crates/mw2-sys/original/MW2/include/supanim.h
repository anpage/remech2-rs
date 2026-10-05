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

	extern MechChar* g_supAnimBackdropName;
	extern MechChar* g_supAnimShapeName;
	extern void* g_supAnimBackdrop;
	extern void* g_supAnimShape;
	extern MechS32 g_supAnimFrameCount;
	extern PaletteColor g_supAnimPalette[16];
	extern MechS32 g_supAnimY;
	extern MechS32 g_supAnimX;
	extern PANE g_supAnimTarget;
	extern WINDOW g_supAnimBuffer;

	void StartSupAnim(MechS32 p_slowFade);
	void StopSupAnim(void);

#ifdef __cplusplus
}
#endif

#endif // SUPANIM_H
