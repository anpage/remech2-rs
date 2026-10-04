#ifndef CUSTOMSTAR_H
#define CUSTOMSTAR_H

#include "decomp.h"
#include "starmech.h"
#include "types.h"

// SIZE 0x80
// A star of a custom battle: the player's (userstar.bwd) or the enemy's.
struct CustomStar {
	MechS32 m_formation; // 0x00 — an index into g_formationOptions (mechvariant.cpp)
	MechS32 m_selected;  // 0x04 — the mech the mech bay edits
	MechS32 m_size;      // 0x08 — the most mechs the star can have
	MechS32 m_count;     // 0x0c — mechs in the star
	MechS32 m_tonnage;   // 0x10 — the heaviest mech allowed (KDMT), in tons
	StarMech m_mechs[3]; // 0x14
};

#endif // CUSTOMSTAR_H
