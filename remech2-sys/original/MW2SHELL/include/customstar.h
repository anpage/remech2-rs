#ifndef CUSTOMSTAR_H
#define CUSTOMSTAR_H

#include "starmech.h"
#include "types.h"

// A star of a custom battle: the player's (userstar.bwd) or the enemy's.
struct CustomStar {
	int32_t m_formation; // 0x00 — an index into g_formationOptions (mechvariant.cpp)
	int32_t m_selected;  // 0x04 — the mech the mech bay edits
	int32_t m_size;      // 0x08 — the most mechs the star can have
	int32_t m_count;     // 0x0c — mechs in the star
	int32_t m_tonnage;   // 0x10 — the heaviest mech allowed (KDMT), in tons
	StarMech m_mechs[3]; // 0x14
};

#endif // CUSTOMSTAR_H
