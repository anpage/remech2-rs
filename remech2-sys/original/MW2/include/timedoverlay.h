#ifndef TIMEDOVERLAY_H
#define TIMEDOVERLAY_H

#include "point.h"
#include "targeting.h"
#include "types.h"

// One of the two in-game message boxes: ShowInGameMessage fills the one it may replace,
// DrawTimedOverlays shows it on its background shape until m_expireTime.
typedef struct TimedOverlay {
	char* m_text;         // 0x00 — a 0x100-byte buffer
	Point m_textPos;      // 0x04
	int32_t m_active;     // 0x0c
	int32_t m_priority;   // 0x10
	int32_t m_font;       // 0x14 — FONT resource
	int32_t m_background; // 0x18 — SHP resource
	int32_t m_expireTime; // 0x1c
	PANE* m_target;       // 0x20
} TimedOverlay;

#endif // TIMEDOVERLAY_H
