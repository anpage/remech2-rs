#ifndef WEAPONSLOT_H
#define WEAPONSLOT_H

#include "decomp.h"
#include "types.h"

// An ammunition bin of a weapon (WeaponSlot::m_bin).
// SIZE 0x14
typedef struct WeaponBin {
	MechU16 m_type;       // 0x00 — an index into g_weaponDefs
	MechS16 m_shots;      // 0x02 — shots left in the bin
	MechU16 m_weapon;     // 0x04 — the index of its weapon in Mech::m_weapons
	MechU16 m_id;         // 0x06 — its id in MechSection::m_slots
	undefined2 m_unk0x08; // 0x08
	MechU16 m_damage;     // 0x0a — WeaponDef::m_damage
	MechS32 m_shotHeat;   // 0x0c — WeaponDef::m_shotHeat
	MechS32 m_heat;       // 0x10 — WeaponDef::m_heat
} WeaponBin;

// WeaponSlot::m_state.
enum {
	c_weaponEmpty = -1,    // out of ammunition
	c_weaponRecycling = 0, // until the weapon's m_recycle ticks have passed
	c_weaponReady = 1,
	c_weaponFiring = 2, // launching the shots of a volley
	c_weaponHeld = 3    // a volley ended with the trigger still down
};

// One of a mech's ten weapons (Mech::m_weapons). A slot with m_unk0x00 at -1 ends the list.
// SIZE 0x68
typedef struct WeaponSlot {
	MechS32 m_status;     // 0x00
	MechS32 m_type;       // 0x04 — an index into g_weaponDefs, or negative for none
	MechS32 m_state;      // 0x08
	MechS32 m_group;      // 0x0c — the weapon group, 0 to 2
	MechS32 m_time;       // 0x10 — the clock of the last volley, or ticks into the current one
	undefined4 m_unk0x14; // 0x14
	MechS32 m_target;     // 0x18 — the locked target, for guided shots
	MechS32 m_targetKind; // 0x1c
	MechS32 m_ammo;       // 0x20 — -1 for unlimited
	MechS32 m_volley;     // 0x24 — shots left in the current volley
	MechS32 m_hardpoint;  // 0x28 — an index into Mech::m_objects
	MechS32 m_slotId;     // 0x2c — the id in MechSection::m_slots
	MechS32 m_binCount;   // 0x30
	MechS32 m_bins[10];   // 0x34 — its ammunition bins, indices into Mech::m_ammoBins
	MechS32 m_index;      // 0x5c — its bit in the network weapons message
	MechS32 m_binIndex;   // 0x60
	WeaponBin* m_bin;     // 0x64
} WeaponSlot;

#endif // WEAPONSLOT_H
