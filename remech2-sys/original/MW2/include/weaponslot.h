#ifndef WEAPONSLOT_H
#define WEAPONSLOT_H

#include "decomp.h"
#include "types.h"

// An ammunition bin of a weapon (WeaponSlot::m_bin).
typedef struct WeaponBin {
	uint16_t m_type;    // 0x00 — an index into g_weaponDefs
	int16_t m_shots;    // 0x02 — shots left in the bin
	uint16_t m_weapon;  // 0x04 — the index of its weapon in Mech::m_weapons
	uint16_t m_id;      // 0x06 — its id in MechSection::m_slots
	uint16_t m_damage;  // 0x0a — WeaponDef::m_damage
	int32_t m_shotHeat; // 0x0c — WeaponDef::m_shotHeat
	int32_t m_heat;     // 0x10 — WeaponDef::m_heat
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
	int32_t m_status;     // 0x00
	int32_t m_type;       // 0x04 — an index into g_weaponDefs, or negative for none
	int32_t m_state;      // 0x08
	int32_t m_group;      // 0x0c — the weapon group, 0 to 2
	int32_t m_time;       // 0x10 — the clock of the last volley, or ticks into the current one
	int32_t m_target;     // 0x18 — the locked target, for guided shots
	int32_t m_targetKind; // 0x1c
	int32_t m_ammo;       // 0x20 — -1 for unlimited
	int32_t m_volley;     // 0x24 — shots left in the current volley
	int32_t m_hardpoint;  // 0x28 — an index into Mech::m_objects
	int32_t m_slotId;     // 0x2c — the id in MechSection::m_slots
	int32_t m_binCount;   // 0x30
	int32_t m_bins[10];   // 0x34 — its ammunition bins, indices into Mech::m_ammoBins
	int32_t m_index;      // 0x5c — its bit in the network weapons message
	int32_t m_binIndex;   // 0x60
	WeaponBin* m_bin;     // 0x64
} WeaponSlot;

#endif // WEAPONSLOT_H
