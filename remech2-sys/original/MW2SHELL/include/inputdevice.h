#ifndef INPUTDEVICE_H
#define INPUTDEVICE_H

#include "decomp.h"
#include "inputdeviceinfo.h"
#include "inputdriver.h"
#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// One enumerated input device: the driver that owns it and what the driver reported.
	// SIZE 0x78
	typedef struct InputDevice {
		InputDriverModule* m_driver; // 0x00
		InputDeviceInfo m_info;      // 0x04
	} InputDevice;

	MechS32 InputEnumDevices(MechS32 p_reset);
	InputDevice* InputGetDevice(MechS32 p_index);

#ifdef __cplusplus
}
#endif

#endif // INPUTDEVICE_H
