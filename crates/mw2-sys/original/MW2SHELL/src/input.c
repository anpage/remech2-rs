#include "input.h"

#include "decomp.h"
#include "inputdevice.h"
#include "types.h"
#include "windowstate.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

enum InputDeviceTable {
	c_deviceTableGrowth = 50
};

// A button that modifies a control's binding (INPUT.MAP writes it as "+ device.button").
// SIZE 0x0c
typedef struct InputModifier {
	MechChar* m_label;     // 0x00
	InputDevice* m_device; // 0x04
	MechS32 m_button;      // 0x08
} InputModifier;

enum InputModifierTable {
	c_modifierCount = 5
};

// A cockpit control, its INPUT.MAP name, and the device axis or button bound to it. A name
// starting with "#j " is a button that only acts while the jump jets are enabled.
// SIZE 0x29
#pragma pack(1)
typedef struct InputControl {
	MechChar* m_label;                    // 0x00
	MechChar* m_name;                     // 0x04
	MechS32 m_isButton;                   // 0x08 — 0: bound to an axis
	InputDevice* m_device;                // 0x0c
	MechS32 m_index;                      // 0x10 — the axis or button
	MechU8 m_unk0x14;                     // 0x14 — never accessed
	MechS32 m_modifiers[c_modifierCount]; // 0x15 — per modifier: 1 held, 2 ignored, else released
} InputControl;
#pragma pack()

DECOMP_SIZE_ASSERT(InputDeviceInfo, 0x74)
DECOMP_SIZE_ASSERT(InputDevice, 0x78)
DECOMP_SIZE_ASSERT(InputModifier, 0x0c)
DECOMP_SIZE_ASSERT(InputControl, 0x29)

MechS32 InputGrowDeviceTable(void);
void InputWriteAxisBinding(InputControl* p_control);
void InputWriteButtonBinding(InputControl* p_control);
void InputWriteModifiers(MechS32* p_modifiers);

// INPUT.MAP stream shared by the input-map writers.
// GLOBAL: MW2SHELL 0x10067840
FILE* g_inputMapFile = NULL;

// GLOBAL: MW2SHELL 0x10067844
MechS32 g_inputDeviceCount = 0;

// GLOBAL: MW2SHELL 0x10067848
MechS32 g_inputDeviceCapacity = 0;

// GLOBAL: MW2SHELL 0x1006784c
InputDevice* g_inputDevices = NULL;

// GLOBAL: MW2SHELL 0x10067850
InputDriverModule* g_inputDrivers[3] = {&g_keyboardDriver, &g_mouseDriver, &g_joystickDriver};

// GLOBAL: MW2SHELL 0x10067860
InputModifier g_inputModifiers[c_modifierCount] = {
	{"Modifier 1", NULL, 0},
	{"Modifier 2", NULL, 0},
	{"Modifier 3", NULL, 0},
	{"Modifier 4", NULL, 0},
	{"Modifier 5", NULL, 0},
};

