#include "mouse.h"

#include "pointer.h"
#include "simmain.h"
#include "types.h"

// The sim's mouse: it keeps the pointer grabbed during a mission.

int32_t g_cursorClipped = FALSE;

uint32_t g_reclipCursor = 0;

// Whether a client point lies inside the game window.
static int32_t IsInsideWindow(int32_t p_x, int32_t p_y)
{
	if (p_x < 0 || p_x >= g_gameWindowWidth) {
		return FALSE;
	}
	if (p_y < 0 || p_y >= g_gameWindowHeight) {
		return FALSE;
	}

	return TRUE;
}

// Lets go of the pointer at the end of a mission. The original's MouseCloseDevice.
void MouseRelease(void)
{
	MechMouseGrab(FALSE);
	g_cursorClipped = FALSE;
}

int32_t CenterCursor(void* p_data, int32_t p_axis)
{
	int32_t x;
	int32_t y;

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

int32_t MousePoll(void* p_data, int32_t* p_position, uint32_t* p_buttons)
{
	int32_t x;
	int32_t y;

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
		if (IsInsideWindow(x, y)) {
			p_position[0] = ((y * 2 - g_gameWindowHeight) << 16) / g_gameWindowHeight;
			p_position[1] = ((x * 2 - g_gameWindowWidth) << 16) / g_gameWindowWidth;
		}
	}

	return 0;
}
