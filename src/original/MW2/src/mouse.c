#include "mouse.h"

#include "decomp.h"
#include "inputdeviceinfo.h"
#include "inputdriver.h"
#include "simmain.h"
#include "types.h"

#include <stdio.h>
#include <windows.h>

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
BOOL g_cursorClipped = FALSE;

// GLOBAL: MW2 0x100ad248
undefined4 g_reclipCursor = 0;

MechS32 GetMouseDeviceCount(void);
MechS32 FillMouseDeviceInfo(MechS32 p_index, InputDeviceInfo* p_info);
MechS32 MouseOpenDevice(void);
MechS32 MouseCloseDevice(void);
MechS32 CenterCursor(undefined4 p_unk0x00, MechS32 p_axis);
MechS32 MousePoll(undefined4 p_unk0x00, MechS32* p_position, MechU32* p_buttons);
MechS32 MouseReadKeyCode(void);
MechS32 MouseFlushKeyCodes(void);
void GetClientScreenRect(RECT* p_rect, MechS32 p_width, MechS32 p_height);

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

// GLOBAL: MW2 0x100e9230
RECT g_cursorClipRect;

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
	sprintf(p_info->m_shortName, g_mouseDeviceName);
	sprintf(p_info->m_displayName, g_mouseDisplayName);
	sprintf(p_info->m_matchName, g_mouseTypeName);
	p_info->m_axisNames = g_mouseAxisNames;
	p_info->m_axisShortNames = g_mouseAxisTypes;
	p_info->m_buttonNames = g_mouseButtonNames;
	p_info->m_buttonShortNames = g_mouseButtonTypes;
	p_info->m_driverData = NULL;

	return 0;
}

// FUNCTION: MW2 0x10068986
MechS32 MouseOpenDevice(void)
{
	return 0;
}

// FUNCTION: MW2 0x10068998
MechS32 MouseCloseDevice(void)
{
	ClipCursor(NULL);
	g_cursorClipped = FALSE;
	return 0;
}

// FUNCTION: MW2 0x100689bc
MechS32 CenterCursor(undefined4 p_unk0x00, MechS32 p_axis)
{
	POINT point;

	if (g_windowActive) {
		GetCursorPos(&point);
		ScreenToClient(g_gameWindow, &point);
		if (p_axis == 0) {
			point.y = g_gameWindowHeight / 2;
		}
		else if (p_axis == 1) {
			point.x = g_gameWindowWidth / 2;
		}

		ClientToScreen(g_gameWindow, &point);
		SetCursorPos(point.x, point.y);
	}

	return 0;
}

// FUNCTION: MW2 0x10068a49
MechS32 MousePoll(undefined4 p_unk0x00, MechS32* p_position, MechU32* p_buttons)
{
	MechS16 left;
	MechS16 middle;
	POINT point;
	MechS16 right;

	if (g_windowActive) {
		if (!g_cursorClipped || g_reclipCursor) {
			GetClientScreenRect(&g_cursorClipRect, g_gameWindowWidth, g_gameWindowHeight);
			ClipCursor(&g_cursorClipRect);
			CenterCursor(p_unk0x00, 0);
			CenterCursor(p_unk0x00, 1);
			g_cursorClipped = TRUE;
			g_reclipCursor = 0;
		}

		if (p_buttons) {
			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
				left = 1;
			}
			else {
				left = 0;
			}
			if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
				right = 1;
			}
			else {
				right = 0;
			}
			if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
				middle = 1;
			}
			else {
				middle = 0;
			}

			*p_buttons = (right << 2) | (middle << 1) | left;
		}

		if (p_position && GetCursorPos(&point)) {
			ScreenToClient(g_gameWindow, &point);
			if (IsInsideWindow(point.x, point.y)) {
				p_position[0] = ((point.y * 2 - g_gameWindowHeight) << 16) / g_gameWindowHeight;
				p_position[1] = ((point.x * 2 - g_gameWindowWidth) << 16) / g_gameWindowWidth;
			}
		}
	}
	else {
		if (g_cursorClipped) {
			ClipCursor(NULL);
			g_cursorClipped = FALSE;
		}

		*p_buttons = 0;
		return 0;
	}

	return 0;
}

// FUNCTION: MW2 0x10068c19
MechS32 MouseReadKeyCode(void)
{
	return 0;
}

// FUNCTION: MW2 0x10068c2b
MechS32 MouseFlushKeyCodes(void)
{
	return 0;
}

// FUNCTION: MW2 0x10068c3d
void GetClientScreenRect(RECT* p_rect, MechS32 p_width, MechS32 p_height)
{
	POINT point;

	point.x = point.y = 0;
	ClientToScreen(g_gameWindow, &point);
	p_rect->left = point.x;
	p_rect->top = point.y;

	point.x = p_width - 1;
	point.y = p_height - 1;
	ClientToScreen(g_gameWindow, &point);
	p_rect->right = point.x;
	p_rect->bottom = point.y;
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
