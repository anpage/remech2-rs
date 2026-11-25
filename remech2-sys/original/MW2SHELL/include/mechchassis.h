#ifndef MECHCHASSIS_H
#define MECHCHASSIS_H

#include "decomp.h"
#include "types.h"

// A mech the bay can load. The table ends with a zeroed entry; only the first 15 are offered.
struct MechChassis {
	char* m_code;         // 0x00 — code of the mech's videos, "awomp%s"
	char* m_prefix;       // 0x04 — prefix of its variant files
	char* m_chassis;      // 0x08 — the chassis of a BWD mech template
	char* m_name;         // 0x0c
	int32_t m_tonnage;    // 0x10
	int32_t m_nameSample; // 0x14 — database item of the name sample, -1 for none
};

#endif // MECHCHASSIS_H
