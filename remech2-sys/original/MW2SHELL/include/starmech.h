#ifndef STARMECH_H
#define STARMECH_H

#include "decomp.h"
#include "types.h"

// SIZE 0x24
// One mech of a star: a negative m_chassis leaves the slot empty.
struct StarMech {
	MechS32 m_chassis;    // 0x00 — an index into g_mechChassis (mechbay.cpp)
	char m_variant[0x10]; // 0x04 — variant file name
	char m_pilot[0x10];   // 0x14 — pilot name
};

#endif // STARMECH_H
