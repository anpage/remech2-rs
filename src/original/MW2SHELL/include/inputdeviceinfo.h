#ifndef INPUTDEVICEINFO_H
#define INPUTDEVICEINFO_H

#include "decomp.h"
#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// What a driver reports about one of its devices: its names and its axis and button names.
	// SIZE 0x74
	typedef struct InputDeviceInfo {
		MechChar m_shortName[0x0c];    // 0x00 — also the device name in INPUT.MAP
		MechChar m_displayName[0x40];  // 0x0c
		MechChar m_matchName[0x0c];    // 0x4c
		MechS32 m_axisCount;           // 0x58
		MechS32 m_buttonCount;         // 0x5c
		MechChar** m_axisNames;        // 0x60
		MechChar** m_axisShortNames;   // 0x64
		MechChar** m_buttonNames;      // 0x68
		MechChar** m_buttonShortNames; // 0x6c
		void* m_driverData;            // 0x70
	} InputDeviceInfo;

#ifdef __cplusplus
}
#endif

#endif // INPUTDEVICEINFO_H
