#ifndef MECHSECTION_H
#define MECHSECTION_H

#include "types.h"

// One of a mech's eight sections (Mech::m_sections), as the network state message carries it.
// The low two nibbles of m_flags scale the damage level of m_armor[0] and m_armor[1]; a
// section with m_internal used up and m_flags bit 0x2000 clear is destroyed.
typedef struct MechSection {
	int32_t m_armor[2]; // front and rear
	int32_t m_internal;
	uint16_t m_slots[12]; // critical slots; above 10000, an ammunition bin id
	int16_t m_slotCount;  // the slots in use
	int16_t m_flags;      // 0x2000: destroyed
} MechSection;

#endif // MECHSECTION_H
