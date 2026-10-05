#ifndef INPUT_H
#define INPUT_H

#include "inputdevice.h"
#include "types.h"

#include <stdio.h>

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

// The functions and globals of input.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern FILE* g_inputMapFile;
	extern MechS32 g_inputDeviceCount;
	extern MechS32 g_inputDeviceCapacity;
	extern InputDevice* g_inputDevices;
	extern InputDriverModule* g_inputDrivers[3];
	extern InputModifier g_inputModifiers[c_modifierCount];
	extern InputControl g_inputControls[0x35];

	void InputFreeDevices(void);

#ifdef __cplusplus
}
#endif

#endif // INPUT_H
