#ifndef MISSIONOBJECTIVE_H
#define MISSIONOBJECTIVE_H

#include "types.h"

#pragma pack(push, 1)

// A condition of an objective: the state another objective must reach. The objective comes
// before its star: TestObjectiveCondition scales m_star by a star's mission and m_objective by
// an objective (a Trials of Grievance star waits on the player's objective that destroys the star
// before it).
typedef struct ObjectiveCondition {
	int32_t m_kind; // 0 ends the list; 1 done, 2 successful, 3 failed
	int32_t m_objective;
	int32_t m_star;
} ObjectiveCondition;

// One objective of a star's mission.
typedef struct MissionObjective {
	uint8_t m_state;         // 5 when successful, 6 when failed
	uint8_t m_priority;      // 1 primary, 2 secondary, 4 tertiary, 8 return
	int32_t m_type;          // one bit (see ai.c's objective type names)
	uint8_t m_allConditions; // every condition must hold, not just one
	ObjectiveCondition m_conditions[8];
	int32_t m_startTime;
	int32_t m_endTime; // when it succeeded or failed
	int32_t m_timeLimit;
	uint8_t m_active;
	uint8_t m_listed; // listed on the objectives panel
	int32_t m_mandatory;
	int32_t m_requiredCount; // how many targets must be done; 0 for all of them
	int32_t m_engagement;
	int32_t m_successSpeech;   // a sound resource, announced when successful
	int32_t m_failSpeech;      // when failed
	char m_successSound[0x10]; // a sound file, loaded by ReadSoundFile
	char m_failSound[0x10];
	int16_t m_targetStar; // the star whose objective m_targetObjective types 0x10000 and up act on
	int16_t m_targetObjective;
	char m_name[0xee - 0xad];
	uint8_t m_targetCount;
	uint16_t m_targets[40]; // AI target ids
} MissionObjective;

#pragma pack(pop)

#endif // MISSIONOBJECTIVE_H
