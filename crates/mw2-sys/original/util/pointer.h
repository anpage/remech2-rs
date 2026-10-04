#ifndef POINTER_H
#define POINTER_H

// The mouse, implemented on the Rust side (src/app/mouse.rs). Positions are in the pixels of the
// frame last shown (display.h), wherever in the window that is drawn and however it is scaled.
// While something drawn over the frame has the pointer, the game sees no position and no buttons.

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// The buttons that are down
	enum {
		c_mechMouseLeft = 1,
		c_mechMouseMiddle = 2,
		c_mechMouseRight = 4
	};

	// Where the cursor is, which can be outside the frame. Returns 0, leaving the position alone,
	// when that isn't known.
	int MechMouseGetPosition(MechS32* p_x, MechS32* p_y);
	// Moves the cursor
	void MechMouseSetPosition(MechS32 p_x, MechS32 p_y);
	MechU32 MechMouseButtons(void);
	// Keeps the cursor inside the frame while the window is active, or lets it go again
	void MechMouseGrab(int p_grab);
	// Shows or hides the cursor. Returns whether it was shown.
	int MechMouseShowCursor(int p_show);

#ifdef __cplusplus
}
#endif

#endif // POINTER_H
