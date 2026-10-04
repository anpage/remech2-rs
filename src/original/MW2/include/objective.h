#ifndef OBJECTIVE_H
#define OBJECTIVE_H

#include "decomp.h"
#include "starmission.h"
#include "types.h"

// The functions and globals of objective.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_missionTime;
	extern MechS32 g_currentObjective[16];
	extern MechS32 g_objectiveAnnounced[48];
	extern StarMission g_objectiveTable[16];
	extern MechS32 g_objectiveCount;
	void CollapseWhitespace(MechChar* p_text);
	extern MechS32 g_forceMissionSuccess;
	extern MechS32 g_missionResultAnnounced;

	MechS32 DoFirstObjtv(StarMission* p_mission, MechS32 p_team);
	MechS32 GetObjectiveTargetState(MechU8* p_target);
	MechS32 HasTeamReachedTarget(MechU8* p_target, MechS32 p_team);
	MechS32 IsTeamNearTarget(MechU8* p_target, MechS32 p_team);
	MechS32 AnnounceObjective(MechS32 p_star, MechS32 p_objective, MechS32 p_state);
	void ChooseNetworkWinner(void);
	MechS32 AnnounceMissionResult(MechS32 p_star, MechS32 p_status);
	MechS32 TestObjectiveCondition(MechS32 p_star, MechS32 p_objective, MechS32 p_condition);
	MechS32 ObjectiveConditionsHold(MechS32 p_star, MechS32 p_objective);
	void UpdateObjective(MechS32 p_star, MechS32 p_objective);
	void UpdateObjectives(void);
	void EndTheMission1(void);
	MechS32 EndTheMission2(void);
	void RestartStarMission(MechS32 p_star);
	void FUN_1001cdd1(void);
	MechS32 FUN_1001cde1(undefined4 p_unk0x00);
	MechU16 GetTeamHomeTarget(MechS32 p_team);

#ifdef __cplusplus
}
#endif

#endif // OBJECTIVE_H
