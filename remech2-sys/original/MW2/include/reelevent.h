#ifndef REELEVENT_H
#define REELEVENT_H

#include "types.h"

// A frame's events in an animation (Reel::m_events): flags that mark the frames an
// animation jumps between (see ReelMotionTask), and the values they compare with the player's
// animation state.
typedef struct ReelEvent {
	uint32_t m_flags;   // 0x00 — 1: a jump target (m_values[0]), 2: a loop start
	int8_t m_values[4]; // 0x04
} ReelEvent;

#endif // REELEVENT_H
