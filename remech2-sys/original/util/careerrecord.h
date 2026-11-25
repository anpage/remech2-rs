#ifndef CAREERRECORD_H
#define CAREERRECORD_H

#include "types.h"

// The mission's combat record, handed to the shell's debriefing in the MissionReport.
// The counters are kept by side (GetPlayerSide, GetThingSide): 0 friendly, 1 enemy and 2
// neutral; the plain names are the enemy's. "Direct" kills are the local player's own, the others
// count every destroyed mech; "vehicles" are the mechs of players that aren't c_playerTypeMech.
typedef struct CareerRecord {
	// players' mechs of side 1 the local player destroyed
	uint16_t m_directMechKills;
	// side 2
	uint16_t m_directNeutralMechKills;
	// side 0
	uint16_t m_directFriendlyMechKills;
	// things of side 1 the local player destroyed
	uint16_t m_directThingKills;
	// side 2
	uint16_t m_directNeutralThingKills;
	// side 0
	uint16_t m_directFriendlyThingKills;
	// shots fired by the local player
	uint16_t m_shotsFired;
	// hits by the local player on side 1
	uint16_t m_hits;
	// side 2
	uint16_t m_neutralHits;
	// side 0
	uint16_t m_friendlyHits;
	// hits taken by the local player
	uint16_t m_hitsTaken;
	// how the local player's mech went down (2, 4)
	uint8_t m_outcome;
	// players' mechs of side 1 with flag 0x1400 destroyed
	uint16_t m_mechKills;
	// side 2
	uint16_t m_neutralMechKills;
	// side 0
	uint16_t m_friendlyMechKills;
	// things of side 1 the local team destroyed
	uint16_t m_teamThingKills;
	// side 2
	uint16_t m_teamNeutralThingKills;
	// side 0
	uint16_t m_teamFriendlyThingKills;
	// shots fired by the local team
	uint16_t m_teamShotsFired;
	// hits by the local team on side 0
	uint16_t m_teamFriendlyHits;
	// side 2
	uint16_t m_teamNeutralHits;
	// side 1
	uint16_t m_teamHits;
	// hits taken by the local team
	uint16_t m_teamHitsTaken;
	// mechs of the local team destroyed
	uint16_t m_wingmenLost;
	// of those, the ones in power state 5 (Mech::m_powerState)
	uint16_t m_wingmenEjected;
	// players (0-2) and game things (3-5) of sides 0, 2 and 1
	int16_t m_sideCounts[6];
	// other mechs of side 1 the local player destroyed
	uint16_t m_directVehicleKills;
	// side 2
	uint16_t m_directNeutralVehicleKills;
	// side 0
	uint16_t m_directFriendlyVehicleKills;
	// other mechs of side 1 with flag 0x1400 destroyed
	uint16_t m_vehicleKills;
	// side 2
	uint16_t m_neutralVehicleKills;
	// side 0
	uint16_t m_friendlyVehicleKills;
	// the players of a network game
	uint16_t m_playerCount;
	// kills, by killer and victim player
	uint16_t m_kills[8][8];
	// the winner: the last player whose SU message arrived, or -1
	int32_t m_winner;
} CareerRecord;

#endif // CAREERRECORD_H
