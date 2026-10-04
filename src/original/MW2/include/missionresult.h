#ifndef MISSIONRESULT_H
#define MISSIONRESULT_H

#include "types.h"

// The mission result EndTheMission2 writes to mw2msn.cfg for the shell: the local team's mission
// times and status and its listed objectives.
// SIZE 0x9d4
typedef struct MissionResult {
	MechU32 m_tag;       // 0x00 — 'MW2M'
	MechS32 m_count;     // 0x04 — of m_objectives
	MechS32 m_startTime; // 0x08
	MechS32 m_endTime;   // 0x0c
	MechS32 m_status;    // 0x10 — StarMission::m_status
	struct {
		MechS32 m_succeeded;   // 0x00
		MechS32 m_priority;    // 0x04
		MechS32 m_startTime;   // 0x08
		MechS32 m_endTime;     // 0x0c
		MechS32 m_mandatory;   // 0x10 — MissionObjective::m_mandatory
		MechChar m_name[0x20]; // 0x14
	} m_objectives[48];        // 0x14
} MissionResult;

#endif // MISSIONRESULT_H
