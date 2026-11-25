#ifndef WEAPONSLOT_H
#define WEAPONSLOT_H

#include "types.h"

// An ammunition bin of a weapon (WeaponSlot::m_bin).
typedef struct WeaponBin {
	uint16_t m_type;    // an index into g_weaponDefs
	int16_t m_shots;    // shots left in the bin
	uint16_t m_weapon;  // the index of its weapon in Mech::m_weapons
	uint16_t m_id;      // its id in MechSection::m_slots
	uint16_t m_damage;  // WeaponDef::m_damage
	int32_t m_shotHeat; // WeaponDef::m_shotHeat
	int32_t m_heat;     // WeaponDef::m_heat
} WeaponBin;

// WeaponSlot::m_state.
enum {
	c_weaponEmpty = -1,    // out of ammunition
	c_weaponRecycling = 0, // until the weapon's m_recycle ticks have passed
	c_weaponReady = 1,
	c_weaponFiring = 2, // launching the shots of a volley
	c_weaponHeld = 3    // a volley ended with the trigger still down
};

// One of a mech's ten weapons (Mech::m_weapons). A slot with m_status at -1 ends the list.
typedef struct WeaponSlot {
	int32_t m_status;
	int32_t m_type; // an index into g_weaponDefs, or negative for none
	int32_t m_state;
	int32_t m_group;  // the weapon group, 0 to 2
	int32_t m_time;   // the clock of the last volley, or ticks into the current one
	int32_t m_target; // the locked target, for guided shots
	int32_t m_targetKind;
	int32_t m_ammo;      // -1 for unlimited
	int32_t m_volley;    // shots left in the current volley
	int32_t m_hardpoint; // an index into Mech::m_objects
	int32_t m_slotId;    // the id in MechSection::m_slots
	int32_t m_binCount;
	int32_t m_bins[10]; // its ammunition bins, indices into Mech::m_ammoBins
	int32_t m_index;    // its bit in the network weapons message
	int32_t m_binIndex;
	WeaponBin* m_bin;
} WeaponSlot;

#endif // WEAPONSLOT_H
