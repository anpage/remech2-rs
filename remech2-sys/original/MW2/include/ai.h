#ifndef AI_H
#define AI_H

#include "aimessage.h"
#include "airule.h"
#include "types.h"

struct Player;
typedef struct AiName AiName;

// The AI states (Player::m_ai.m_state), and the commands the local player gives its star.
enum {
	c_aiStateIdle = 0,
	c_aiStateAvoid = 1,
	c_aiStateTarget = 2,
	c_aiStateAttack = 3,
	c_aiStateFlee = 4,
	c_aiStateFollow = 5,
	c_aiStateRecon = 6,
	c_aiStatePatrol = 7,
	c_aiStateGoDirect = 8,
	c_aiStateRest = 10,
	c_aiStateShutdown = 11,
	c_aiStateDead = 12
};

// The messages a rule waits for (AiRule::m_message).
enum {
	c_aiMessageNone = 0,
	c_aiMessageProx = 1,
	c_aiMessageDist = 2,
	c_aiMessageReach = 3,
	c_aiMessageTrue = 4,
	c_aiMessageFalse = 5,
	c_aiMessageDestroy = 6,
	c_aiMessageTargetable = 7
};

// The transitions a rule makes (AiRule::m_transition).
enum {
	c_aiTransitionNull = 0,
	c_aiTransitionClearStack = 1,
	c_aiTransitionPush = 2,
	c_aiTransitionPop = 3,
	c_aiTransitionNotify = 4,
	c_aiTransitionEvaluate = 5
};

// AI target ids: a type in bits 8-11 and an index in the low byte, or a symbolic target.
enum {
	c_aiTargetNav = 0x100,
	c_aiTargetPlayer = 0x200,
	c_aiTargetThing = 0x400,
	c_aiTargetHome = 0x2100,
	c_aiTargetRbAnchor = 0x2101,
	c_aiTargetUser = 0x2200,
	c_aiTargetMyLeader = 0x2201,
	c_aiTargetMe = 0x2202,
	c_aiTargetFriendly = 0x4200,
	c_aiTargetEnemy = 0x4201
};

// A name for a value, for the AI's log.
struct AiName {
	const char* m_name; // 0x00
	int32_t m_value;    // 0x04
};

// For each script slot, the script (by resource index) for a player that leads its team
// ([1] == 0) or follows ([1] == 1).
typedef int16_t AiScriptTable[16][2];

typedef void (*AiStateFn)(struct Player* p_player, uint16_t p_target);
typedef int16_t (*AiMessageFn)(struct Player* p_player, int16_t p_target, int16_t p_arg);
typedef int32_t (*AiTransitionFn)(struct Player* p_player, AiRule* p_rule);

