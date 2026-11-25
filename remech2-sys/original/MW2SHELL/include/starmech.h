#ifndef STARMECH_H
#define STARMECH_H

#include "types.h"

// One mech of a star: a negative m_chassis leaves the slot empty.
struct StarMech {
	int32_t m_chassis;    // an index into g_mechChassis (mechbay.cpp)
	char m_variant[0x10]; // variant file name
	char m_pilot[0x10];   // pilot name
};

#endif // STARMECH_H
