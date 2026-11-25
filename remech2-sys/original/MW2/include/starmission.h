#ifndef STARMISSION_H
#define STARMISSION_H

#include "decomp.h"
#include "missionobjective.h"
#include "types.h"

#pragma pack(push, 1)

// A star's (a team's) mission: its objectives, and how it went.
typedef struct StarMission {
	int32_t m_objectiveCount;          // 0x00
	int32_t m_startTime;               // 0x04
	int32_t m_endTime;                 // 0x08
	int32_t m_timeLimit;               // 0x0c
	int32_t m_successSpeech;           // 0x10 — a sound resource, announced when successful
	int32_t m_failSpeech;              // 0x14 — when failed
	char m_successSound[0x10];         // 0x18 — a sound file, loaded by ReadSoundFile
	char m_failSound[0x10];            // 0x28
	uint8_t m_status;                  // 0x38 — 0 in progress, 2 successful, 3 failed, 4 out of time
	undefined m_affiliation;           // 0x39
	MissionObjective m_objectives[48]; // 0x3a
} StarMission;

#pragma pack(pop)

#endif // STARMISSION_H
