/* The joystick input driver (g_joystickDriver): joystick devices through the multimedia
   joystick API. A C unit: JoystickSetMatchName, a void function, ends without the C++ front end's jmp
   to the epilogue. It starts on the 16-byte boundary right after MouseState's code. */
#include "debugprint.h"
#include "decomp.h"
#include "inputdeviceinfo.h"
#include "inputdriver.h"
#include "types.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// GLOBAL: MW2SHELL 0x10090528
static MechChar g_joystickOemName[0x40];

// GLOBAL: MW2SHELL 0x10090568
static MechChar g_joystickRegistryKey[0x100];

// SIZE 0x88
// The joystick driver's data for one device (InputDeviceInfo::m_driverData).
typedef struct JoystickData {
	UINT m_id;              // 0x00 — joystick id for joyGetPosEx
	DWORD m_flags;          // 0x04 — JOY_RETURN* flags
	MechS32 m_center[6];    // 0x08 — per axis: x, y, z, r, u, v
	MechS32 m_deadZone[6];  // 0x20
	MechDouble m_scale[6];  // 0x38
	MechS32 m_povButton[4]; // 0x68 — the button word each POV direction sets...
	MechU32 m_povMask[4];   // 0x78 — ...and its bits
} JoystickData;

DECOMP_SIZE_ASSERT(JoystickData, 0x88)

// FUNCTION: MW2SHELL 0x1003ad20
MechS32 GetJoystickDeviceCount()
{
	return joyGetNumDevs();
}

BOOL GetJoystickRegistryName(MechS32 p_index, const MechChar* p_driver, MechChar* p_name, size_t p_size);
void JoystickSetMatchName(InputDeviceInfo* p_info, JOYCAPS* p_caps);
MechS32 JoystickCloseDevice(InputDeviceInfo* p_device);

// Fills in joystick p_index: its names, its axes (a tracker's third axis is its head roll) and
// its buttons and POV hat. Returns 0, or 1 on failure.
// Not 100%: the stack slots of the locals are permuted.
// FUNCTION: MW2SHELL 0x1003ad36
MechS32 FillJoystickDeviceInfo(MechS32 p_index, InputDeviceInfo* p_info)
{
	JOYCAPS caps;
	MechChar** axisShortNames;
	MechChar** buttonNames;
	MechS32 count;
	MechChar** axisNames;
	MechChar** buttonShortNames;
	JoystickData* data;

	if (joyGetDevCaps(p_index, &caps, sizeof(caps))) {
		return 1;
	}

	p_info->m_axisNames = axisNames = (MechChar**) calloc(6, sizeof(MechChar*));
	p_info->m_axisShortNames = axisShortNames = (MechChar**) calloc(6, sizeof(MechChar*));
	p_info->m_buttonNames = buttonNames = (MechChar**) calloc(0x24, sizeof(MechChar*));
	p_info->m_buttonShortNames = buttonShortNames = (MechChar**) calloc(0x24, sizeof(MechChar*));
	if (!axisNames || !axisShortNames || !buttonNames || !buttonShortNames) {
		JoystickCloseDevice(p_info);
		return 1;
	}

	data = (JoystickData*) calloc(1, sizeof(JoystickData));
	if (!data) {
		return 1;
	}
	else {
		data->m_id = p_index;
		p_info->m_driverData = data;
	}

	if (!GetJoystickRegistryName(p_index, caps.szRegKey, p_info->m_displayName, sizeof(p_info->m_displayName))) {
		sprintf(p_info->m_displayName, "Joystick %d", p_index + 1);
	}
	sprintf(p_info->m_shortName, "joystick%d", p_index + 1);
	JoystickSetMatchName(p_info, &caps);

	axisNames[0] = "Down/Up Movement";
	axisShortNames[0] = "Down/Up";
	axisNames[1] = "Left/Right Movement";
	axisShortNames[1] = "Left/Right";
	count = 2;
	if (caps.wCaps & JOYCAPS_HASZ) {
		if (!memcmp(p_info->m_matchName, "tracker", 8)) {
			axisNames[count] = "Left/Right Head Roll";
			axisShortNames[count] = "HeadRoll";
			count++;
		}
		else {
			axisNames[count] = "Throttle Control";
			axisShortNames[count] = "Throttle";
			count++;
		}
	}
	if (caps.wCaps & JOYCAPS_HASR) {
		axisNames[count] = "Rudder Movement";
		axisShortNames[count] = "Rudder";
		count++;
	}
	if (caps.wCaps & JOYCAPS_HASU) {
		axisNames[count] = "5th axis Movement";
		axisShortNames[count] = "5thAxis";
		count++;
	}
	if (caps.wCaps & JOYCAPS_HASV) {
		axisNames[count] = "6th axis Movement";
		axisShortNames[count] = "6thAxis";
		count++;
	}
	p_info->m_axisCount = count;

	buttonNames[0] = (MechChar*) calloc(0x20, 0xc);
	buttonShortNames[0] = (MechChar*) calloc(0x20, 0xc);
	if (!buttonNames[0] || !buttonShortNames[0]) {
		JoystickCloseDevice(p_info);
		return 1;
	}

	for (count = 0; count < (MechS32) caps.wNumButtons; count++) {
		sprintf(buttonNames[count], "Button %d", count + 1);
		sprintf(buttonShortNames[count], "Button%d", count + 1);
		buttonNames[count + 1] = buttonNames[count] + 0xc;
		buttonShortNames[count + 1] = buttonShortNames[count] + 0xc;
	}

	count = caps.wNumButtons;
	if (caps.wCaps & JOYCAPS_HASPOV) {
		buttonNames[count] = "Hat Up";
		buttonShortNames[count] = "HatUp";
		count++;
		buttonNames[count] = "Hat Right";
		buttonShortNames[count] = "HatRight";
		count++;
		buttonNames[count] = "Hat Down";
		buttonShortNames[count] = "HatDown";
		count++;
		buttonNames[count] = "Hat Left";
		buttonShortNames[count] = "HatLeft";
		count++;
	}
	p_info->m_buttonCount = count;

	return 0;
}