#define CONTROL(label, name, isButton) {label, name, isButton, NULL, 0, 0, {0, 0, 0, 0, 0}}
// GLOBAL: MW2SHELL 0x100678a0
InputControl g_inputControls[0x35] = {
	CONTROL("Throttle", "throttle", 0),
	CONTROL("Increase", "throttle_up", 1),
	CONTROL("Decrease", "throttle_down", 1),
	CONTROL("Turn", "legs_pan_delta", 0),
	CONTROL("Turn Left", "legs_pan_minus", 1),
	CONTROL("Turn Right", "legs_pan_plus", 1),
	CONTROL("Twist", "torso_pan", 0),
	CONTROL("Twist Left", "torso_pan_minus", 1),
	CONTROL("Twist Right", "torso_pan_plus", 1),
	CONTROL("Reset", "torso_pan_reset", 1),
	CONTROL("Look Left/Right", "pilot_pan", 0),
	CONTROL("Look Left", "pilot_pan_minus", 1),
	CONTROL("Look Right", "pilot_pan_plus", 1),
	CONTROL("Reset", "pilot_pan_reset", 1),
	CONTROL("Look Up/Down", "pilot_tilt", 0),
	CONTROL("Look Up", "pilot_tilt_minus", 1),
	CONTROL("Look Down", "pilot_tilt_plus", 1),
	CONTROL("Reset", "pilot_tilt_reset", 1),
	CONTROL("Zoom", "zoom_factor", 0),
	CONTROL("Zoom In", "zoom_factor_plus", 1),
	CONTROL("Zoom Out", "zoom_factor_minus", 1),
	CONTROL("Reset", "zoom_factor_reset", 1),
	CONTROL("Enable", "jumpjet_enable", 1),
	CONTROL("Left", "jumpjet_fire_left", 1),
	CONTROL("Right", "jumpjet_fire_right", 1),
	CONTROL("Forward", "jumpjet_fire_forward", 1),
	CONTROL("Backward", "jumpjet_fire_backward", 1),
	CONTROL("Spin Left", "#j legs_pan_left", 1),
	CONTROL("Spin Right", "#j legs_pan_right", 1),
	CONTROL("Fire Single/Group", "weapon_fire", 1),
	CONTROL("Fire Group 1", "weapon_fire_group_1", 1),
	CONTROL("Fire Group 1", "weapon_fire_group_2", 1),
	CONTROL("Fire Group 1", "weapon_fire_group_3", 1),
	CONTROL("Next Weapon", "weapon_cycle", 1),
	CONTROL("Next Weapon Group", "weapon_cycle_group", 1),
	CONTROL("Toggle Single/Group", "toggle_group_fire", 1),
	CONTROL("Next Nav Point", "advance_nav", 1),
	CONTROL("Next Target", "advance_target", 1),
	CONTROL("Under Reticle", "target_reticle", 1),
	CONTROL("Nearest Enemy", "nearest_enemy", 1),
	CONTROL("Inspect", "inspect_target", 1),
	CONTROL("Distance", "track_distance_delta", 0),
	CONTROL("Move Closer", "track_distance_minus", 1),
	CONTROL("Move Away", "track_distance_plus", 1),
	CONTROL("Circle", "eyepoint_pan_delta", 0),
	CONTROL("Circle Left", "eyepoint_pan_minus", 1),
	CONTROL("Circle Right", "eyepoint_pan_plus", 1),
	CONTROL("Height", "track_height_delta", 0),
	CONTROL("Up", "track_height_plus", 1),
	CONTROL("Down", "track_height_minus", 1),
	CONTROL("Tilt", "eyepoint_tilt_delta", 0),
	CONTROL("Up", "eyepoint_tilt_plus", 1),
	CONTROL("Down", "eyepoint_tilt_minus", 1),
};
#undef CONTROL

// Enumerate the devices of every driver, once unless p_reset; returns the device count.
// Stack-slot permutation: index, device and count. The original loads g_inputDeviceCount for
// the capacity comparison; reordering the global definitions didn't flip it.
// FUNCTION: MW2SHELL 0x10031bf0
MechS32 InputEnumDevices(MechS32 p_reset)
{
	MechS32 index;
	MechS32 driver;
	MechS32 device;
	MechS32 count;

	if (g_inputDevices != NULL && !p_reset) {
		return g_inputDeviceCount;
	}

	if (p_reset) {
		g_inputDeviceCount = 0;
	}

	for (driver = 0; driver < 3; driver++) {
		if (g_inputDrivers[driver] != NULL) {
			count = g_inputDrivers[driver]->m_getDeviceCount();
			if (count) {
				for (device = 0; device < count; device++) {
					index = g_inputDeviceCount++;
					if (g_inputDeviceCount > g_inputDeviceCapacity && !InputGrowDeviceTable()) {
						return 0;
					}

					if (g_inputDrivers[driver]->m_fillDeviceInfo(device, &g_inputDevices[index].m_info)) {
						g_inputDeviceCount--;
						continue;
					}

					g_inputDevices[index].m_driver = g_inputDrivers[driver];
				}
			}
		}
	}

	return g_inputDeviceCount;
}

// Operand order: the original compares p_index < g_inputDeviceCount with g_inputDeviceCount
// loaded first (one declaration-order attempt didn't flip it).
// FUNCTION: MW2SHELL 0x10031d34
InputDevice* InputGetDevice(MechS32 p_index)
{
	if (p_index < 0 || p_index >= g_inputDeviceCount) {
		return NULL;
	}

	return &g_inputDevices[p_index];
}

// FUNCTION: MW2SHELL 0x10031d75
void InputFreeDevices(void)
{
	MechS32 i;

	for (i = 0; i < g_inputDeviceCount; i++) {
		if (g_inputDevices[i].m_driver != NULL) {
			g_inputDevices[i].m_driver->m_closeDevice(g_inputDevices[i].m_info.m_driverData);
		}
	}

	if (g_inputDevices != NULL) {
		MechHeapFree(g_primaryHeap, g_inputDevices);
	}

	g_inputDevices = NULL;
	g_inputDeviceCount = 0;
	g_inputDeviceCapacity = 0;
}

// FUNCTION: MW2SHELL 0x10031e32
MechS32 InputGetModifierCount(void)
{
	return 5;
}

