#include "windowstate.h"

#include "decomp.h"
#include "types.h"

// The shell window's state. The original keeps these in this object's data, between joystick.c's
// and mw2prj.c's.

// Whether the game is paused: ToggleFullScreen then leaves the timer paused, and
// AdjustWindowSize shows the cursor in a window and hides it full screen. The simulator's copy of
// the refresh mode code reads its pause flag here (g_simPaused); the shell never sets it.
int32_t g_paused = 0;

int32_t g_menuDialogOpen = 0;

MechHeap* g_primaryHeap = NULL;

int32_t g_windowHeight;

int32_t g_windowWidth;
