#ifndef REEL_H
#define REEL_H

#include "reelevent.h"
#include "types.h"

// An animation of a loaded animation file (LoadReels), in g_reels: per frame, an
// amount to move (m_amounts) and its events (m_events, after the file's animation records).
typedef struct Reel {
	int32_t m_byName; // the file was looked up by name (ResourceRef::m_id -1)
	int32_t m_frameCount;
	int32_t m_kind; // 0-2: moves along x, y or z; 3-5: turns about them
	int32_t* m_amounts;
	ReelEvent* m_events;
} Reel;

#endif // REEL_H
