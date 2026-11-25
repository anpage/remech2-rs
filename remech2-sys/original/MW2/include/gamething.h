#ifndef GAMETHING_H
#define GAMETHING_H

#include "types.h"

typedef struct GameThing {
	int16_t m_flags;               // 0x00 — 4: destroyed
	int16_t m_teamsReached;        // 0x02 — a bit per team that reached it
	int32_t m_staticObject;        // 0x04 — an index into g_staticObjects
	int32_t m_hitPoints;           // 0x08 — destroyed when they run out
	int32_t m_affiliation;         // 0x0c
	int32_t m_radius;              // 0x10 — its shape's radius (AfterWorldLoader)
	char m_name[0x2a - 0x14];      // 0x14
	char m_shortName[0x40 - 0x2a]; // 0x2a — for the target panel
} GameThing;

#endif // GAMETHING_H
