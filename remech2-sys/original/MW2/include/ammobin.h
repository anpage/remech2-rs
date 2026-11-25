#ifndef AMMOBIN_H
#define AMMOBIN_H

#include "types.h"

// An ammunition bin of a mech (Mech::m_ammoBins, Mech::m_ammoBinCount of them): the weapon it feeds and
// the critical-slot id it occupies (DestroyMech).
typedef struct AmmoBin {
	int16_t m_type;
	int16_t m_shots;
	int16_t m_weapon; // an index into Mech::m_weapons, or -1
	int16_t m_id;     // the id in MechSection::m_slots
	int16_t m_damage;
	int32_t m_shotHeat;
	int32_t m_heat;
} AmmoBin;

#endif // AMMOBIN_H
