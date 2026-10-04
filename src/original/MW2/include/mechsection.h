#ifndef MECHSECTION_H
#define MECHSECTION_H

#include "decomp.h"
#include "types.h"

// One of a mech's eight sections (Mech::m_sections), as the network state message carries it.
// The low two nibbles of m_flags scale the damage level of m_armor[0] and m_armor[1]; a
// section with m_unk0x08 used up and m_flags bit 0x2000 clear is destroyed.
// SIZE 0x28
typedef struct MechSection {
	MechS32 m_armor[2];  // 0x00 — front and rear
	MechS32 m_internal;  // 0x08
	MechU16 m_slots[12]; // 0x0c — critical slots; above 10000, an ammunition bin id
	MechS16 m_slotCount; // 0x24 — the slots in use
	MechS16 m_flags;     // 0x26 — 0x2000: destroyed
} MechSection;

#endif // MECHSECTION_H