// Reads joystick p_info's capabilities: the axes it reports, their centers, dead zones and
// scales, and the button words and bits of the POV hat's four directions. Returns 0, or 1
// when the joystick is missing or unplugged.
// Not 100%: the stack slots of the locals are permuted (which also changes their encodings).
// FUNCTION: MW2SHELL 0x1003b246
MechS32 JoystickOpenDevice(InputDeviceInfo* p_info)
{
	JOYINFOEX infoEx;
	JOYCAPS caps;
	MechDouble range;
	JOYINFO info;
	MechS32 button;
	JoystickData* data;

	data = (JoystickData*) p_info->m_driverData;
	if (!data) {
		return 1;
	}

	if (joyGetDevCaps(data->m_id, &caps, sizeof(caps))) {
		return 1;
	}

	if (caps.wNumAxes <= 3 && caps.wMaxButtons <= 4) {
		if (joyGetPos(data->m_id, &info) == JOYERR_UNPLUGGED) {
			return 1;
		}
	}
	else {
		memset(&infoEx, 0, sizeof(infoEx));
		infoEx.dwSize = sizeof(infoEx);
		if (joyGetPosEx(data->m_id, &infoEx) == JOYERR_UNPLUGGED) {
			return 1;
		}
	}

	data->m_flags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNBUTTONS | JOY_RETURNCENTERED;
	data->m_center[0] = caps.wXmin + (MechS32) ((range = caps.wXmax - caps.wXmin + 1) / 2.0);
	data->m_deadZone[0] = (MechS32) (range / 16.0);
	data->m_scale[0] = 131072.0 / range;
	data->m_center[1] = caps.wYmin + (MechS32) ((range = caps.wYmax - caps.wYmin + 1) / 2.0);
	data->m_deadZone[1] = (MechS32) (range / 16.0);
	data->m_scale[1] = 131072.0 / range;
	if (caps.wCaps & JOYCAPS_HASZ) {
		data->m_flags |= JOY_RETURNZ;
		data->m_center[2] = caps.wZmin + (MechS32) ((range = caps.wZmax - caps.wZmin + 1) / 2.0);
		data->m_deadZone[2] = (MechS32) (range / 16.0);
		data->m_scale[2] = 131072.0 / range;
	}
	if (caps.wCaps & JOYCAPS_HASR) {
		data->m_flags |= JOY_RETURNR;
		data->m_center[3] = caps.wRmin + (MechS32) ((range = caps.wRmax - caps.wRmin + 1) / 2.0);
		data->m_deadZone[3] = (MechS32) (range / 8.0);
		data->m_scale[3] = 131072.0 / range;
	}
	if (caps.wCaps & JOYCAPS_HASU) {
		data->m_flags |= JOY_RETURNU;
		data->m_center[4] = caps.wUmin + (MechS32) ((range = caps.wUmax - caps.wUmin + 1) / 2.0);
		data->m_deadZone[4] = (MechS32) (range / 16.0);
		data->m_scale[4] = 131072.0 / range;
	}
	if (caps.wCaps & JOYCAPS_HASV) {
		data->m_flags |= JOY_RETURNV;
		data->m_center[5] = caps.wVmin + (MechS32) ((range = caps.wVmax - caps.wVmin + 1) / 2.0);
		data->m_deadZone[5] = (MechS32) (range / 16.0);
		data->m_scale[5] = 131072.0 / range;
	}
	if (caps.wCaps & JOYCAPS_HASPOV) {
		data->m_flags |= JOY_RETURNPOV;
		button = caps.wNumButtons;
		if (button >= 32) {
			data->m_povButton[0] = 1;
		}
		else {
			data->m_povButton[0] = 0;
		}
		data->m_povMask[0] = 1 << button % 32;
		button++;
		if (button >= 32) {
			data->m_povButton[1] = 1;
		}
		else {
			data->m_povButton[1] = 0;
		}
		data->m_povMask[1] = 1 << button % 32;
		button++;
		if (button >= 32) {
			data->m_povButton[2] = 1;
		}
		else {
			data->m_povButton[2] = 0;
		}
		data->m_povMask[2] = 1 << button % 32;
		button++;
		if (button >= 32) {
			data->m_povButton[3] = 1;
		}
		else {
			data->m_povButton[3] = 0;
		}
		data->m_povMask[3] = 1 << button % 32;
	}

	return 0;
}

