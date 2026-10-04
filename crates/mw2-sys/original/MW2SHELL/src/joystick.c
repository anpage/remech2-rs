/* The joystick input driver (g_joystickDriver). The original was built on the multimedia joystick
   API (joyGetPosEx), taking each device's name from the registry.
   TODO: joysticks aren't supported for now: the driver reports no devices. Its entry points are
   kept as the place to bring them back. */
#include "inputdeviceinfo.h"
#include "inputdriver.h"
#include "types.h"

// FUNCTION: MW2SHELL 0x1003ad20
MechS32 GetJoystickDeviceCount()
{
	return 0;
}

// Fills in joystick p_index: its names, its axes and its buttons and POV hat. Returns 0, or 1 on
// failure.
// FUNCTION: MW2SHELL 0x1003ad36
MechS32 FillJoystickDeviceInfo(MechS32 p_index, InputDeviceInfo* p_info)
{
	return 1;
}

// Returns 1 on failure.
// FUNCTION: MW2SHELL 0x1003b246
MechS32 JoystickOpenDevice(InputDeviceInfo* p_info)
{
	return 1;
}

// Frees what FillJoystickDeviceInfo and JoystickOpenDevice allocated.
// FUNCTION: MW2SHELL 0x1003b7cf
MechS32 JoystickCloseDevice(InputDeviceInfo* p_device)
{
	return 0;
}

// FUNCTION: MW2SHELL 0x1003b8b7
MechS32 JoystickCenterAxis()
{
	return 0;
}

// Reads the joystick into the six axes and the two button words, the POV hat setting the buttons
// its directions map to. Returns 1 without somewhere to put them, else 0.
// FUNCTION: MW2SHELL 0x1003b8c9
MechS32 JoystickPoll(void* p_data, MechS32* p_axes, MechU32* p_buttons)
{
	return 1;
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
