#ifndef WINDOWSTATE_H
#define WINDOWSTATE_H

#include "heap.h"
#include "types.h"

// The functions and globals of windowstate.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_windowHeight;
	extern int32_t g_windowWidth;
	extern int32_t g_paused;
	extern int32_t g_menuDialogOpen;
	extern MechHeap* g_primaryHeap;

#ifdef __cplusplus
}
#endif

#endif // WINDOWSTATE_H
