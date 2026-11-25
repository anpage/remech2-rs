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

	extern int32_t g_missionTime;
	extern int32_t g_currentObjective[16];
	extern int32_t g_objectiveAnnounced[48];
	extern StarMission g_objectiveTable[16];
	extern int32_t g_objectiveCount;
	void CollapseWhitespace(char* p_text);
	extern int32_t g_forceMissionSuccess;
	extern int32_t g_missionResultAnnounced;
	extern uint32_t g_missionResultTag;

	int32_t DoFirstObjtv(StarMission* p_mission, int32_t p_team);
	int32_t GetObjectiveTargetState(uint8_t* p_target);
	int32_t HasTeamReachedTarget(uint8_t* p_target, int32_t p_team);
	int32_t IsTeamNearTarget(uint8_t* p_target, int32_t p_team);
	int32_t AnnounceObjective(int32_t p_star, int32_t p_objective, int32_t p_state);
	void ChooseNetworkWinner(void);
	int32_t AnnounceMissionResult(int32_t p_star, int32_t p_status);
	int32_t TestObjectiveCondition(int32_t p_star, int32_t p_objective, int32_t p_condition);
	int32_t ObjectiveConditionsHold(int32_t p_star, int32_t p_objective);
	void UpdateObjective(int32_t p_star, int32_t p_objective);
	void UpdateObjectives(void);
	void EndTheMission1(void);
	int32_t EndTheMission2(void);
	void RestartStarMission(int32_t p_star);
	uint16_t GetTeamHomeTarget(int32_t p_team);

#ifdef __cplusplus
}
#endif

#endif // OBJECTIVE_H
