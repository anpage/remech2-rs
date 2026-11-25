#ifndef TEAMFORMATION_H
#define TEAMFORMATION_H

#include "types.h"

// A named formation: each of the eight slots' offset from the leader and heading.
typedef struct TeamFormation {
	char m_name[0x10];    // 0x00
	int32_t m_x[8];       // 0x10
	int32_t m_z[8];       // 0x30
	int32_t m_heading[8]; // 0x50
} TeamFormation;

#endif // TEAMFORMATION_H
