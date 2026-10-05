#ifndef INPUTDEVICESTATE_H
#define INPUTDEVICESTATE_H

#include "types.h"

// What a device's poll reports: its axes and its buttons, a bit each.
// SIZE 0x50
typedef struct InputDeviceState {
	MechS32 m_axes[16];   // 0x00
	MechU32 m_buttons[4]; // 0x40
} InputDeviceState;

#endif // INPUTDEVICESTATE_H
