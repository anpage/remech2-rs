#ifndef CAREERRECORD_H
#define CAREERRECORD_H

#include "decomp.h"
#include "types.h"

#pragma pack(push, 1)

// The mission's combat record, saved to MW2CAR.CFG for the shell (MW2SHELL's CareerRecord) as one
// block. The counters are kept by side (GetPlayerSide, GetThingSide): 0 friendly, 1 enemy and 2
// neutral; the plain names are the enemy's. "Direct" kills are the local player's own, the others
// count every destroyed mech; "vehicles" are the mechs of players that aren't c_playerTypeMech.
// SIZE 0xd6
typedef struct CareerRecord {
	undefined m_unk0x00[0x07 - 0x00];     // 0x00
	MechU16 m_directMechKills;            // 0x07 — players' mechs of side 1 the local player destroyed
	MechU16 m_directNeutralMechKills;     // 0x09 — side 2
	MechU16 m_directFriendlyMechKills;    // 0x0b — side 0
	MechU16 m_directThingKills;           // 0x0d — things of side 1 the local player destroyed
	MechU16 m_directNeutralThingKills;    // 0x0f — side 2
	MechU16 m_directFriendlyThingKills;   // 0x11 — side 0
	MechU16 m_shotsFired;                 // 0x13 — shots fired by the local player
	MechU16 m_hits;                       // 0x15 — hits by the local player on side 1
	MechU16 m_neutralHits;                // 0x17 — side 2
	MechU16 m_friendlyHits;               // 0x19 — side 0
	MechU16 m_hitsTaken;                  // 0x1b — hits taken by the local player
	MechU8 m_outcome;                     // 0x1d — how the local player's mech went down (2, 4)
	MechU16 m_mechKills;                  // 0x1e — players' mechs of side 1 with flag 0x1400 destroyed
	MechU16 m_neutralMechKills;           // 0x20 — side 2
	MechU16 m_friendlyMechKills;          // 0x22 — side 0
	MechU16 m_teamThingKills;             // 0x24 — things of side 1 the local team destroyed
	MechU16 m_teamNeutralThingKills;      // 0x26 — side 2
	MechU16 m_teamFriendlyThingKills;     // 0x28 — side 0
	MechU16 m_teamShotsFired;             // 0x2a — shots fired by the local team
	MechU16 m_teamFriendlyHits;           // 0x2c — hits by the local team on side 0
	MechU16 m_teamNeutralHits;            // 0x2e — side 2
	MechU16 m_teamHits;                   // 0x30 — side 1
	MechU16 m_teamHitsTaken;              // 0x32 — hits taken by the local team
	MechU16 m_wingmenLost;                // 0x34 — mechs of the local team destroyed
	MechU16 m_wingmenEjected;             // 0x36 — of those, the ones in power state 5 (Mech::m_powerState)
	MechS16 m_sideCounts[6];              // 0x38 — players (0-2) and game things (3-5) of sides 0, 2 and 1
	MechU16 m_directVehicleKills;         // 0x44 — other mechs of side 1 the local player destroyed
	MechU16 m_directNeutralVehicleKills;  // 0x46 — side 2
	MechU16 m_directFriendlyVehicleKills; // 0x48 — side 0
	MechU16 m_vehicleKills;               // 0x4a — other mechs of side 1 with flag 0x1400 destroyed
	MechU16 m_neutralVehicleKills;        // 0x4c — side 2
	MechU16 m_friendlyVehicleKills;       // 0x4e — side 0
	MechU16 m_playerCount;                // 0x50 — the players of a network game
	MechU16 m_kills[8][8];                // 0x52 — kills, by killer and victim player
	MechS32 m_winner;                     // 0xd2 — the winner: the last player whose SU message arrived, or -1
} CareerRecord;

#pragma pack(pop)

#endif // CAREERRECORD_H
