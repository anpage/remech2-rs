#ifndef MECHCHASSIS_H
#define MECHCHASSIS_H

#include "types.h"

// A mech the bay can load. The table ends with a zeroed entry; only the first 15 are offered.
struct MechChassis {
	char* m_code;    // code of the mech's videos, "awomp%s"
	char* m_prefix;  // prefix of its variant files
	char* m_chassis; // the chassis of a BWD mech template
	char* m_name;
	int32_t m_tonnage;
	int32_t m_nameSample; // database item of the name sample, -1 for none
};

#endif // MECHCHASSIS_H
