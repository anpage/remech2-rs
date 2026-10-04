#ifndef MECHCHASSIS_H
#define MECHCHASSIS_H

#include "decomp.h"
#include "types.h"

// SIZE 0x18
// A mech the bay can load. The table ends with a zeroed entry; only the first 15 are offered.
struct MechChassis {
	MechChar* m_code;     // 0x00 — code of the mech's videos, "awomp%s"
	MechChar* m_prefix;   // 0x04 — prefix of its variant files
	MechChar* m_chassis;  // 0x08 — the chassis of a BWD mech template
	MechChar* m_name;     // 0x0c
	MechS32 m_tonnage;    // 0x10
	MechS32 m_nameSample; // 0x14 — database item of the name sample, -1 for none
};

#endif // MECHCHASSIS_H
