#ifndef MECH_H
#define MECH_H

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
	struct Player* m_player;
	Ramp m_torsoTwist;                // 16.16 degrees, towards the steering's torso pan
	Ramp m_torsoPitch;                // 16.16 degrees, towards the steering's torso tilt
	Ramp m_speed;                     // forward, towards m_topSpeed times the throttle (a door's x)
	Ramp m_turnRate;                  // added to the heading per second (a door's z)
	Ramp m_throttle;                  // 0x400 plus the steering's throttle times m_mobility (a door's y)
	WeaponSlot* m_weapons;            // ten
	MechSection* m_sections;          // the eight sections
	void* m_ammoBins;                 // the rest of the allocation, after the sections: AmmoBin records
	struct SceneObject* m_torsoObj;   // turns with the player's torso pitch, twist and roll
	struct SceneObject* m_pitchObj;   // pitches towards the point the weapons converge at
	struct SceneObject* m_objects[8]; // parts: the weapons fire from them, 6 and 7 are the jump jets
	int32_t m_topSpeed;               // the .MEK's speed, per throttle unit
	int32_t m_stateTime;              // the clock when the power or heat state changed (self-destruct's end)
	int32_t m_deltaHeat;              // heat added this tick
	int32_t m_heat;                   // 16.16: past 80 the mech overheats (m_flags 0x4)
	int32_t m_cooling;                // per tick: the .MEK's heat sinks times the climate's rate
	int32_t m_powerState;             // of, su, nm, sd, dd, ej, co, ds (0 to 7; g_powerStateNames)
	int32_t m_collisionTicks;         // how many ticks it has been colliding
	int32_t m_weaponCount;
	int32_t m_selectedWeapon;     // an index into m_weapons, or -1
	int32_t m_mobility;           // 16.16, scales the throttle: each leg hit takes 0.1
	int32_t m_lastSelectedWeapon; // m_selectedWeapon before this tick's weapon input
	int32_t m_autopilot;          // 1 and 2 are on
	int32_t m_jumpFuel;           // the jump jets fire while it is positive; -2 without jets
	int32_t m_jumpJets;           // the .MEK's jump jets: each critical hit takes one
	int32_t m_ammoBinCount;
	int32_t m_height;        // of its object's origin above its feet (the MGD's)
	int32_t m_cockpitHeight; // added to the eyepoint (g_eyeHeightOffset)
	int32_t m_maxTorsoTwist; // 16.16 degrees either way
	int32_t m_tons;          // ApplyCollisionDamage scales collision damage by it
	int32_t m_radius;        // splash damage reaches it this much further
	int32_t m_jumpThrust;    // the jump jets' upward acceleration
	int32_t m_velocityX;
	int32_t m_velocityY;
	int32_t m_velocityZ;
	int32_t m_newVelocityX; // the velocity integrated this tick, before collisions
	int32_t m_newVelocityY;
	int32_t m_newVelocityZ;
	// 0x1 a trigger is held, 0x4 overheating (the heat passed 80), 0x40 locking on, 0x80
	// locked on, 0x2000 the player's torso pitch follows m_torsoPitch, 0x4000 moving a weapon
	// between groups, 0x8000 the target is in the lock cone.
	uint16_t m_flags;
} Mech;

#pragma pack(pop)

#endif // MECH_H
