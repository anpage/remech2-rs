#ifndef REEL_H
#define REEL_H

#include "reelevent.h"
#include "types.h"

// An animation of a loaded animation file (LoadReels), in g_reels: per frame, an
// amount to move (m_amounts) and its events (m_events, after the file's animation records).
// SIZE 0x14
typedef struct Reel {
	MechS32 m_byName;     // 0x00 — the file was looked up by name (ResourceRef::m_id -1)
	MechS32 m_frameCount; // 0x04
	MechS32 m_kind;       // 0x08 — 0-2: moves along x, y or z; 3-5: turns about them
	MechS32* m_amounts;   // 0x0c
	ReelEvent* m_events;  // 0x10
} Reel;

#endif // REEL_H