// The modifier table accepts one index past its end.
// FUNCTION: MW2SHELL 0x10031e47
InputModifier* InputGetModifier(MechS32 p_index)
{
	if (p_index < 0 || p_index > c_modifierCount) {
		return NULL;
	}

	return &g_inputModifiers[p_index];
}

// FUNCTION: MW2SHELL 0x10031e7f
MechS32 InputGetControlCount(void)
{
	return 0x35;
}

// FUNCTION: MW2SHELL 0x10031e94
InputControl* InputGetControl(MechS32 p_index)
{
	if (p_index < 0 || p_index >= sizeof(g_inputControls) / sizeof(g_inputControls[0])) {
		return NULL;
	}

	return &g_inputControls[p_index];
}

// FUNCTION: MW2SHELL 0x10031ece
void InputOpenMap(void)
{
	if (g_inputMapFile != NULL) {
		return;
	}

	g_inputMapFile = fopen("INPUT.MAP", "w");
}

// Write one control's binding to INPUT.MAP.
// FUNCTION: MW2SHELL 0x10031f02
void InputWriteControl(InputControl* p_control)
{
	if (g_inputMapFile == NULL) {
		return;
	}
	if (p_control->m_device == NULL) {
		return;
	}

	if (p_control->m_name[0] == '#' && p_control->m_name[1] == 'j' && p_control->m_name[2] == ' ') {
		fprintf(g_inputMapFile, "jumpjet_enable {\n");
		InputWriteButtonBinding(p_control);
		fprintf(g_inputMapFile, "}\n");
		fprintf(g_inputMapFile, "%s {\n", p_control->m_name + 3);
		InputWriteButtonBinding(p_control);
		fprintf(g_inputMapFile, "}\n");
	}
	else {
		fprintf(g_inputMapFile, "%s {\n", p_control->m_name);
		if (!p_control->m_isButton) {
			InputWriteAxisBinding(p_control);
		}
		else {
			InputWriteButtonBinding(p_control);
		}
		fprintf(g_inputMapFile, "}\n");
	}
}

// FUNCTION: MW2SHELL 0x10032033
void InputCloseMap(void)
{
	if (g_inputMapFile == NULL) {
		return;
	}

	fclose(g_inputMapFile);
	g_inputMapFile = NULL;
}

// FUNCTION: MW2SHELL 0x10032068
MechS32 InputGrowDeviceTable(void)
{
	if (g_inputDevices == NULL) {
		g_inputDevices =
			(InputDevice*) MechHeapAlloc(g_primaryHeap, c_deviceTableGrowth * sizeof(InputDevice));
	}
	else {
		g_inputDevices = (InputDevice*) MechHeapReAlloc(
			g_primaryHeap,
			g_inputDevices,
			(g_inputDeviceCapacity + c_deviceTableGrowth) * sizeof(InputDevice)
		);
	}

	if (g_inputDevices == NULL) {
		return 0;
	}

	memset(&g_inputDevices[g_inputDeviceCapacity], 0, c_deviceTableGrowth * sizeof(InputDevice));
	g_inputDeviceCapacity += c_deviceTableGrowth;
	return g_inputDeviceCapacity;
}

// Write an axis binding and its modifiers.
// FUNCTION: MW2SHELL 0x1003210d
void InputWriteAxisBinding(InputControl* p_control)
{
	InputDevice* device;

	device = p_control->m_device;
	fprintf(
		g_inputMapFile,
		"\t+ %s\t%s\n",
		device->m_info.m_shortName,
		device->m_info.m_axisShortNames[p_control->m_index]
	);
	InputWriteModifiers(p_control->m_modifiers);
}

// Write a button binding and its modifiers.
// FUNCTION: MW2SHELL 0x1003215d
void InputWriteButtonBinding(InputControl* p_control)
{
	InputDevice* device;

	device = p_control->m_device;
	fprintf(
		g_inputMapFile,
		"\t+ %s\t%s\n",
		device->m_info.m_shortName,
		device->m_info.m_buttonShortNames[p_control->m_index]
	);
	InputWriteModifiers(p_control->m_modifiers);
}

// Stack-slot permutation: i and device.
// FUNCTION: MW2SHELL 0x100321ad
void InputWriteModifiers(MechS32* p_modifiers)
{
	MechS32 i;
	InputDevice* device;
	MechS32 button;

	for (i = 0; i < c_modifierCount; i++) {
		device = g_inputModifiers[i].m_device;
		button = g_inputModifiers[i].m_button;
		if (p_modifiers[i] != 2 && device != NULL) {
			fprintf(
				g_inputMapFile,
				"\t%c %s\t%s\n",
				p_modifiers[i] == 1 ? '+' : '-',
				device->m_info.m_shortName,
				device->m_info.m_buttonShortNames[button]
			);
		}
	}
}
