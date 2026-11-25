#ifndef MISSION_H
#define MISSION_H

#include "careerrecord.h"
#include "missionresult.h"

// The sim hands this to the shell for debriefing
typedef struct MissionReport {
	MissionResult m_result;
	CareerRecord m_career;
} MissionReport;

typedef struct BwdBuffer {
	const char* m_name;
	const void* m_data;
	uint32_t m_size;
} BwdBuffer;

// The shell hands this to the sim to start a mission
typedef struct MissionLaunch {
	const BwdBuffer* m_bwds;
	uint32_t m_bwdCount;
} MissionLaunch;

#endif // MISSION_H
