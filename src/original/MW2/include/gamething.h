#ifndef GAMETHING_H
#define GAMETHING_H

#include "decomp.h"
#include "types.h"

// SIZE 0x40
typedef struct GameThing {
	MechS16 m_flags;                   // 0x00 — 4: destroyed
	MechS16 m_teamsReached;            // 0x02 — a bit per team that reached it
	MechS32 m_staticObject;            // 0x04 — an index into g_staticObjects
	MechS32 m_hitPoints;               // 0x08 — destroyed when they run out
	MechS32 m_affiliation;             // 0x0c
	MechS32 m_radius;                  // 0x10 — its shape's radius (AfterWorldLoader)
	MechChar m_name[0x2a - 0x14];      // 0x14
	MechChar m_shortName[0x40 - 0x2a]; // 0x2a — for the target panel
} GameThing;

#endif // GAMETHING_H
