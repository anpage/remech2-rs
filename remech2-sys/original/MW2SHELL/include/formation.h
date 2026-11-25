#ifndef FORMATION_H
#define FORMATION_H

#include "types.h"

// A star formation: the positions of its three mechs and its name.
struct Formation {
	int32_t* m_positions; // three x, y pairs, never read
	char* m_name;
};

#endif // FORMATION_H
