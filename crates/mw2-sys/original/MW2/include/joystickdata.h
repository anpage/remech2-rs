#ifndef JOYSTICKDATA_H
#define JOYSTICKDATA_H

#include "types.h"

#include <windows.h>

// The joystick driver's data for an open device (InputDeviceInfo::m_driverData), allocated by
// FillJoystickDeviceInfo.
// SIZE 0x88
typedef struct JoystickData {
	UINT m_id;               // 0x00 — the multimedia joystick id
	DWORD m_flags;           // 0x04 — the JOYINFOEX flags to read with (JOY_RETURNZ ... JOY_RETURNPOV)
	MechS32 m_centers[6];    // 0x08 — of the X, Y, Z, R, U and V axes
	MechS32 m_deadZones[6];  // 0x20
	MechDouble m_scales[6];  // 0x38
	MechS32 m_povButtons[4]; // 0x68 — the button word each POV direction sets a bit in: forward, right,
							 // backward, left
	MechU32 m_povMasks[4];   // 0x78
} JoystickData;

#endif // JOYSTICKDATA_H
