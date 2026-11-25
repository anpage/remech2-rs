#ifndef MISSIONTABLE_H
#define MISSIONTABLE_H

#include "bwdrecord.h"
#include "missionentrycondition.h"
#include "types.h"

#pragma pack(push, 1)

// An entry of a mission table, one per objective of the star's mission (StarMission), which
// SetUpStarMission copies into the objective.
typedef struct MissionEntry {
	int32_t m_type;   // the first entry's must be 0x10
	uint8_t m_listed; // 'V': on the objectives panel
	uint8_t m_reserved0x05;
	uint8_t m_allConditions;
	uint8_t m_reserved0x07[0x0a - 0x07];
	MissionEntryCondition m_conditions[8];
	int32_t m_timeLimit;
	uint8_t m_priority;
	uint8_t m_requirement; // 'M' mandatory (MissionObjective::m_mandatory), 'O' optional, 'N' neither
	uint8_t m_requiredCount;
	uint8_t m_engagement;
	uint8_t m_reserved0x32[0x34 - 0x32];
	char m_successSound[0x3f - 0x34];
	char m_failSound[0x4a - 0x3f];
	char m_name[0x53 - 0x4a]; // the event list the objective waits on
	int16_t m_targetStar;
	int16_t m_targetObjective;
	char m_title[0x97 - 0x57];
} MissionEntry;

// A mission table (LoadMissionTable): a star's mission and its objectives.
typedef struct MissionTable {
	BwdRecord m_header;
	int32_t m_star; // the table's slot
	int32_t m_timeLimit;
	uint8_t m_reserved0x10[0x12 - 0x10];
	char m_successSound[0x1d - 0x12];
	char m_failSound[0x26 - 0x1d];
	MissionEntry m_entries[1]; // up to the record's end
} MissionTable;

#pragma pack(pop)

#endif // MISSIONTABLE_H
