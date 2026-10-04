#include "joystick.h"

#include "inputdeviceinfo.h"
#include "inputdriver.h"
#include "types.h"

// The joystick input driver. The original was built on the multimedia joystick API (joyGetPosEx),
// taking each device's name from the registry.
// TODO: joysticks aren't supported for now: the driver reports no devices. Its entry points are
// kept as the place to bring them back.

MechS32 GetJoystickDeviceCount(void);
MechS32 FillJoystickDeviceInfo(MechS32 p_index, InputDeviceInfo* p_info);
MechS32 JoystickOpenDevice(InputDeviceInfo* p_info);
MechS32 JoystickCloseDevice(InputDeviceInfo* p_info);
MechS32 JoystickCenterAxis(void);
MechS32 JoystickPoll(void* p_data, MechS32* p_axes, MechU32* p_buttons);
MechS32 JoystickReadKeyCode(void);
MechS32 JoystickFlushKeyCodes(void);

// GLOBAL: MW2 0x100a6f58
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

// FUNCTION: MW2 0x10049e70
MechS32 GetJoystickDeviceCount(void)
{
	return 0;
}

// Fills p_info for joystick p_index: its names, its driver data and the names of its axes,
// buttons and POV directions. Returns 1 on failure.
// FUNCTION: MW2 0x10049e86
MechS32 FillJoystickDeviceInfo(MechS32 p_index, InputDeviceInfo* p_info)
{
	return 1;
}

// Returns 1 on failure.
// FUNCTION: MW2 0x1004a3be
MechS32 JoystickOpenDevice(InputDeviceInfo* p_info)
{
	return 1;
}

// Frees what FillJoystickDeviceInfo and JoystickOpenDevice allocated.
// FUNCTION: MW2 0x1004a947
MechS32 JoystickCloseDevice(InputDeviceInfo* p_info)
{
	return 0;
}

// FUNCTION: MW2 0x1004aa59
MechS32 JoystickCenterAxis(void)
{
	return 0;
}

// Reads the joystick into the six axes (Y and X first, then Z, R, U and V) and the two button
// words, the POV hat setting the buttons its directions map to. Returns 1 without somewhere to
// put them, else 0.
// FUNCTION: MW2 0x1004aa6b
MechS32 JoystickPoll(void* p_data, MechS32* p_axes, MechU32* p_buttons)
{
	return 1;
}

// FUNCTION: MW2 0x1004ad42
MechS32 JoystickReadKeyCode(void)
{
	return 2;
}

// FUNCTION: MW2 0x1004ad57
MechS32 JoystickFlushKeyCodes(void)
{
	return 2;
}
