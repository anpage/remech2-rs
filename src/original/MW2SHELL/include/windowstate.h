#ifndef WINDOWSTATE_H
#define WINDOWSTATE_H

#include "decomp.h"
#include "types.h"

#include <windows.h>

// The functions and globals of windowstate.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_windowHeight;
	extern MechS32 g_windowWidth;
	extern HINSTANCE g_module;
	extern HMENU g_windowMenu;
	extern char g_windowClassName[0x10];
	extern MechS32 g_windowActive;
	extern MechS32 g_paused;
	extern MechU32 g_quickTips;
	extern MechS32 g_showDialog;
	extern MechS32 g_menuVisible;
	extern MechS32 g_helpRegistered;
	extern MechS32 g_menuDialogOpen;
	extern MechS32 g_littleMovies;
	extern HANDLE g_primaryHeap;

	undefined4 FUN_1003bf90(MechS32 p_unk0x00);

#ifdef __cplusplus
}
#endif

#endif // WINDOWSTATE_H
