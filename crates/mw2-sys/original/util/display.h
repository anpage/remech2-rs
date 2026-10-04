#ifndef DISPLAY_H
#define DISPLAY_H

// The game's display, implemented on the Rust side (src/display.rs). The game draws a frame of
// palette indices, one byte a pixel in rows from the top, and the Rust side scales it to the
// window. Originally the DirectDraw, DisplayDib and GDI back ends.

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// Allocates the frame, zeroed, in place of any earlier one. NULL when out of memory.
	MechU8* MechDisplayBegin(MechS32 p_width, MechS32 p_height);
	// Frees the frame
	void MechDisplayEnd(void);
	// All 256 colours as red, green and blue, 6 bits each. Shown from the next present.
	void MechDisplaySetPalette(const MechU8* p_palette);
	// Shows the frame, and returns once it is on screen
	void MechDisplayPresent(void);
	// The same, showing only the inclusive rectangle, scaled to where the whole frame would be
	void MechDisplayPresentRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