// FUNCTION: MW2SHELL 0x1003b7cf
MechS32 JoystickCloseDevice(InputDeviceInfo* p_device)
{
	if (p_device != NULL) {
		if (p_device->m_axisNames != NULL) {
			free(p_device->m_axisNames);
		}
		if (p_device->m_axisShortNames != NULL) {
			free(p_device->m_axisShortNames);
		}
		if (p_device->m_buttonNames != NULL) {
			if (p_device->m_buttonNames[0] != NULL) {
				free(p_device->m_buttonNames[0]);
			}
			free(p_device->m_buttonNames);
		}
		if (p_device->m_buttonShortNames != NULL) {
			if (p_device->m_buttonShortNames[0] != NULL) {
				free(p_device->m_buttonShortNames[0]);
			}
			free(p_device->m_buttonShortNames);
		}
		if (p_device->m_driverData != NULL) {
			free(p_device->m_driverData);
		}
	}

	return 0;
}

// FUNCTION: MW2SHELL 0x1003b8b7
MechS32 JoystickCenterAxis()
{
	return 0;
}

MechS32 JoystickScaleAxis(MechS32 p_value, MechS32 p_deadZone, MechS32 p_center, MechDouble p_scale);

// Polls a joystick: its six axes into p_axes (y and x first, then the ones it has) and its
// buttons, with the POV hat's directions mapped to buttons, into p_buttons.
// Not 100%: the stack slots of info and i are permuted.
// FUNCTION: MW2SHELL 0x1003b8c9
MechS32 JoystickPoll(JoystickData* p_data, MechS32* p_axes, MechU32* p_buttons)
{
	JOYINFOEX info;
	MechS32 i;

	memset(&info, 0, sizeof(info));
	info.dwSize = sizeof(info);

	if (!p_buttons || !p_axes) {
		return 1;
	}

	for (i = 0; i < 6; i++) {
		p_axes[i] = 0;
	}
	p_buttons[0] = p_buttons[1] = 0;

	if (!g_windowActive) {
		return 0;
	}

	info.dwFlags = p_data->m_flags;
	if (joyGetPosEx(p_data->m_id, &info) == JOYERR_NOERROR) {
		p_buttons[0] = info.dwButtons;

		if (p_data->m_flags & JOY_RETURNPOV) {
			switch (info.dwPOV) {
			case JOY_POVFORWARD:
				p_buttons[p_data->m_povButton[0]] |= p_data->m_povMask[0];
				break;
			case JOY_POVRIGHT:
				p_buttons[p_data->m_povButton[1]] |= p_data->m_povMask[1];
				break;
			case JOY_POVBACKWARD:
				p_buttons[p_data->m_povButton[2]] |= p_data->m_povMask[2];
				break;
			case JOY_POVLEFT:
				p_buttons[p_data->m_povButton[3]] |= p_data->m_povMask[3];
				break;
			}
		}

		p_axes[0] = JoystickScaleAxis(info.dwYpos, p_data->m_deadZone[1], p_data->m_center[1], p_data->m_scale[1]);
		p_axes[1] = JoystickScaleAxis(info.dwXpos, p_data->m_deadZone[0], p_data->m_center[0], p_data->m_scale[0]);
		i = 2;

		if (p_data->m_flags & JOY_RETURNZ) {
			p_axes[i] = JoystickScaleAxis(info.dwZpos, p_data->m_deadZone[2], p_data->m_center[2], p_data->m_scale[2]);
			i++;
		}
		if (p_data->m_flags & JOY_RETURNR) {
			p_axes[i] = JoystickScaleAxis(info.dwRpos, p_data->m_deadZone[3], p_data->m_center[3], p_data->m_scale[3]);
			i++;
		}
		if (p_data->m_flags & JOY_RETURNU) {
			p_axes[i] = JoystickScaleAxis(info.dwUpos, p_data->m_deadZone[4], p_data->m_center[4], p_data->m_scale[4]);
			i++;
		}
		if (p_data->m_flags & JOY_RETURNV) {
			p_axes[i] = JoystickScaleAxis(info.dwVpos, p_data->m_deadZone[5], p_data->m_center[5], p_data->m_scale[5]);
			i++;
		}
	}

	return 0;
}

