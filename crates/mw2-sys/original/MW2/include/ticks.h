#ifndef TICKS_H
#define TICKS_H

#include "types.h"

// The tick counters of ticks.asm (ticks.c in COMPAT_MODE).
#ifdef __cplusplus
extern "C"
{
#endif

	void GameTickTimerCallback(void);
	MechS16 AllocTicks(MechU32 p_flags);
	MechS32 GetTicks(MechU32 p_handle);
	void ResetTicks(MechU32 p_handle);
	void SetTicks(MechU32 p_handle, MechS32 p_ticks);
	void FreeTicks(MechU32 p_handle);
	void PauseTimer(MechS32 p_flags, MechS32 p_paused);

#ifdef __cplusplus
}
#endif

// ticks.asm's routines and data. reccmp reads annotations from C sources only, so they're here, by
// name.

// FUNCTION: MW2 0x10067ed8
// GameTickTimerCallback

// FUNCTION: MW2 0x10067f01
// AllocTicks

// FUNCTION: MW2 0x10067f6f
// GetTicks

// FUNCTION: MW2 0x10067faa
// ResetTicks

// FUNCTION: MW2 0x10067fe5
// SetTicks

// FUNCTION: MW2 0x10068023
// FreeTicks

// FUNCTION: MW2 0x10068058
// PauseTimer

// GLOBAL: MW2 0x100ad008
// g_ticksPaused

// GLOBAL: MW2 0x100ad00c
// g_ticks1Bases

// GLOBAL: MW2 0x100ad10c
// g_ticks2Bases

// GLOBAL: MW2 0x100ad20c
// g_ticks1

// GLOBAL: MW2 0x100ad210
// g_ticks2

#endif // TICKS_H
