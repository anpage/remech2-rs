#ifndef CUSTOMSTAR_H
#define CUSTOMSTAR_H

#include "starmech.h"
#include "types.h"

// A star of a custom battle: the player's (userstar.bwd) or the enemy's.
struct CustomStar {
	int32_t m_formation; // an index into g_formationOptions (mechvariant.cpp)
	int32_t m_selected;  // the mech the mech bay edits
	int32_t m_size;      // the most mechs the star can have
	int32_t m_count;     // mechs in the star
	int32_t m_tonnage;   // the heaviest mech allowed (KDMT), in tons
	StarMech m_mechs[3];
};

#endif // CUSTOMSTAR_H
