#ifndef MISSION_H
#define MISSION_H

#include "careerrecord.h"
#include "missionresult.h"

// The sim hands this to the shell for debriefing
typedef struct MissionReport {
	MissionResult m_result;
	CareerRecord m_career;
} MissionReport;

#endif // MISSION_H
