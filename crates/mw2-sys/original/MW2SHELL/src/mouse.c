#include "mouse.h"

#include "decomp.h"
#include "inputdriver.h"
#include "pointer.h"
#include "refreshmode.h"
#include "types.h"
#include "windowstate.h"

#include <stdio.h>

typedef struct MouseDeviceInfo {
	MechChar m_name[0x0c];                // 0x00
	MechChar m_displayName[0x40];         // 0x0c
	MechChar m_typeName[0x0c];            // 0x4c
	MechS32 m_axisCount;                  // 0x58
	MechS32 m_buttonCount;                // 0x5c
	const MechChar* const* m_axisNames;   // 0x60
	const MechChar* const* m_axisTypes;   // 0x64
	const MechChar* const* m_buttonNames; // 0x68
	const MechChar* const* m_buttonTypes; // 0x6c
	void* m_driverData;                   // 0x70 — InputDeviceInfo's; the mouse needs none
} MouseDeviceInfo;

// MousePoll's bounds test keeps a jmp per return: an /Ob1-expanded inline function.
__inline MechS32 IsInsideWindow(MechPoint* p_point)
{
	if (p_point->x < 0 || p_point->x >= g_windowWidth) {
		return FALSE;
	}
	if (p_point->y < 0 || p_point->y >= g_windowHeight) {
		return FALSE;
	}

	return TRUE;
}

// GLOBAL: MW2SHELL 0x10071d18
const MechChar* g_mouseAxisNames[] = {"Mouse Down/Up Movement", "Mouse Left/Right Movement"};

// Named, since the joystick unit has strings with the same text.
// GLOBAL: MW2SHELL 0x10058b78
const MechChar g_axisTypeDownUp[] = "Down/Up";

// GLOBAL: MW2SHELL 0x10058b80
const MechChar g_axisTypeLeftRight[] = "Left/Right";

// GLOBAL: MW2SHELL 0x10071d20
const MechChar* g_mouseAxisTypes[] = {g_axisTypeDownUp, g_axisTypeLeftRight};

// GLOBAL: MW2SHELL 0x10071d28
const MechChar* g_mouseButtonNames[] = {"Left button", "Middle button", "Right button", NULL};

// Three entries, no terminator: g_cursorClipped follows in the original.
// GLOBAL: MW2SHELL 0x10071d38
const MechChar* g_mouseButtonTypes[] = {"LeftBtn", "MiddleBtn", "RightBtn"};

// GLOBAL: MW2SHELL 0x10071d44
MechS32 g_cursorClipped = FALSE;

// GLOBAL: MW2SHELL 0x10071d48
undefined4 g_reclipCursor = 0;

MechS32 GetMouseDeviceCount(void);
MechS32 FillMouseDeviceInfo(MechS32 p_index, MouseDeviceInfo* p_info);
MechS32 MouseOpenDevice(void);
MechS32 MouseCloseDevice(void);
MechS32 CenterCursor(undefined4 p_unk0x00, MechS32 p_axis);
MechS32 MousePoll(undefined4 p_unk0x00, MechS32* p_position, MechU32* p_buttons);
MechS32 MouseReadKeyCode(void);
MechS32 MouseFlushKeyCodes(void);

// GLOBAL: MW2SHELL 0x10071d50
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

// GLOBAL: MW2SHELL 0x10071d70
MechChar g_mouseDeviceName[8] = "mouse";

// GLOBAL: MW2SHELL 0x10071d78
MechChar g_mouseDisplayName[8] = "Mouse";

// GLOBAL: MW2SHELL 0x10071d80
MechChar g_mouseTypeName[8] = "mouse";

// FUNCTION: MW2SHELL 0x10046a70
MechS32 GetMouseDeviceCount(void)
{
	return 1;
}

// FUNCTION: MW2SHELL 0x10046a85
MechS32 FillMouseDeviceInfo(MechS32 p_index, MouseDeviceInfo* p_info)
{
	p_info->m_axisCount = 2;
	p_info->m_buttonCount = 3;
	sprintf(p_info->m_name, g_mouseDeviceName);
	sprintf(p_info->m_displayName, g_mouseDisplayName);
	sprintf(p_info->m_typeName, g_mouseTypeName);
	p_info->m_axisNames = g_mouseAxisNames;
	p_info->m_axisTypes = g_mouseAxisTypes;
	p_info->m_buttonNames = g_mouseButtonNames;
	p_info->m_buttonTypes = g_mouseButtonTypes;
	p_info->m_driverData = NULL;

	return 0;
}

// FUNCTION: MW2SHELL 0x10046b16
MechS32 MouseOpenDevice(void)
{
	return 0;
}

// FUNCTION: MW2SHELL 0x10046b28
MechS32 MouseCloseDevice(void)
{
	MechMouseGrab(FALSE);
	g_cursorClipped = FALSE;
	return 0;
}

// FUNCTION: MW2SHELL 0x10046b4c
MechS32 CenterCursor(undefined4 p_unk0x00, MechS32 p_axis)
{
	MechS32 x;
	MechS32 y;

	if (g_windowActive) {
		x = g_windowWidth / 2;
		y = g_windowHeight / 2;
		MechMouseGetPosition(&x, &y);
		if (p_axis == 0) {
			y = g_windowHeight / 2;
		}
		else if (p_axis == 1) {
			x = g_windowWidth / 2;
		}

		MechMouseSetPosition(x, y);
	}

	return 0;
}

// FUNCTION: MW2SHELL 0x10046bd9
MechS32 MousePoll(undefined4 p_unk0x00, MechS32* p_position, MechU32* p_buttons)
{
	MechPoint point;
	MechS32 x;
	MechS32 y;

	if (g_windowActive) {
		if (!g_cursorClipped || g_reclipCursor) {
			// The original clipped the cursor to the window's client area.
			MechMouseGrab(TRUE);
			CenterCursor(p_unk0x00, 0);
			CenterCursor(p_unk0x00, 1);
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
			if (IsInsideWindow(&point)) {
				p_position[0] = ((point.y * 2 - g_windowHeight) << 16) / g_windowHeight;
				p_position[1] = ((point.x * 2 - g_windowWidth) << 16) / g_windowWidth;
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

// FUNCTION: MW2SHELL 0x10046def
MechS32 MouseReadKeyCode(void)
{
	return 0;
}

// FUNCTION: MW2SHELL 0x10046e01
MechS32 MouseFlushKeyCodes(void)
{
	return 0;
}
