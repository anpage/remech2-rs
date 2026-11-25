#ifndef ELAPSED_H
#define ELAPSED_H

// Wall-clock time, implemented on the Rust side (src/elapsed.rs). It replaces timeGetTime and
// Sleep.

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// Milliseconds since the program started, wrapping around after 2^32 as timeGetTime did
	uint32_t MechMilliseconds(void);
	// Blocks for p_milliseconds without pumping the window
	void MechSleep(uint32_t p_milliseconds);

#ifdef __cplusplus
}
#endif

#endif // ELAPSED_H
