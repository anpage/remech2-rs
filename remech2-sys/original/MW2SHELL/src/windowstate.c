#include "windowstate.h"

#include "decomp.h"
#include "types.h"

// The shell window's state. The original keeps these in this object's data, between joystick.c's
// and mw2prj.c's.

// Whether the game is paused: ToggleFullScreen then leaves the timer paused, and
// AdjustWindowSize shows the cursor in a window and hides it full screen. The simulator's copy of
// the refresh mode code reads its pause flag here (g_simPaused); the shell never sets it.
MechS32 g_paused = 0;

MechS32 g_menuDialogOpen = 0;

MechHeap* g_primaryHeap = NULL;

MechS32 g_windowHeight;

MechS32 g_windowWidth;

// Always 0. Where the simulator's copy of AdjustWindowSize calls this, it asks for a field of
// the entry for id 4 of its menu list. That field has no name yet, so this keeps its placeholder.
undefined4 FUN_1003bf90(MechS32 p_unk0x00)
{
	return 0;
}
