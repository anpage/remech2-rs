#ifndef GAMETHING_H
#define GAMETHING_H

#include "types.h"

typedef struct GameThing {
	int16_t m_flags;        // 4: destroyed
	int16_t m_teamsReached; // a bit per team that reached it
	int32_t m_staticObject; // an index into g_staticObjects
	int32_t m_hitPoints;    // destroyed when they run out
	int32_t m_affiliation;
	int32_t m_radius; // its shape's radius (AfterWorldLoader)
	char m_name[0x2a - 0x14];
	char m_shortName[0x40 - 0x2a]; // for the target panel
} GameThing;

#endif // GAMETHING_H
