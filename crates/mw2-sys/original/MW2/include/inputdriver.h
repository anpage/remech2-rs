#ifndef INPUTDRIVER_H
#define INPUTDRIVER_H

#include "decomp.h"
#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// The entry points of one input device class (keyboard, mouse, joystick), called by the
	// device enumeration through a table of these.
	// SIZE 0x20
	typedef struct InputDriverModule {
		MechS32 (*m_getDeviceCount)(); // 0x00
		MechS32 (*m_fillDeviceInfo)(); // 0x04
		MechS32 (*m_openDevice)();     // 0x08
		MechS32 (*m_closeDevice)();    // 0x0c
		MechS32 (*m_centerAxis)();     // 0x10
		MechS32 (*m_poll)();           // 0x14 — axes and buttons
		MechS32 (*m_readKeyCode)();    // 0x18
		MechS32 (*m_flushKeyCodes)();  // 0x1c
	} InputDriverModule;

#ifdef __cplusplus
}
#endif

#endif // INPUTDRIVER_H
