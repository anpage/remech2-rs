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
	const MechChar* m_name; // 0x00
	MechS32 m_value;        // 0x04
};

// For each script slot, the script (by resource index) for a player that leads its team
// ([1] == 0) or follows ([1] == 1).
typedef MechS16 AiScriptTable[16][2];

typedef void (*AiStateFn)(struct Player* p_player, MechU16 p_target);
typedef MechS16 (*AiMessageFn)(struct Player* p_player, MechS16 p_target, MechS16 p_arg);
typedef MechS32 (*AiTransitionFn)(struct Player* p_player, AiRule* p_rule);

// The functions and globals of ai.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_debugStar;
	extern MechS32 g_debugObjective;
	extern MechS32 g_lairdoCheat;
	extern MechS32 g_debugLastLine;
	extern MechS32 g_debugFirstLine;
	extern MechS32 g_debugListedStar;
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
	extern MechS16 g_invalidTargetLogCount;
	extern AiScriptTable g_mechScripts;
	extern AiScriptTable g_artilleryScripts;
	extern AiScriptTable g_wandererScripts;
	extern AiScriptTable g_truckScripts;
	extern AiScriptTable g_tankScripts;
	extern MechS16 g_helicopterScripts[15][2];
	extern MechS32 g_aiSpreadTargets;
	extern MechS32 g_localStarAssigned;
	extern AiStateFn g_aiStateFns[14];
	extern AiMessageFn g_aiMessageFns[8];
	extern AiTransitionFn g_aiTransitionFns[5];
	extern void* g_aiScripts[10];
	extern MechS16 g_hiddenTargetCount;
	extern AiRule* g_aiRules[60][6];
	extern MechS32 g_aiStateTime;

	void UpdateAI(struct Player* p_player);
	MechS32 RunAIRules(struct Player* p_player);
	void CollectAIRules(struct Player* p_player);
	void FirstAI(void);
	void InitializeAI(struct Player* p_player);
	void ResetAI(struct Player* p_player);
	void TargetAttacker(struct Player* p_player);
	const MechChar* FindAIName(MechS16 p_value, AiName* p_names, MechS16 p_count);
	void ClearAILog(void);
	void LogPlayerStatusLines(void);
	MechU32 GetPointDistance(MechS32* p_a, MechS32* p_b);
	MechS32 ChooseTeamLeader(MechS32 p_team);
	void RetargetGoals(MechS32 p_team, MechS32 p_index, MechS32 p_target);
	MechS16 ResolveTarget(struct Player* p_player, MechS16 p_target);
	MechS16 AiMessageTrue(struct Player* p_player, MechU16 p_target);
	MechS16 AiMessageFalse(void);
	MechS16 AiMessageReach(struct Player* p_player, MechS16 p_target, MechS16 p_arg);
	MechS16 AiMessageProx(struct Player* p_player, MechS16 p_target, MechS16 p_arg);
	void LogPlayerSkillLines(void);
	MechS16 AiMessageDist(struct Player* p_player, MechS16 p_target, MechS16 p_arg);
	void LogStarMissionLines(MechS32 p_team);
	MechS32 IsTargetDetectable(struct Player* p_player, MechS16 p_target, MechS16 p_distance, MechS32 p_check);
	struct Player* FindNearestTarget(
		struct Player* p_player,
		MechS16 p_target,
		MechS16 p_arg,
		MechS16* p_nearest,
		MechS16* p_best
	);
	void LogAIStatus(void);
	MechS16 AiMessageTargetable(struct Player* p_player, MechS16 p_target, MechS16 p_arg);
	MechS16 FUN_100531d1(void);
	MechS16 FUN_100531e4(void);
	MechS16 AiMessageDestroy(struct Player* p_player, MechU16 p_target);
	void AiStateIdle(struct Player* p_player);
	void AiStateMove(struct Player* p_player, MechU16 p_target);
	void AiStateAttack(struct Player* p_player, MechU16 p_target);
	MechS32 AiTransitionNull(struct Player* p_player, AiRule* p_rule);
	MechS32 AiTransitionClearStack(struct Player* p_player, AiRule* p_rule);
	MechS32 AiTransitionPush(struct Player* p_player, AiRule* p_rule);
	MechS32 AiTransitionPop(struct Player* p_player, AiRule* p_rule);
	MechU16 FindAttacker(struct Player* p_player);
	MechS16 ResolveRuleTarget(MechU16 p_value, MechS16 p_goal, MechS16 p_found, MechS16 p_target);
	MechS32 SetTarget(struct Player* p_player, MechS16 p_target);
	MechS32 GetTargetRange(MechS16 p_target);
	MechS32 GetApproachThrottle(struct Player* p_player, MechS32 p_range);
	MechS32 AimTorsoPan(struct Player* p_player, MechS32 p_angle, MechS32 p_delta);
	MechS32 AimTorsoTilt(struct Player* p_player, MechS32 p_delta);
	MechS32 SteerToTarget(struct Player* p_player);
	void LeaveAIState(struct Player* p_player);
	void EnterAIState(struct Player* p_player, MechU16 p_state);
	MechS16 NextTarget(struct Player* p_player, MechS16 p_target, MechS16 p_previous);
	MechS32 GetTargetBearing(struct Player* p_player);
	MechS32 IsTargetDone(MechU16 p_target, MechS32 p_check);
	void QueueAIState(struct Player* p_player, MechS16 p_state, MechU16 p_target);
	MechS32 HasAIState(struct Player* p_player, MechS16 p_state);
	void SetAIState(struct Player* p_player, MechS16 p_state, MechS16 p_target, MechS32 p_push);
	void ReleaseNavPoints(struct Player* p_player);
	void PlacePatrolNavs(struct Player* p_player);
	void AdvanceNavTarget(struct Player* p_player, MechS16 p_target);
	void PlaceFormationNav(struct Player* p_player);
	void RecordAttack(MechS32 p_index, MechU32 p_target);
	MechS32 LoadAIScripts(void);
	MechS32 AiTransitionNotify(struct Player* p_player, AiRule* p_rule);
	MechS32 FindStarSlotPlayer(MechS32 p_slot);
	void PostAIMessage(struct Player* p_player, MechS16 p_message, MechU16 p_target, MechS16 p_arg);
	MechS16 OrderPlayers(struct Player* p_player, MechS16 p_targets, MechS16 p_state, MechS16 p_target);
	MechS32 OrderStarSlot(MechS32 p_slot, MechS16 p_command);
	MechS32 AddClamped(MechS32 p_value, MechS32 p_delta, MechS32 p_sameSign);
	void ResetStarOrders(MechS32 p_team);
	void AssignStarObjective(MechS32 p_team);
	MechS16 GetEngagementAIFlags(MechS32 p_value);
	void SetAIScript(struct Player* p_player, MechS16 p_slot, MechS16 p_script, MechS16 p_leader);
	MechU16 GetObjectiveTarget(MechS32 p_team, MechS16 p_objective, MechS32 p_index);
	MechS32 GetTeamMembers(MechS32 p_team, struct Player** p_members, MechS32 p_aliveOnly);
	void HandleStarOrder(MechS32 p_team, AiMessage* p_order);
	MechS32 AssignStarTarget(MechS32 p_team, MechU16 p_target);
	void LeadStar(MechS32 p_team);
	void ReleaseAnchorNav(struct Player* p_player);
	MechS32 GetLocalStarSize(void);
	void RunStarCommand(MechS32 p_command, MechS32 p_slot);
	void SweepTorso(struct Player* p_player);

#ifdef __cplusplus
}
#endif

#endif // AI_H
