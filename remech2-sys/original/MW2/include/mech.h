#ifndef MECH_H
#define MECH_H

#include "decomp.h"
#include "mechsection.h"
#include "ramp.h"
#include "types.h"
#include "weaponslot.h"

// A player's mech: the state of every player type that moves (doors, vehicles and artillery use
// it too). Only the members matched code reaches are laid out.
struct SceneObject;
struct Player;

#pragma pack(push, 1)

typedef struct Mech {
	struct Player* m_player;          // 0x00
	Ramp m_torsoTwist;                // 0x04 — 16.16 degrees, towards the steering's torso pan
	Ramp m_torsoPitch;                // 0x14 — 16.16 degrees, towards the steering's torso tilt
	Ramp m_speed;                     // 0x24 — forward, towards m_topSpeed times the throttle (a door's x)
	Ramp m_turnRate;                  // 0x34 — added to the heading per second (a door's z)
	Ramp m_throttle;                  // 0x44 — 0x400 plus the steering's throttle times m_mobility (a door's y)
	WeaponSlot* m_weapons;            // 0x54 — ten
	MechSection* m_sections;          // 0x58 — the eight sections
	void* m_ammoBins;                 // 0x5c — the rest of the allocation, after the sections: AmmoBin records
	struct SceneObject* m_torsoObj;   // 0x60 — turns with the player's torso pitch, twist and roll
	struct SceneObject* m_pitchObj;   // 0x64 — pitches towards the point the weapons converge at
	struct SceneObject* m_objects[8]; // 0x68 — parts: the weapons fire from them, 6 and 7 are the jump jets
	int32_t m_topSpeed;               // 0x88 — the .MEK's speed, per throttle unit
	int32_t m_stateTime;              // 0x8c — the clock when the power or heat state changed (self-destruct's end)
	int32_t m_deltaHeat;              // 0x94 — heat added this tick
	int32_t m_heat;                   // 0x98 — 16.16: past 80 the mech overheats (m_flags 0x4)
	int32_t m_cooling;                // 0x9c — per tick: the .MEK's heat sinks times the climate's rate
	int32_t m_powerState;             // 0xa0 — of, su, nm, sd, dd, ej, co, ds (0 to 7; g_powerStateNames)
	int32_t m_collisionTicks;         // 0xa4 — how many ticks it has been colliding
	int32_t m_weaponCount;            // 0xa8
	int32_t m_selectedWeapon;         // 0xac — an index into m_weapons, or -1
	int32_t m_mobility;               // 0xb0 — 16.16, scales the throttle: each leg hit takes 0.1
	int32_t m_lastSelectedWeapon;     // 0xb8 — m_selectedWeapon before this tick's weapon input
	int32_t m_autopilot;              // 0xbc — 1 and 2 are on
	int32_t m_jumpFuel;               // 0xc0 — the jump jets fire while it is positive; -2 without jets
	int32_t m_jumpJets;               // 0xc4 — the .MEK's jump jets: each critical hit takes one
	int32_t m_ammoBinCount;           // 0xc8
	int32_t m_height;                 // 0xcc — of its object's origin above its feet (the MGD's)
	int32_t m_cockpitHeight;          // 0xd0 — added to the eyepoint (g_eyeHeightOffset)
	int32_t m_maxTorsoTwist;          // 0xe0 — 16.16 degrees either way
	int32_t m_tons;                   // 0xe4 — ApplyCollisionDamage scales collision damage by it
	int32_t m_radius;                 // 0xe8 — splash damage reaches it this much further
	int32_t m_jumpThrust;             // 0xec — the jump jets' upward acceleration
	int32_t m_velocityX;              // 0xf4
	int32_t m_velocityY;              // 0xf8
	int32_t m_velocityZ;              // 0xfc
	int32_t m_newVelocityX;           // 0x100 — the velocity integrated this tick, before collisions
	int32_t m_newVelocityY;           // 0x104
	int32_t m_newVelocityZ;           // 0x108
	// 0x10c: 0x1 a trigger is held, 0x4 overheating (the heat passed 80), 0x40 locking on, 0x80
	// locked on, 0x2000 the player's torso pitch follows m_torsoPitch, 0x4000 moving a weapon
	// between groups, 0x8000 the target is in the lock cone.
	uint16_t m_flags; // 0x10c
} Mech;

#pragma pack(pop)

#endif // MECH_H
