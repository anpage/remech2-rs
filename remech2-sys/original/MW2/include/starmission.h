#ifndef STARMISSION_H
#define STARMISSION_H

#include "missionobjective.h"
#include "types.h"

#pragma pack(push, 1)

// A star's (a team's) mission: its objectives, and how it went.
typedef struct StarMission {
	int32_t m_objectiveCount;
	int32_t m_startTime;
	int32_t m_endTime;
	int32_t m_timeLimit;
	int32_t m_successSpeech;   // a sound resource, announced when successful
	int32_t m_failSpeech;      // when failed
	char m_successSound[0x10]; // a sound file, loaded by ReadSoundFile
	char m_failSound[0x10];
	uint8_t m_status; // 0 in progress, 2 successful, 3 failed, 4 out of time
	uint8_t m_affiliation;
	MissionObjective m_objectives[48];
} StarMission;

#pragma pack(pop)

#endif // STARMISSION_H