// FUNCTION: MW2SHELL 0x1003bba0
MechS32 JoystickReadKeyCode()
{
	return 2;
}

// FUNCTION: MW2SHELL 0x1003bbb5
MechS32 JoystickFlushKeyCodes()
{
	return 2;
}

// Maps an axis reading to -0x10000..0x10000 around p_center, with a dead zone of p_deadZone.
// FUNCTION: MW2SHELL 0x1003bbca
MechS32 JoystickScaleAxis(MechS32 p_value, MechS32 p_deadZone, MechS32 p_center, MechDouble p_scale)
{
	if ((p_value -= p_center) < 0) {
		if (-p_deadZone > p_value) {
			p_value += p_deadZone;
			p_value = (MechS32) (p_value * p_scale);
			if (p_value < -0x10000) {
				p_value = -0x10000;
			}
		}
		else {
			p_value = 0;
		}
	}
	else if (p_deadZone < p_value) {
		p_value -= p_deadZone;
		p_value = (MechS32) (p_value * p_scale);
		if (p_value > 0x10000) {
			p_value = 0x10000;
		}
	}
	else {
		p_value = 0;
	}

	return p_value;
}

// Looks up the OEM name of joystick p_index (0-based) of the driver p_driver in the registry.
// Not 100%: the stack slots of result, size, type and key are permuted.
// FUNCTION: MW2SHELL 0x1003bc87
BOOL GetJoystickRegistryName(MechS32 p_index, const MechChar* p_driver, MechChar* p_name, size_t p_size)
{
	LONG result;
	DWORD size;
	DWORD type;
	HKEY key;

	sprintf(
		g_joystickRegistryKey,
		"System\\CurrentControlSet\\Control\\MediaResources\\Joystick\\%s\\CurrentJoystickSettings",
		p_driver
	);
	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_joystickRegistryKey, 0, KEY_QUERY_VALUE, &key);
	if (result != ERROR_SUCCESS) {
		DebugPrint("Could not open registry joystick current config: %d\n", result);
		return FALSE;
	}

	sprintf(g_joystickRegistryKey, "Joystick%dOEMName", p_index + 1);
	size = sizeof(g_joystickOemName);
	type = REG_SZ;
	result = RegQueryValueEx(key, g_joystickRegistryKey, NULL, &type, (LPBYTE) g_joystickOemName, &size);
	if (result == ERROR_SUCCESS) {
		RegCloseKey(key);
		sprintf(
			g_joystickRegistryKey,
			"System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\OEM\\%s",
			g_joystickOemName
		);
		result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_joystickRegistryKey, 0, KEY_QUERY_VALUE, &key);
		if (result == ERROR_SUCCESS) {
			size = sizeof(g_joystickOemName);
			result = RegQueryValueEx(key, "OEMName", NULL, &type, (LPBYTE) g_joystickOemName, &size);
			if (result == ERROR_SUCCESS) {
				strncpy(p_name, g_joystickOemName, p_size);
			}
		}
	}
	RegCloseKey(key);

	return result == ERROR_SUCCESS ? TRUE : FALSE;
}

