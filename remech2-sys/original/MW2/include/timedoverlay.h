#ifndef TIMEDOVERLAY_H
#define TIMEDOVERLAY_H

#include "point.h"
#include "targeting.h"
#include "types.h"

// One of the two in-game message boxes: ShowInGameMessage fills the one it may replace,
// DrawTimedOverlays shows it on its background shape until m_expireTime.
typedef struct TimedOverlay {
	char* m_text; // a 0x100-byte buffer
	Point m_textPos;
	int32_t m_active;
	int32_t m_priority;
	int32_t m_font;       // FONT resource
	int32_t m_background; // SHP resource
	int32_t m_expireTime;
	PANE* m_target;
} TimedOverlay;

#endif // TIMEDOVERLAY_H
