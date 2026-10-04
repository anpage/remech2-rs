#ifndef TEAMFORMATION_H
#define TEAMFORMATION_H

#include "types.h"

// A named formation: each of the eight slots' offset from the leader and heading.
// SIZE 0x70
typedef struct TeamFormation {
	MechChar m_name[0x10]; // 0x00
	MechS32 m_x[8];        // 0x10
	MechS32 m_z[8];        // 0x30
	MechS32 m_heading[8];  // 0x50
} TeamFormation;

#endif // TEAMFORMATION_H
