#ifndef MISSIONRESULT_H
#define MISSIONRESULT_H

#include "types.h"

typedef struct MissionResultObjective {
	// 1 when the objective reached state 5, else 0
	int32_t m_succeeded;
	// MissionObjective::m_priority: 1 primary, 2 secondary, 4 tertiary, 8 return
	int32_t m_type;
	int32_t m_startTime;
	// in seconds, negative when never reached
	int32_t m_endTime;
	// MissionObjective::m_mandatory
	int32_t m_mandatory;
	char m_name[0x20];
} MissionResultObjective;

typedef struct MissionResult {
	// 'MW2M'
	uint32_t m_tag;
	int32_t m_objectiveCount;
	int32_t m_startTime;
	int32_t m_endTime;
	// StarMission::m_status: 2 completed, 3 failed
	int32_t m_outcome;
	MissionResultObjective m_objectives[48];
} MissionResult;

#endif // MISSIONRESULT_H