// The functions and globals of ai.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_debugStar;
	extern int32_t g_debugObjective;
	extern int32_t g_lairdoCheat;
	extern int32_t g_debugLastLine;
	extern int32_t g_debugFirstLine;
	extern int32_t g_debugListedStar;
	extern AiName g_aiMessageNames[8];
	extern AiName g_aiTransitionNames[7];
	extern AiName g_aiStateNames[13];
	extern AiName g_aiStateShortNames[14];
	extern AiName g_aiSymbolicTargetNames[7];
	extern AiName g_aiTargetTypeNames[3];
	extern AiName g_aiTargetTypeLetters[3];
	extern AiName g_shapeKindNames[3];
	extern AiName g_playerTypeNames[9];
	extern AiName g_powerStateNames[8];
	extern AiName g_objectiveTypeNames[15];
	extern AiName g_aiBehaviorNames[14];
	extern int16_t g_invalidTargetLogCount;
	extern AiScriptTable g_mechScripts;
	extern AiScriptTable g_artilleryScripts;
	extern AiScriptTable g_wandererScripts;
	extern AiScriptTable g_truckScripts;
	extern AiScriptTable g_tankScripts;
	extern int16_t g_helicopterScripts[15][2];
	extern int32_t g_aiSpreadTargets;
	extern int32_t g_localStarAssigned;
	extern AiStateFn g_aiStateFns[14];
	extern AiMessageFn g_aiMessageFns[8];
	extern AiTransitionFn g_aiTransitionFns[5];
	extern void* g_aiScripts[10];
	extern int16_t g_hiddenTargetCount;
	extern AiRule* g_aiRules[60][6];
	extern int32_t g_aiStateTime;

	void UpdateAI(struct Player* p_player);
	int32_t RunAIRules(struct Player* p_player);
	void CollectAIRules(struct Player* p_player);
	void FirstAI(void);
	void InitializeAI(struct Player* p_player);
	void ResetAI(struct Player* p_player);
	void TargetAttacker(struct Player* p_player);
	const char* FindAIName(int16_t p_value, AiName* p_names, int16_t p_count);
	void LogPlayerStatusLines(void);
	uint32_t GetPointDistance(int32_t* p_a, int32_t* p_b);
	int32_t ChooseTeamLeader(int32_t p_team);
	void RetargetGoals(int32_t p_team, int32_t p_index, int32_t p_target);
	int16_t ResolveTarget(struct Player* p_player, int16_t p_target);
	int16_t AiMessageTrue(struct Player* p_player, int16_t p_target, int16_t p_arg);
	int16_t AiMessageFalse(struct Player* p_player, int16_t p_target, int16_t p_arg);
	int16_t AiMessageReach(struct Player* p_player, int16_t p_target, int16_t p_arg);
	int16_t AiMessageProx(struct Player* p_player, int16_t p_target, int16_t p_arg);
	void LogPlayerSkillLines(void);
	int16_t AiMessageDist(struct Player* p_player, int16_t p_target, int16_t p_arg);
	void LogStarMissionLines(int32_t p_team);
	int32_t IsTargetDetectable(struct Player* p_player, int16_t p_target, int16_t p_distance, int32_t p_check);
	struct Player* FindNearestTarget(
		struct Player* p_player,
		int16_t p_target,
		int16_t p_arg,
		int16_t* p_nearest,
		int16_t* p_best
	);
	void LogAIStatus(void);
	int16_t AiMessageTargetable(struct Player* p_player, int16_t p_target, int16_t p_arg);
	int16_t AiMessageDestroy(struct Player* p_player, int16_t p_target, int16_t p_arg);
	void AiStateIdle(struct Player* p_player, uint16_t p_target);
	void AiStateMove(struct Player* p_player, uint16_t p_target);
	void AiStateAttack(struct Player* p_player, uint16_t p_target);
	int32_t AiTransitionNull(struct Player* p_player, AiRule* p_rule);
	int32_t AiTransitionClearStack(struct Player* p_player, AiRule* p_rule);
	int32_t AiTransitionPush(struct Player* p_player, AiRule* p_rule);
	int32_t AiTransitionPop(struct Player* p_player, AiRule* p_rule);
	uint16_t FindAttacker(struct Player* p_player);
	int16_t ResolveRuleTarget(uint16_t p_value, int16_t p_goal, int16_t p_found, int16_t p_target);
	int32_t SetTarget(struct Player* p_player, int16_t p_target);
	int32_t GetTargetRange(int16_t p_target);
	int32_t GetApproachThrottle(struct Player* p_player, int32_t p_range);
	int32_t AimTorsoPan(struct Player* p_player, int32_t p_angle, int32_t p_delta);
	int32_t AimTorsoTilt(struct Player* p_player, int32_t p_delta);
	int32_t SteerToTarget(struct Player* p_player);
	void LeaveAIState(struct Player* p_player);
	void EnterAIState(struct Player* p_player, uint16_t p_state);
	int16_t NextTarget(struct Player* p_player, int16_t p_target, int16_t p_previous);
	int32_t GetTargetBearing(struct Player* p_player);
	int32_t IsTargetDone(uint16_t p_target, int32_t p_check);
	void QueueAIState(struct Player* p_player, int16_t p_state, uint16_t p_target);
	int32_t HasAIState(struct Player* p_player, int16_t p_state);
	void SetAIState(struct Player* p_player, int16_t p_state, int16_t p_target, int32_t p_push);
	void ReleaseNavPoints(struct Player* p_player);
	void PlacePatrolNavs(struct Player* p_player);
	void AdvanceNavTarget(struct Player* p_player, int16_t p_target);
	void PlaceFormationNav(struct Player* p_player);
	void RecordAttack(int32_t p_index, uint32_t p_target);
	int32_t LoadAIScripts(void);
	int32_t AiTransitionNotify(struct Player* p_player, AiRule* p_rule);
	int32_t FindStarSlotPlayer(int32_t p_slot);
	void PostAIMessage(struct Player* p_player, int16_t p_message, uint16_t p_target, int16_t p_arg);
	int16_t OrderPlayers(struct Player* p_player, int16_t p_targets, int16_t p_state, int16_t p_target);
	int32_t OrderStarSlot(int32_t p_slot, int16_t p_command);
	int32_t AddClamped(int32_t p_value, int32_t p_delta, int32_t p_sameSign);
	void ResetStarOrders(int32_t p_team);
	void AssignStarObjective(int32_t p_team);
	int16_t GetEngagementAIFlags(int32_t p_value);
	void SetAIScript(struct Player* p_player, int16_t p_slot, int16_t p_script, int16_t p_leader);
	uint16_t GetObjectiveTarget(int32_t p_team, int16_t p_objective, int32_t p_index);
	int32_t GetTeamMembers(int32_t p_team, struct Player** p_members, int32_t p_aliveOnly);
	void HandleStarOrder(int32_t p_team, AiMessage* p_order);
	int32_t AssignStarTarget(int32_t p_team, uint16_t p_target);
	void LeadStar(int32_t p_team);
	void ReleaseAnchorNav(struct Player* p_player);
	int32_t GetLocalStarSize(void);
	void RunStarCommand(int32_t p_command, int32_t p_slot);
	void SweepTorso(struct Player* p_player);

#ifdef __cplusplus
}
#endif

#endif // AI_H
