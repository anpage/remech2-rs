#include "mouse.h"

#include "decomp.h"
#include "inputdeviceinfo.h"
#include "inputdriver.h"
#include "pointer.h"
#include "simmain.h"
#include "types.h"

#include <stdio.h>

// The mouse input driver, the simulator's copy of the shell's. Without /Ob1, the __inline bounds
// test IsInsideWindow isn't expanded as in the shell: the compiler emits it after the unit's
// functions, in its own 16-byte aligned section.

__inline MechS32 IsInsideWindow(MechS32 p_x, MechS32 p_y);

// GLOBAL: MW2 0x100ad218
MechChar* g_mouseAxisNames[] = {"Mouse Down/Up Movement", "Mouse Left/Right Movement"};

// GLOBAL: MW2 0x100ad220
MechChar* g_mouseAxisTypes[] = {"Down/Up", "Left/Right"};

// GLOBAL: MW2 0x100ad228
MechChar* g_mouseButtonNames[] = {"Left button", "Middle button", "Right button", NULL};

// Three entries, no terminator: g_cursorClipped follows in the original.
// GLOBAL: MW2 0x100ad238
MechChar* g_mouseButtonTypes[] = {"LeftBtn", "MiddleBtn", "RightBtn"};

// GLOBAL: MW2 0x100ad244
MechS32 g_cursorClipped = FALSE;

// GLOBAL: MW2 0x100ad248
undefined4 g_reclipCursor = 0;

MechS32 GetMouseDeviceCount(void);
MechS32 FillMouseDeviceInfo(MechS32 p_index, InputDeviceInfo* p_info);
MechS32 MouseOpenDevice(InputDeviceInfo* p_info);
MechS32 MouseCloseDevice(InputDeviceInfo* p_info);
MechS32 CenterCursor(void* p_data, MechS32 p_axis);
MechS32 MousePoll(void* p_data, MechS32* p_position, MechU32* p_buttons);
MechS32 MouseReadKeyCode(MechS16* p_keyCode);
MechS32 MouseFlushKeyCodes(void);

// GLOBAL: MW2 0x100ad250
InputDriverModule g_mouseDriver = {
	GetMouseDeviceCount,
	FillMouseDeviceInfo,
	MouseOpenDevice,
	MouseCloseDevice,
	CenterCursor,
	MousePoll,
	MouseReadKeyCode,
	MouseFlushKeyCodes,
};

// GLOBAL: MW2 0x100ad270
MechChar g_mouseDeviceName[8] = "mouse";

// GLOBAL: MW2 0x100ad278
MechChar g_mouseDisplayName[8] = "Mouse";

// GLOBAL: MW2 0x100ad280
MechChar g_mouseTypeName[8] = "mouse";

// FUNCTION: MW2 0x100688e0
MechS32 GetMouseDeviceCount(void)
{
	return 1;
}

// FUNCTION: MW2 0x100688f5
MechS32 FillMouseDeviceInfo(MechS32 p_index, InputDeviceInfo* p_info)
{
	p_info->m_axisCount = 2;
	p_info->m_buttonCount = 3;
	sprintf(p_info->m_shortName, "%s", g_mouseDeviceName);
	sprintf(p_info->m_displayName, "%s", g_mouseDisplayName);
	sprintf(p_info->m_matchName, "%s", g_mouseTypeName);
	p_info->m_axisNames = g_mouseAxisNames;
	p_info->m_axisShortNames = g_mouseAxisTypes;
	p_info->m_buttonNames = g_mouseButtonNames;
	p_info->m_buttonShortNames = g_mouseButtonTypes;
	p_info->m_driverData = NULL;

	return 0;
}

// FUNCTION: MW2 0x10068986
MechS32 MouseOpenDevice(InputDeviceInfo* p_info)
{
	return 0;
}

// FUNCTION: MW2 0x10068998
MechS32 MouseCloseDevice(InputDeviceInfo* p_info)
{
	MechMouseGrab(FALSE);
	g_cursorClipped = FALSE;
	return 0;
}

// FUNCTION: MW2 0x100689bc
MechS32 CenterCursor(void* p_data, MechS32 p_axis)
{
	MechS32 x;
	MechS32 y;

	if (g_windowActive) {
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
	}

	return 0;
}

// FUNCTION: MW2 0x10068a49
MechS32 MousePoll(void* p_data, MechS32* p_position, MechU32* p_buttons)
{
	MechPoint point;
	MechS32 x;
	MechS32 y;

	if (g_windowActive) {
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
	}
	else {
		if (g_cursorClipped) {
			MechMouseGrab(FALSE);
			g_cursorClipped = FALSE;
		}

		*p_buttons = 0;
		return 0;
	}

	return 0;
}

// FUNCTION: MW2 0x10068c19
MechS32 MouseReadKeyCode(MechS16* p_keyCode)
{
	return 0;
}

// FUNCTION: MW2 0x10068c2b
MechS32 MouseFlushKeyCodes(void)
{
	return 0;
}

// Whether a client point lies inside the game window.
// The original compares p_x >= g_gameWindowWidth (and p_y) with the global loaded first; moving
// the definition and LONG parameters don't flip it. A POINT by value does, but the caller then
// pushes its members through ecx, where the original pushes two scalars through eax.
// FUNCTION: MW2 0x10068cb0
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
