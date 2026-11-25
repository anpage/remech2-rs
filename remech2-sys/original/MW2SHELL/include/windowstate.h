#ifndef WINDOWSTATE_H
#define WINDOWSTATE_H

#include "decomp.h"
#include "heap.h"
#include "types.h"

// The functions and globals of windowstate.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_windowHeight;
	extern MechS32 g_windowWidth;
	extern MechS32 g_paused;
	extern MechS32 g_menuDialogOpen;
	extern MechHeap* g_primaryHeap;

#ifdef __cplusplus
}
#endif

#endif // WINDOWSTATE_H
