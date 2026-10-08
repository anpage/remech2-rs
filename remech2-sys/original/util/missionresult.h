#ifndef MISSIONRESULT_H
#define MISSIONRESULT_H

#include "types.h"

typedef struct MissionResultObjective {
	// 1 when the objective reached state 5, else 0
	MechS32 m_succeeded;
	// MissionObjective::m_priority: 1 primary, 2 secondary, 4 tertiary, 8 return
	MechS32 m_type;
	MechS32 m_startTime;
	// in seconds, negative when never reached
	MechS32 m_endTime;
	// MissionObjective::m_mandatory
	MechS32 m_mandatory;
	MechChar m_name[0x20];
} MissionResultObjective;

typedef struct MissionResult {
	// 'MW2M'
	MechU32 m_tag;
	MechS32 m_objectiveCount;
	MechS32 m_startTime;
	MechS32 m_endTime;
	// StarMission::m_status: 2 completed, 3 failed
	MechS32 m_outcome;
	MissionResultObjective m_objectives[48];
} MissionResult;

#endif // MISSIONRESULT_H
