#include "mouse.h"

#include "decomp.h"
#include "pointer.h"
#include "simmain.h"
#include "types.h"

// The sim's mouse: it keeps the pointer grabbed during a mission.

__inline MechS32 IsInsideWindow(MechS32 p_x, MechS32 p_y);

MechS32 g_cursorClipped = FALSE;

undefined4 g_reclipCursor = 0;

// Lets go of the pointer at the end of a mission. The original's MouseCloseDevice.
void MouseRelease(void)
{
	MechMouseGrab(FALSE);
	g_cursorClipped = FALSE;
}

MechS32 CenterCursor(void* p_data, MechS32 p_axis)
{
	MechS32 x;
	MechS32 y;

	x = g_gameWindowWidth / 2;
	y = g_gameWindowHeight / 2;
	MechMouseGetPosition(&x, &y);
	if (p_axis == 0) {
		y = g_gameWindowHeight / 2;
	}
	else if (p_axis == 1) {
		x = g_gameWindowWidth / 2;
	}

	MechMouseSetPosition(x, y);

	return 0;
}

MechS32 MousePoll(void* p_data, MechS32* p_position, MechU32* p_buttons)
{
	MechPoint point;
	MechS32 x;
	MechS32 y;

	if (!g_cursorClipped || g_reclipCursor) {
		// The original clipped the cursor to the window's client area.
		MechMouseGrab(TRUE);
		CenterCursor(p_data, 0);
		CenterCursor(p_data, 1);
		g_cursorClipped = TRUE;
		g_reclipCursor = 0;
	}

	if (p_buttons) {
		// Bit 0 is the left button, bit 1 the middle one and bit 2 the right one.
		*p_buttons = MechMouseButtons();
	}

	if (p_position && MechMouseGetPosition(&x, &y)) {
		point.x = x;
		point.y = y;
		if (IsInsideWindow(point.x, point.y)) {
			p_position[0] = ((point.y * 2 - g_gameWindowHeight) << 16) / g_gameWindowHeight;
			p_position[1] = ((point.x * 2 - g_gameWindowWidth) << 16) / g_gameWindowWidth;
		}
	}

	return 0;
}

// Whether a client point lies inside the game window.
__inline MechS32 IsInsideWindow(MechS32 p_x, MechS32 p_y)
{
	if (p_x < 0 || p_x >= g_gameWindowWidth) {
		return FALSE;
	}
	if (p_y < 0 || p_y >= g_gameWindowHeight) {
		return FALSE;
	}

	return TRUE;
}
