#ifndef FORMATION_H
#define FORMATION_H

#include "decomp.h"
#include "types.h"

// SIZE 0x08
// A star formation: the positions of its three mechs and its name.
struct Formation {
	MechS32* m_positions; // 0x00 — three x, y pairs, never read
	MechChar* m_name;     // 0x04
};

#endif // FORMATION_H
