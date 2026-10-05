#ifndef INPUTDRIVER_H
#define INPUTDRIVER_H

#include "decomp.h"
#include "inputdeviceinfo.h"
#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// The entry points of one input device class (keyboard, mouse, joystick), called by the
	// device enumeration through a table of these.
	// SIZE 0x20
	typedef struct InputDriverModule {
		MechS32 (*m_getDeviceCount)(void);                                         // 0x00
		MechS32 (*m_fillDeviceInfo)(MechS32 p_index, InputDeviceInfo* p_info);     // 0x04
		MechS32 (*m_openDevice)(InputDeviceInfo* p_info);                          // 0x08
		MechS32 (*m_closeDevice)(InputDeviceInfo* p_info);                         // 0x0c
		MechS32 (*m_centerAxis)(void* p_data, MechS32 p_axis);                     // 0x10
		MechS32 (*m_poll)(void* p_data, MechS32* p_axes, MechU32* p_buttons);      // 0x14 — axes and buttons
		MechS32 (*m_readKeyCode)(MechS16* p_keyCode);                              // 0x18
		MechS32 (*m_flushKeyCodes)(void);                                          // 0x1c
	} InputDriverModule;

	extern InputDriverModule g_keyboardDriver;
	extern InputDriverModule g_mouseDriver;
	extern InputDriverModule g_joystickDriver;

#ifdef __cplusplus
}
#endif

#endif // INPUTDRIVER_H