// Picks the name INPUT.MAP matches the joystick by, from its OEM name or failing that from
// its capabilities.
// FUNCTION: MW2SHELL 0x1003bdea
void JoystickSetMatchName(InputDeviceInfo* p_info, JOYCAPS* p_caps)
{
	MechU16 caps;

	caps = p_caps->wCaps;
	if (strstr(p_info->m_displayName, "Tracker")) {
		memcpy(p_info->m_matchName, "tracker", sizeof("tracker"));
	}
	else if (
		strstr(p_info->m_displayName, "SideWinder") ||
		((caps & JOYCAPS_HASZ) && (caps & JOYCAPS_HASR) && (caps & JOYCAPS_HASPOV) && (caps & JOYCAPS_POV4DIR) &&
		 p_caps->wNumButtons == 8)
	) {
		memcpy(p_info->m_matchName, "sidewind", sizeof("sidewind"));
	}
	else if (
		strstr(p_info->m_displayName, "Flightstick Pro") ||
		((caps & JOYCAPS_HASZ) && (caps & JOYCAPS_HASPOV) && (caps & JOYCAPS_POV4DIR) && p_caps->wNumButtons == 4)
	) {
		memcpy(p_info->m_matchName, "fltstick", sizeof("fltstick"));
	}
	else if (
		strstr(p_info->m_displayName, "Thrustmaster") ||
		((caps & JOYCAPS_HASR) && (caps & JOYCAPS_HASPOV) && (caps & JOYCAPS_POV4DIR) && p_caps->wNumButtons == 4)
	) {
		memcpy(p_info->m_matchName, "tmaster", sizeof("tmaster"));
	}
	else {
		memcpy(p_info->m_matchName, "joystick", sizeof("joystick"));
	}
}

// GLOBAL: MW2SHELL 0x1006a800
InputDriverModule g_joystickDriver = {
	GetJoystickDeviceCount,
	(MechS32 (*)()) FillJoystickDeviceInfo,
	(MechS32 (*)()) JoystickOpenDevice,
	(MechS32 (*)()) JoystickCloseDevice,
	JoystickCenterAxis,
	(MechS32 (*)()) JoystickPoll,
	JoystickReadKeyCode,
	JoystickFlushKeyCodes,
};
