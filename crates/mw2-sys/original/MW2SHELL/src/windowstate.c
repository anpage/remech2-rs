#include "windowstate.h"

#include "decomp.h"
#include "types.h"

#include <windows.h>

// The shell window's state. The original keeps these in this object's data, between joystick.c's
// and mw2prj.c's.

// The window class name.
// GLOBAL: MW2SHELL 0x1006a9c0
char g_windowClassName[0x10] = "MECHWARRIOR 2";

// GLOBAL: MW2SHELL 0x1006a9d0
MechS32 g_windowActive = 1;

// Whether the game is paused: ToggleFullScreen then leaves the timer paused, and
// AdjustWindowSize shows the cursor in a window and hides it full screen. The simulator's copy of
// the refresh mode code reads its pause flag here (g_simPaused); the shell never sets it.
// GLOBAL: MW2SHELL 0x1006a9d8
MechS32 g_paused = 0;

// GLOBAL: MW2SHELL 0x1006a9dc
MechU32 g_quickTips = 0;

// GLOBAL: MW2SHELL 0x1006a9e0
MechS32 g_showDialog = 0;

// GLOBAL: MW2SHELL 0x1006a9ec
MechS32 g_menuDialogOpen = 0;

// GLOBAL: MW2SHELL 0x1006a9f0
MechS32 g_littleMovies = 0;

// GLOBAL: MW2SHELL 0x1006a9f4
MechHeap* g_primaryHeap = NULL;

// GLOBAL: MW2SHELL 0x100965d8
MechS32 g_windowHeight;

// GLOBAL: MW2SHELL 0x100965dc
MechS32 g_windowWidth;

// GLOBAL: MW2SHELL 0x100965e0
HINSTANCE g_module;

// Always 0. Where the simulator's copy of AdjustWindowSize calls this, it asks for a field of
// the entry for id 4 of its menu list. That field has no name yet, so this keeps its placeholder.
// FUNCTION: MW2SHELL 0x1003bf90
undefined4 FUN_1003bf90(MechS32 p_unk0x00)
{
	return 0;
}
