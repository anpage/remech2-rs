#ifndef TICKS_H
#define TICKS_H

#include "types.h"

// The tick counters, implemented on the Rust side (src/sim/ticks.rs). There are two, counting at
// the rate of the 181 Hz Miles timer whose callback incremented the original's (ticks.asm): the
// first can be paused with the game, the second runs on. A handle counts one of them from where
// it was allocated, reset or set.
#ifdef __cplusplus
extern "C"
{
#endif

	// Start and stop both counters, where the original registered and released its timer
	void StartTicks(void);
	void StopTicks(void);

	// p_flags picks the counter: 0x80 the first, 0x100 the second. Returns -1 when out of handles.
	MechS16 AllocTicks(MechU32 p_flags);
	MechS32 GetTicks(MechU32 p_handle);
	void ResetTicks(MechU32 p_handle);
	void SetTicks(MechU32 p_handle, MechS32 p_ticks);
	void FreeTicks(MechU32 p_handle);
	// Pauses or resumes the counters p_flags picks, as for AllocTicks
	void PauseTimer(MechS32 p_flags, MechS32 p_paused);

#ifdef __cplusplus
}
#endif

#endif // TICKS_H
