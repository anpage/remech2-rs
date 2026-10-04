#ifndef MISSIONTABLE_H
#define MISSIONTABLE_H

#include "bwdrecord.h"
#include "decomp.h"
#include "missionentrycondition.h"
#include "types.h"

#pragma pack(push, 1)

// An entry of a mission table, one per objective of the star's mission (StarMission), which
// SetUpStarMission copies into the objective.
// SIZE 0x97
typedef struct MissionEntry {
	MechS32 m_type;                        // 0x00 — the first entry's must be 0x10
	MechU8 m_listed;                       // 0x04 — 'V': on the objectives panel
	undefined m_unk0x05;                   // 0x05
	MechU8 m_allConditions;                // 0x06
	undefined m_unk0x07[0x0a - 0x07];      // 0x07
	MissionEntryCondition m_conditions[8]; // 0x0a
	MechS32 m_timeLimit;                   // 0x2a
	MechU8 m_priority;                     // 0x2e
	MechU8 m_requirement;             // 0x2f — 'M' mandatory (MissionObjective::m_mandatory), 'O' optional, 'N' neither
	MechU8 m_requiredCount;           // 0x30
	MechU8 m_engagement;              // 0x31
	undefined m_unk0x32[0x34 - 0x32]; // 0x32
	MechChar m_successSound[0x3f - 0x34]; // 0x34
	MechChar m_failSound[0x4a - 0x3f];    // 0x3f
	MechChar m_name[0x53 - 0x4a];         // 0x4a — the event list the objective waits on
	MechS16 m_targetStar;                 // 0x53
	MechS16 m_targetObjective;            // 0x55
	MechChar m_title[0x97 - 0x57];        // 0x57
} MissionEntry;

// A mission table (LoadMissionTable): a star's mission and its objectives.
typedef struct MissionTable {
	BwdRecord m_header;                   // 0x00
	MechS32 m_star;                       // 0x08 — the table's slot
	MechS32 m_timeLimit;                  // 0x0c
	undefined m_unk0x10[0x12 - 0x10];     // 0x10
	MechChar m_successSound[0x1d - 0x12]; // 0x12
	MechChar m_failSound[0x26 - 0x1d];    // 0x1d
	MissionEntry m_entries[1];            // 0x26 — up to the record's end
} MissionTable;

#pragma pack(pop)

#endif // MISSIONTABLE_H
