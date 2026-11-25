#ifndef MISSIONENTRYCONDITION_H
#define MISSIONENTRYCONDITION_H

#include "types.h"

#pragma pack(push, 1)

// A condition of a mission table entry, which SetUpStarMission turns into an ObjectiveCondition.
typedef struct MissionEntryCondition {
	char m_kind;         // 0x00 — 'C' done, 'S' successful, 'F' failed; anything else ends the list
	uint8_t m_objective; // 0x01 — copied to the ObjectiveCondition's (SetUpStarMission)
	uint8_t m_star;      // 0x02
	uint8_t m_unk0x03;   // 0x03
} MissionEntryCondition;

#pragma pack(pop)

#endif // MISSIONENTRYCONDITION_H
