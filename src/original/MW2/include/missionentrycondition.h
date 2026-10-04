#ifndef MISSIONENTRYCONDITION_H
#define MISSIONENTRYCONDITION_H

#include "decomp.h"
#include "types.h"

#pragma pack(push, 1)

// A condition of a mission table entry, which SetUpStarMission turns into an ObjectiveCondition.
// SIZE 0x4
typedef struct MissionEntryCondition {
	MechChar m_kind;     // 0x00 — 'C' done, 'S' successful, 'F' failed; anything else ends the list
	MechU8 m_objective;  // 0x01 — copied to the ObjectiveCondition's (SetUpStarMission)
	MechU8 m_star;       // 0x02
	undefined m_unk0x03; // 0x03
} MissionEntryCondition;

#pragma pack(pop)

#endif // MISSIONENTRYCONDITION_H
