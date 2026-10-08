#ifndef CAREERRECORD_H
#define CAREERRECORD_H

#include "types.h"

// The mission's combat record, handed to the shell's debriefing in the MissionReport.
// The counters are kept by side (GetPlayerSide, GetThingSide): 0 friendly, 1 enemy and 2
// neutral; the plain names are the enemy's. "Direct" kills are the local player's own, the others
// count every destroyed mech; "vehicles" are the mechs of players that aren't c_playerTypeMech.
typedef struct CareerRecord {
	// players' mechs of side 1 the local player destroyed
	MechU16 m_directMechKills;
	// side 2
	MechU16 m_directNeutralMechKills;
	// side 0
	MechU16 m_directFriendlyMechKills;
	// things of side 1 the local player destroyed
	MechU16 m_directThingKills;
	// side 2
	MechU16 m_directNeutralThingKills;
	// side 0
	MechU16 m_directFriendlyThingKills;
	// shots fired by the local player
	MechU16 m_shotsFired;
	// hits by the local player on side 1
	MechU16 m_hits;
	// side 2
	MechU16 m_neutralHits;
	// side 0
	MechU16 m_friendlyHits;
	// hits taken by the local player
	MechU16 m_hitsTaken;
	// how the local player's mech went down (2, 4)
	MechU8 m_outcome;
	// players' mechs of side 1 with flag 0x1400 destroyed
	MechU16 m_mechKills;
	// side 2
	MechU16 m_neutralMechKills;
	// side 0
	MechU16 m_friendlyMechKills;
	// things of side 1 the local team destroyed
	MechU16 m_teamThingKills;
	// side 2
	MechU16 m_teamNeutralThingKills;
	// side 0
	MechU16 m_teamFriendlyThingKills;
	// shots fired by the local team
	MechU16 m_teamShotsFired;
	// hits by the local team on side 0
	MechU16 m_teamFriendlyHits;
	// side 2
	MechU16 m_teamNeutralHits;
	// side 1
	MechU16 m_teamHits;
	// hits taken by the local team
	MechU16 m_teamHitsTaken;
	// mechs of the local team destroyed
	MechU16 m_wingmenLost;
	// of those, the ones in power state 5 (Mech::m_powerState)
	MechU16 m_wingmenEjected;
	// players (0-2) and game things (3-5) of sides 0, 2 and 1
	MechS16 m_sideCounts[6];
	// other mechs of side 1 the local player destroyed
	MechU16 m_directVehicleKills;
	// side 2
	MechU16 m_directNeutralVehicleKills;
	// side 0
	MechU16 m_directFriendlyVehicleKills;
	// other mechs of side 1 with flag 0x1400 destroyed
	MechU16 m_vehicleKills;
	// side 2
	MechU16 m_neutralVehicleKills;
	// side 0
	MechU16 m_friendlyVehicleKills;
	// the players of a network game
	MechU16 m_playerCount;
	// kills, by killer and victim player
	MechU16 m_kills[8][8];
	// the winner: the last player whose SU message arrived, or -1
	MechS32 m_winner;
} CareerRecord;

#endif // CAREERRECORD_H
