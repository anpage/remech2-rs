#ifndef AMMOBIN_H
#define AMMOBIN_H

#include "types.h"

// An ammunition bin of a mech (Mech::m_ammoBins, Mech::m_ammoBinCount of them): the weapon it feeds and
// the critical-slot id it occupies (DestroyMech).
typedef struct AmmoBin {
	int16_t m_type;     // 0x00
	int16_t m_shots;    // 0x02
	int16_t m_weapon;   // 0x04 — an index into Mech::m_weapons, or -1
	int16_t m_id;       // 0x06 — the id in MechSection::m_slots
	int16_t m_damage;   // 0x0a
	int32_t m_shotHeat; // 0x0c
	int32_t m_heat;     // 0x10
} AmmoBin;

#endif // AMMOBIN_H
