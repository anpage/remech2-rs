#ifndef MISSIONOBJECTIVE_H
#define MISSIONOBJECTIVE_H

#include "decomp.h"
#include "types.h"

#pragma pack(push, 1)

// A condition of an objective: the state another objective must reach. The objective comes
// before its star: TestObjectiveCondition scales the word at 0x08 by a star's mission and the one at 0x04 by
// an objective (a Trials of Grievance star waits on the player's objective that destroys the star
// before it).
// SIZE 0xc
typedef struct ObjectiveCondition {
	MechS32 m_kind;      // 0x00 — 0 ends the list; 1 done, 2 successful, 3 failed
	MechS32 m_objective; // 0x04
	MechS32 m_star;      // 0x08
} ObjectiveCondition;

// One objective of a star's mission.
// SIZE 0x13f
typedef struct MissionObjective {
	MechU8 m_state;                     // 0x00 — 5 when successful, 6 when failed
	MechU8 m_priority;                  // 0x01 — 1 primary, 2 secondary, 4 tertiary, 8 return
	MechS32 m_type;                     // 0x02 — one bit (see ai.c's objective type names)
	MechU8 m_allConditions;             // 0x06 — every condition must hold, not just one
	ObjectiveCondition m_conditions[8]; // 0x07
	MechS32 m_startTime;                // 0x67
	MechS32 m_endTime;                  // 0x6b — when it succeeded or failed
	MechS32 m_timeLimit;                // 0x6f
	MechU8 m_active;                    // 0x73
	MechU8 m_listed;                    // 0x74 — listed on the objectives panel
	MechS32 m_mandatory;                // 0x75
	MechS32 m_requiredCount;            // 0x79 — how many targets must be done; 0 for all of them
	MechS32 m_engagement;               // 0x7d
	MechS32 m_successSpeech;            // 0x81 — a sound resource, announced when successful
	MechS32 m_failSpeech;               // 0x85 — when failed
	MechChar m_successSound[0x10];      // 0x89 — a sound file, loaded by ReadSoundFile
	MechChar m_failSound[0x10];         // 0x99
	MechS16 m_targetStar;               // 0xa9 — the star whose objective m_targetObjective types 0x10000 and up act on
	MechS16 m_targetObjective;          // 0xab
	MechChar m_name[0xee - 0xad];       // 0xad
	MechU8 m_targetCount;               // 0xee
	MechU16 m_targets[40];              // 0xef — AI target ids
} MissionObjective;

#pragma pack(pop)

#endif // MISSIONOBJECTIVE_H
