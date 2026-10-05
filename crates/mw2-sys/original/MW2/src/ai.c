#include "ai.h"

#include "aiweapons.h"
#include "clock.h"
#include "debugbreak.h"
#include "decomp.h"
#include "eyepoint.h"
#include "fixedmul.h"
#include "geocache.h"
#include "lineofsight.h"
#include "loadres.h"
#include "maneuvers.h"
#include "mw2log.h"
#include "mw2prj.h"
#include "network.h"
#include "objective.h"
#include "overlay.h"
#include "players.h"
#include "random.h"
#include "shots.h"
#include "simmain.h"
#include "speech.h"
#include "targeting.h"
#include "team.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The AI: each AI player runs a state machine (c_aiState…) driven by scripts, AIT resources
// of rules. A script lists, for each state, the rules that apply in it; a rule waits for a
// message (c_aiMessage…) about a target, and on it makes a transition (c_aiTransition…) and
// enters a new state with a new target. Targets are ids: a type (c_aiTarget…) in bits 8-11 and
// an index in the low byte, or one of the symbolic targets (the agp_… names) the rules resolve
// against the player (NextTarget). The leader of each star (team) steers the others towards
// the star's current mission objective (LeadStar). tools/dump_ait.py prints the scripts from
// MW2.PRJ.

DECOMP_SIZE_ASSERT(AiRule, 0x0c)
DECOMP_SIZE_ASSERT(Player, 0x1aa)
DECOMP_SIZE_ASSERT(PlayerTargetInfo, 0x28)

// GLOBAL: MW2 0x100a88f0
MechS32 g_debugStar = -1;

// GLOBAL: MW2 0x100a88f4
MechS32 g_debugObjective = 0;

// GLOBAL: MW2 0x100a88f8
MechS32 g_debugLastLine = -1;

// GLOBAL: MW2 0x100a88fc
MechS32 g_debugFirstLine = -1;

// GLOBAL: MW2 0x100a8900
MechS32 g_debugListedStar = -1;

// GLOBAL: MW2 0x100a8908
AiName g_aiMessageNames[8] = {
	{"NO_MESSAGE", c_aiMessageNone},
	{"M_PROX", c_aiMessageProx},
	{"M_DIST", c_aiMessageDist},
	{"M_REACH", c_aiMessageReach},
	{"M_TRUE", c_aiMessageTrue},
	{"M_FALSE", c_aiMessageFalse},
	{"M_DESTROY", c_aiMessageDestroy},
	{"M_TGTABLE", c_aiMessageTargetable},
};

// GLOBAL: MW2 0x100a8948
AiName g_aiTransitionNames[7] = {
	{"T_NULL", c_aiTransitionNull},
	{"T_CLEAR_STACK", c_aiTransitionClearStack},
	{"T_PUSH", c_aiTransitionPush},
	{"T_POP", c_aiTransitionPop},
	{"T_NOTIFY", c_aiTransitionNotify},
	{"T_EVALUATE", c_aiTransitionEvaluate},
	{"nothing", -1},
};

// GLOBAL: MW2 0x100a8980
AiName g_aiStateNames[13] = {
	{"idle", c_aiStateIdle},
	{"avoid", c_aiStateAvoid},
	{"target", c_aiStateTarget},
	{"attack", c_aiStateAttack},
	{"flee", c_aiStateFlee},
	{"follow", c_aiStateFollow},
	{"recon", c_aiStateRecon},
	{"patrol", c_aiStatePatrol},
	{"godirect", c_aiStateGoDirect},
	{"rest", c_aiStateRest},
	{"shutdown", c_aiStateShutdown},
	{"dead", c_aiStateDead},
	{NULL, 0},
};

// GLOBAL: MW2 0x100a89e8
AiName g_aiStateShortNames[14] = {
	{"nil", -1},
	{"idl", c_aiStateIdle},
	{"avd", c_aiStateAvoid},
	{"tgt", c_aiStateTarget},
	{"atk", c_aiStateAttack},
	{"fle", c_aiStateFlee},
	{"fol", c_aiStateFollow},
	{"rec", c_aiStateRecon},
	{"ptl", c_aiStatePatrol},
	{"god", c_aiStateGoDirect},
	{"res", c_aiStateRest},
	{"shu", c_aiStateShutdown},
	{"ded", c_aiStateDead},
	{NULL, 0},
};

// GLOBAL: MW2 0x100a8a58
AiName g_aiSymbolicTargetNames[7] = {
	{"agp_user", c_aiTargetUser},
	{"agp_myleader", c_aiTargetMyLeader},
	{"agp_me", c_aiTargetMe},
	{"agp_home", c_aiTargetHome},
	{"agp_rbanchor", c_aiTargetRbAnchor},
	{"agp_friendly", c_aiTargetFriendly},
	{"agp_enemy", c_aiTargetEnemy},
};

// GLOBAL: MW2 0x100a8a90
AiName g_aiTargetTypeNames[3] = {
	{"nv", c_aiTargetNav},
	{"gp", c_aiTargetPlayer},
	{"gt", c_aiTargetThing},
};

// GLOBAL: MW2 0x100a8aa8
AiName g_aiTargetTypeLetters[3] = {
	{"n", c_aiTargetNav},
	{"p", c_aiTargetPlayer},
	{"t", c_aiTargetThing},
};

// GLOBAL: MW2 0x100a8ac0
AiName g_shapeKindNames[3] = {
	{"p", 0x100},
	{"t", 0x200},
	{"r", 0x800},
};

// GLOBAL: MW2 0x100a8ad8
AiName g_playerTypeNames[9] = {
	{"GP_NULL", 0},
	{"GP_MW2MECH", 1},
	{"GP_MW2TRUCK", 2},
	{"GP_MW2ARTILLERY", 3},
	{"GP_MW2TANK", 4},
	{"GP_MW2HELICOPTER", 5},
	{"GP_MW2WANDERER", 6},
	{"GP_MW2DOOR", 7},
	{NULL, 0},
};

// GLOBAL: MW2 0x100a8b20
AiName g_powerStateNames[8] = {
	{"of", 0},
	{"su", 1},
	{"nm", 2},
	{"sd", 3},
	{"dd", 4},
	{"ej", 5},
	{"co", 6},
	{"ds", 7},
};

// GLOBAL: MW2 0x100a8b60
AiName g_objectiveTypeNames[15] = {
	{"non", 0},
	{"ann", 1},
	{"des", 2},
	{"def", 4},
	{"rec", 8},
	{"beg", 0x10},
	{"ret", 0x20},
	{"sur", 0x40},
	{"end", 0x80},
	{"got", 0x100},
	{"wai", 0x200},
	{"sle", 0x400},
	{"shu", 0x800},
	{"xpl", 0x1000},
	{"lev", 0x2000},
};

// GLOBAL: MW2 0x100a8bd8
AiName g_aiBehaviorNames[14] = {
	{"      ", -1},
	{"stupid", 0},
	{"behind", 1},
	{"achick", 2},
	{"asrp  ", 3},
	{"ajmpin", 4},
	{"adfa  ", 5},
	{"kama  ", 6},
	{"wchick", 7},
	{"wbackp", 8},
	{"wpeek ", 9},
	{"avoid ", 10},
	{"sprint", 11},
	{"circle", 12},
};

// GLOBAL: MW2 0x100a8c48
MechS16 g_invalidTargetLogCount = 0;

// GLOBAL: MW2 0x100a8f38
AiScriptTable g_mechScripts = {
	{1, 1},
	{7, 3},
	{7, 3},
	{6, 2},
	{9, 5},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{1, 1},
	{1, 1},
	{8, 4},
	{0, 0},
};

// GLOBAL: MW2 0x100a8f78
AiScriptTable g_artilleryScripts = {
	{1, 1},
	{7, 3},
	{7, 3},
	{6, 2},
	{9, 5},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{1, 1},
	{1, 1},
	{8, 4},
	{0, 0},
};

// GLOBAL: MW2 0x100a8fb8
AiScriptTable g_wandererScripts = {
	{1, 1},
	{7, 3},
	{7, 3},
	{6, 2},
	{9, 5},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{1, 1},
	{1, 1},
	{8, 4},
	{0, 0},
};

// GLOBAL: MW2 0x100a8ff8
AiScriptTable g_truckScripts = {
	{1, 1},
	{7, 3},
	{7, 3},
	{6, 2},
	{9, 5},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{1, 1},
	{1, 1},
	{8, 4},
	{0, 0},
};

// GLOBAL: MW2 0x100a9038
AiScriptTable g_tankScripts = {
	{1, 1},
	{7, 3},
	{7, 3},
	{6, 2},
	{9, 5},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{1, 1},
	{1, 1},
	{8, 4},
	{0, 0},
};

// GLOBAL: MW2 0x100a9078
MechS16 g_helicopterScripts[15][2] = {
	{1, 1},
	{7, 3},
	{7, 3},
	{6, 2},
	{9, 5},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{9, 5},
	{1, 1},
	{1, 1},
	{1, 1},
	{1, 1},
	{8, 4},
};

// GLOBAL: MW2 0x100a90b4
MechS32 g_lairdoCheat = 0;

// GLOBAL: MW2 0x100a90b8
MechS32 g_aiSpreadTargets = 1;

// GLOBAL: MW2 0x100a90c0
MechS32 g_localStarAssigned = 0;

// GLOBAL: MW2 0x100a90c8
AiStateFn g_aiStateFns[14] = {
	AiStateIdle,
	AiStateMove,
	AiStateMove,
	AiStateAttack,
	AiStateMove,
	AiStateMove,
	AiStateMove,
	AiStateMove,
	AiStateMove,
	AiStateMove,
	AiStateMove,
	AiStateIdle,
	NULL,
	NULL,
};

// GLOBAL: MW2 0x100a9100
AiMessageFn g_aiMessageFns[8] = {
	NULL,
	AiMessageProx,
	AiMessageDist,
	AiMessageReach,
	AiMessageTrue,
	AiMessageFalse,
	AiMessageDestroy,
	AiMessageTargetable,
};

// GLOBAL: MW2 0x100a9120
AiTransitionFn g_aiTransitionFns[5] = {
	AiTransitionNull,
	AiTransitionClearStack,
	AiTransitionPush,
	AiTransitionPop,
	AiTransitionNotify,
};

// GLOBAL: MW2 0x100e9e10
void* g_aiScripts[10];

// GLOBAL: MW2 0x100e9e38
MechS16 g_hiddenTargetCount;

// GLOBAL: MW2 0x100e9e40
AiRule* g_aiRules[60][6];

// GLOBAL: MW2 0x100ea3e0
MechS32 g_aiStateTime;

// Runs p_player's AI for a tick: the star leader leads its star (LeadStar); an AI player runs
// its rules and, unless one changed its state, its state's function; a player the AI doesn't
// drive records an attack on a side-1 player its mech runs into (RecordAttack).
// reccmp leaves the call through g_aiStateFns unresolved (its call regex only matches a bare
// [address]), so both sides show raw addresses.
// FUNCTION: MW2 0x10051100
void UpdateAI(Player* p_player)
{
	if (p_player->m_aiMode == 1) {
		return;
	}

	if (p_player->m_index == g_localPlayerId && g_monoEnabled) {
		LogAIStatus();
	}

	if (p_player->m_index == 0) {
		g_hiddenTargetCount = 0;
	}

	if (p_player->m_ai.m_state == c_aiStateDead) {
		return;
	}

	if (GetTeamLeader(p_player->m_team) == p_player->m_index) {
		LeadStar(p_player->m_team);
	}

	if (p_player->m_aiMode != 2) {
		if (p_player->m_mech->m_collisionTicks && p_player->m_collidedWith != -1 &&
			GetPlayerSide(p_player->m_collidedWith) == 1) {
			RecordAttack(p_player->m_index, g_players[p_player->m_collidedWith]->m_index);
		}
	}
	else {
		if (!RunAIRules(p_player)) {
			g_aiStateFns[p_player->m_ai.m_state](p_player, p_player->m_ai.m_target);
		}

		SetTarget(p_player, p_player->m_ai.m_goal);
	}
}

// Matches p_player's rules against the posted message and their own messages; the first that
// fires makes its transition and, unless that says otherwise, enters its new state. Whether one
// did.
// reccmp leaves the call through g_aiMessageFns and g_aiTransitionFns unresolved (its call regex
// only matches a bare [address]), so both sides show raw addresses. Stack-slot permutation: arg,
// i, rule and target and valid.
// FUNCTION: MW2 0x10051251
MechS32 RunAIRules(Player* p_player)
{
	AiRule* rule;
	MechS16 target;
	MechS16 arg;
	MechS16 i;
	MechS32 posted;
	MechS32 valid;

	i = 0;
	posted = FALSE;
	TargetAttacker(p_player);

	while (p_player->m_rules[i]) {
		rule = p_player->m_rules[i++];
		target = ResolveRuleTarget(rule->m_target, p_player->m_ai.m_goal, 0, p_player->m_ai.m_target);
		arg = ResolveRuleTarget(rule->m_arg, p_player->m_ai.m_goal, 0, p_player->m_ai.m_target);

		if (rule->m_message == p_player->m_ai.m_posted.m_message && p_player->m_ai.m_posted.m_target == target) {
			posted = TRUE;
			p_player->m_ai.m_posted.m_message = 0;
		}

		if (posted ||
			(valid = rule->m_message < 8 && (target = g_aiMessageFns[rule->m_message](p_player, target, arg)))) {
			if (g_aiTransitionFns[rule->m_transition](p_player, rule)) {
				SetAIState(
					p_player,
					rule->m_newState,
					ResolveRuleTarget(rule->m_newTarget, p_player->m_ai.m_goal, target, p_player->m_ai.m_target),
					0
				);
				return TRUE;
			}
		}
	}

	return FALSE;
}

// Collects the rules of p_player's state from its scripts into m_rules, the higher-priority
// script's first; a rule for the same message and target as one collected is skipped.
// Stack-slot permutation: count, found, i, j, pos, rule and script and skip.
// FUNCTION: MW2 0x1005141c
void CollectAIRules(Player* p_player)
{
	MechU16 count;
	MechU32 found;
	MechS16 j;
	MechS16 pos;
	MechS16 i;
	MechS16 skip;
	MechU16* script;
	AiRule* rule;
	MechS16 ruleCount;

	p_player->m_rules[0] = NULL;

	for (i = 2; i >= 0; i--) {
		script = p_player->m_ruleSets[i];
		if (!script) {
			continue;
		}

		pos = 0;
		count = script[pos++];
		skip = ruleCount = 0;

		while (count--) {
			found = script[pos++] == p_player->m_ai.m_state;
			if (found) {
				ruleCount = script[pos];
				break;
			}
			else {
				skip += script[pos];
				pos++;
			}
		}

		if (!found) {
			continue;
		}

		pos = *script * 4 + 2;
		pos = pos + skip * 12;
		rule = (AiRule*) ((MechU8*) script + pos);

		for (pos = 0; pos < ruleCount; pos++, rule++) {
			if (!rule->m_message) {
				break;
			}

			j = 0;
			found = FALSE;
			while (!found && p_player->m_rules[j]) {
				if (p_player->m_rules[j]->m_target == rule->m_target &&
					p_player->m_rules[j]->m_message == rule->m_message) {
					found = TRUE;
				}

				j++;
			}

			if (!found) {
				p_player->m_rules[j] = rule;
				p_player->m_rules[j + 1] = NULL;
			}
		}
	}
}

// Operand order: the loop test (i < g_objectiveCount) compares with i in eax in the original.
// FUNCTION: MW2 0x1005166e
void FirstAI(void)
{
	MechS32 i;

	if (g_monoEnabled) {
	}

	for (i = 0; i < g_objectiveCount; i++) {
		AssignTeamSlots(i, 0);
		ResetStarOrders(i);
	}
}

// Sets up p_player's AI: its rules, state and anchor nav, whether it may engage (an enemy of the
// local player's team) and, for an AI player, its scripts and its skills and ranges from the gpspec's
// AI parameters.
// FUNCTION: MW2 0x100516c5
void InitializeAI(Player* p_player)
{
	p_player->m_rules = g_aiRules[p_player->m_index];
	p_player->m_ai.m_state = c_aiStateIdle;
	p_player->m_nav = 0x1000;
	ResetAI(p_player);

	if (g_players[g_localPlayerId]->m_team != p_player->m_team) {
		p_player->m_engageAtWill = 1;
	}
	else {
		p_player->m_engageAtWill = 0;
	}

	if (p_player->m_aiMode == 2) {
		SetAIScript(p_player, 0, 0, GetTeamLeader(p_player->m_team));

		p_player->m_gunnery = p_player->m_aiParams[0];
		if (!p_player->m_gunnery) {
			p_player->m_gunnery = 1;
		}

		p_player->m_leashRange = p_player->m_aiParams[1];
		if (!p_player->m_leashRange) {
			p_player->m_leashRange = 250;
		}

		p_player->m_restAlertRange = p_player->m_aiParams[2];
		if (!p_player->m_restAlertRange) {
			p_player->m_restAlertRange = 250;
		}

		p_player->m_alertRange = p_player->m_aiParams[3];
		if (!p_player->m_alertRange) {
			p_player->m_alertRange = 250;
		}

		p_player->m_piloting = p_player->m_aiParams[4];
		if (!p_player->m_piloting) {
			p_player->m_piloting = 1;
		}

		p_player->m_gunnery++;
		p_player->m_leashRange *= 100;
		p_player->m_restAlertRange *= 100;
		p_player->m_alertRange *= 100;
	}

	InitializeManeuvers(p_player);
}

// Clears p_player's AI state, target, goal, scripts, stack and posted message.
// FUNCTION: MW2 0x100518cd
void ResetAI(Player* p_player)
{
	MechS32 i;

	LeaveAIState(p_player);
	p_player->m_ai.m_target = 0;
	p_player->m_ai.m_goal = 0;
	p_player->m_ai.m_flags = 0;

	if (p_player->m_aiMode != 2) {
		p_player->m_ai.m_state = c_aiStateIdle;
	}
	else {
		p_player->m_ai.m_state = c_aiStateDead;
	}

	for (i = 0; i < 3; i++) {
		SetAIScript(p_player, i, -1, 0);
	}

	p_player->m_rules[0] = NULL;
	p_player->m_stackCount = 0;
	p_player->m_ai.m_posted.m_message = 0;
	p_player->m_nextFireTime = 0;
	ReleaseAnchorNav(p_player);
	CollectAIRules(p_player);
}

// Turns p_player on a player attacking it (FindAttacker), unless its AI flags hold it (bit 0) or
// it is busy with that player already, calling off the teammate on it (FindNearestTarget).
// Stack-slot permutation: best, nearest and other and target.
// FUNCTION: MW2 0x100519b3
void TargetAttacker(Player* p_player)
{
	MechS16 target;
	MechS16 nearest;
	Player* other;
	MechS16 best;

	if (p_player->m_ai.m_flags & 1) {
		return;
	}

	target = FindAttacker(p_player);
	if (target != -1 && (p_player->m_ai.m_goal != p_player->m_nav || p_player->m_ai.m_state != c_aiStateGoDirect) &&
		((p_player->m_ai.m_state != c_aiStateTarget && p_player->m_ai.m_state != c_aiStateAttack) ||
		 p_player->m_ai.m_goal != target)) {
		other = FindNearestTarget(p_player, target, -3, &best, &nearest);
		if (other) {
			if (other->m_ai.m_flags & 0x40) {
				target = -1;
			}
			else {
				SetAIState(other, c_aiStateIdle, 0, 0);
			}
		}

		if (target != -1) {
			SetAIState(p_player, c_aiStateTarget, target, 1);
		}
	}
}

// Returns the name of p_value in p_names, or NULL.
// FUNCTION: MW2 0x10051ad8
const MechChar* FindAIName(MechS16 p_value, AiName* p_names, MechS16 p_count)
{
	MechS16 i;

	for (i = 0; i < p_count; i++) {
		if (p_names[i].m_value == p_value) {
			return p_names[i].m_name;
		}
	}

	return NULL;
}

// Clears the AI log page's lines (the drawing is compiled out).
// FUNCTION: MW2 0x10051b35
void ClearAILog(void)
{
	MechS32 i;

	for (i = 3; i < 25; i++) {
		if (g_monoEnabled) {
		}
	}
}

// Stack-slot permutation: behavior, color, i, input, line, objective, player, saved,
// secondsLeft, sensor and steering and target. The original addresses player at [ebp-0x1b4]
// (disp32), so its code is longer and reccmp compares only the recompiled length of it. Operand
// order: the loop test (i < g_playerCount) compares with i in eax in the original. The four
// inputs are summed in a different order (commutative).
// FUNCTION: MW2 0x10051b6e
void LogPlayerStatusLines(void)
{
	const MechChar* behavior;
	const MechChar* sensor;
	MechChar invalidLine[80];
	PlayerSteering* steering;
	MechS8 input;
	MechS32 secondsLeft;
	MechS32 color;
	MechS32 i;
	MissionObjective* objective;
	MechChar line[100];
	Player* player;
	Player saved;
	MechS16 target;

	if (g_monoEnabled) {
	}

	if (g_monoEnabled) {
	}

	for (i = 0; i < g_playerCount; i++) {
		input = ' ';
		player = g_players[i];
		objective = &g_objectiveTable[player->m_team].m_objectives[g_currentObjective[player->m_team]];
		if (g_currentObjective[player->m_team] == -1 || objective->m_timeLimit < 0) {
			secondsLeft = 0;
		}
		else {
			secondsLeft = objective->m_timeLimit - (g_missionTime - objective->m_startTime);
		}

		if (player->m_index == g_localPlayerId) {
			target = player->m_targetInfo.m_target;
		}
		else {
			target = player->m_ai.m_target;
		}

		saved = *player;
		SetTarget(player, target);
		if (player->m_targetInfo.m_distance < 0) {
			DebugBreakpoint();
		}

		if (player->m_steering->m_jumpJetEnabled) {
			steering = player->m_steering;
			if (steering->m_jumpJetFireLeft) {
				input = '<';
			}
			else if (steering->m_jumpJetFireRight) {
				input = '>';
			}
			else if (steering->m_jumpJetFireForward) {
				input = '^';
			}
			else if (steering->m_jumpJetFireBackward) {
				input = 'v';
			}
			else {
				input = '+';
			}

			if (steering->m_jumpJetFireLeft + steering->m_jumpJetFireRight + steering->m_jumpJetFireForward +
					steering->m_jumpJetFireBackward >
				1) {
				input = '?';
			}
		}

		if (player->m_avoidShape) {
			sensor = FindAIName(player->m_avoidShape->m_kind & 0xf00, g_shapeKindNames, 3);
		}
		else {
			sensor = " ";
		}

		if (player->m_ai.m_state == c_aiStateAttack) {
			behavior = FindAIName(player->m_maneuver, g_aiBehaviorNames, 14);
		}
		else {
			behavior = "    ";
		}

		sprintf(
			line,
			"%c%2.2d/%1.1d % 2.2d/%3.3s/%3.3d  %3.3s %1.1s% 3.3d %1.1s %3.3d %4.4s %6.6d %4.4d %2.2s %4.4d %5.4d "
			"%10.10s %c%c%1.1s",
			player == g_localPlayer ? '>' : ' ',
			player->m_index,
			player->m_team,
			g_currentObjective[player->m_team],
			FindAIName(objective->m_type, g_objectiveTypeNames, 15),
			secondsLeft,
			FindAIName(player->m_ai.m_state, g_aiStateShortNames, 13),
			FindAIName(target & 0xf00, g_aiTargetTypeLetters, 3),
			target & 0xff,
			FindAIName(player->m_ai.m_goal & 0xf00, g_aiTargetTypeLetters, 3),
			player->m_ai.m_goal & 0xff,
			behavior,
			player->m_targetInfo.m_distance,
			player->m_steering->m_throttle,
			FindAIName(player->m_mech->m_powerState, g_powerStateNames, 8),
			player->m_mech->m_heat >> 16,
			player->m_steering->m_turn >> 16,
			player->m_name,
			input,
			player->m_steering->m_weaponFire ? '*' : ' ',
			sensor
		);

		color = 7;
		if (!(player->m_mech->m_powerState & 4)) {
			color |= 8;
		}

		if (player->m_index + 4 < 25 && g_monoEnabled) {
		}

		if ((player->m_ai.m_goal == 0 || player->m_ai.m_target == 0) && ++g_invalidTargetLogCount < 10) {
			sprintf(
				invalidLine,
				"%6d : %2d **** Mech %2d has invalid target\n",
				g_currentClock,
				player->m_team,
				player->m_index
			);
			WriteToMw2Log(invalidLine);
		}

		player->m_targetInfo = saved.m_targetInfo;
	}
}

// Returns the distance between the points p_a and p_b.
// Stack-slot permutation: distance and unused.
// FUNCTION: MW2 0x100520d7
MechU32 GetPointDistance(MechS32* p_a, MechS32* p_b)
{
	MechS32 unused;
	MechU32 distance;

	GetBearingAndRange(p_a[0] - p_b[0], p_a[1] - p_b[1], p_a[2] - p_b[2], &unused, &unused, &distance, &unused);
	return distance;
}

// Makes the first able member of star p_team other than its leader the leader. Returns it, or -1.
// Operand order: the loop test (i < count) compares with i in eax in the original. Stack-slot
// permutation: count, i, member and members and newLeader.
// FUNCTION: MW2 0x1005212a
MechS32 ChooseTeamLeader(MechS32 p_team)
{
	Player* member;
	MechS32 newLeader;
	MechS32 count;
	Player* members[8];
	MechS32 i;
	MechS32 leader;

	leader = GetTeamLeader(p_team);
	newLeader = -1;
	count = GetTeamMembers(p_team, members, TRUE);

	if (count) {
		for (i = 0; i < count; i++) {
			member = members[i];
			if (member->m_index == leader) {
				continue;
			}

			if (!(member->m_flags & 6)) {
				newLeader = member->m_index;
				break;
			}
		}

		SetTeamLeader(p_team, newLeader);
	}

	return newLeader;
}

// Points the goal of every member of star p_team that has player p_index as its goal at
// player p_target instead.
// Operand order: the loop test (i < g_playerCount) compares with i in eax in the original.
// Stack-slot permutation: i and player.
// FUNCTION: MW2 0x100521e0
void RetargetGoals(MechS32 p_team, MechS32 p_index, MechS32 p_target)
{
	MechS32 i;
	Player* player;

	for (i = 0; i < g_playerCount; i++) {
		player = g_players[i];
		if (player->m_team == p_team && !(player->m_ai.m_goal & 0xf000) && (player->m_ai.m_goal & c_aiTargetPlayer) &&
			(player->m_ai.m_goal & 0xff) == p_index) {
			player->m_ai.m_goal = p_target | c_aiTargetPlayer;
		}
	}
}

// Resolves the symbolic target p_target for p_player (FindNearestTarget).
// FUNCTION: MW2 0x10052284
MechS16 ResolveTarget(Player* p_player, MechS16 p_target)
{
	MechS16 nearest;
	MechS16 best;

	if (!(p_target & 0xf000)) {
		return p_target;
	}

	FindNearestTarget(p_player, p_target, -3, &nearest, &best);
	if (p_player->m_ai.m_state == c_aiStateTarget) {
		if (nearest) {
			return nearest;
		}

		if (!g_aiSpreadTargets) {
			return best;
		}

		return 0;
	}

	return best;
}

// The rule message functions (g_aiMessageFns): each returns the target the message matches
// for p_player, or 0. M_TRUE always matches. The original declared some of them with fewer
// parameters (or an unsigned target) and cast them into the table.
// FUNCTION: MW2 0x10052311
MechS16 AiMessageTrue(Player* p_player, MechS16 p_target, MechS16 p_arg)
{
	return p_target;
}

// M_FALSE never matches.
// FUNCTION: MW2 0x10052325
MechS16 AiMessageFalse(Player* p_player, MechS16 p_target, MechS16 p_arg)
{
	return 0;
}

// M_REACH: whether p_player is within its range of p_target (p_arg hundreds, or the target's
// range); reaching its anchor nav releases it, and a patrol moves on to the next nav.
// FUNCTION: MW2 0x10052338
MechS16 AiMessageReach(Player* p_player, MechS16 p_target, MechS16 p_arg)
{
	MechS16 result;

	if (p_arg == 0) {
		result = AiMessageProx(p_player, p_target, GetTargetRange(p_target) / 100);
		if (result && (p_target & c_aiTargetNav)) {
			if (p_player->m_nav == p_target) {
				ReleaseAnchorNav(p_player);
			}

			if (p_player->m_ai.m_state == c_aiStatePatrol) {
				AdvanceNavTarget(p_player, p_target);
				p_player->m_ai.m_target = p_player->m_targetInfo.m_target;
				if (p_player->m_targetInfo.m_target & 0x1000) {
					AdvanceNavTarget(p_player, c_aiTargetNav);
				}
			}
			else {
				AdvanceNavTarget(p_player, p_target);
			}
		}
	}
	else {
		result = AiMessageProx(p_player, p_target, p_arg);
	}

	return result;
}

// M_PROX: the nearest of p_target's targets closer than p_arg hundreds.
// Operand order: the final test (nearest < limit) compares with nearest in eax in the original.
// Stack-slot permutation: limit, nearest, nearestTarget and result and target.
// FUNCTION: MW2 0x10052445
MechS16 AiMessageProx(Player* p_player, MechS16 p_target, MechS16 p_arg)
{
	MechS16 nearestTarget;
	MechS16 result;
	MechS32 limit;
	MechS32 nearest;
	MechS16 target;

	result = 0;
	nearestTarget = -1;
	target = -1;

	if (p_arg == -3) {
		limit = 100000000;
	}
	else {
		limit = p_arg * 100;
	}

	nearest = limit;
	while ((target = NextTarget(p_player, p_target, target)) != -1) {
		if (!IsTargetDone(target, 2) && SetTarget(p_player, target) && p_player->m_targetInfo.m_distance < nearest) {
			nearest = p_player->m_targetInfo.m_distance;
			nearestTarget = target;
		}
	}

	if (nearest < limit && nearestTarget != -1) {
		result = nearestTarget;
	}

	return result;
}

// Operand order: the loop test (i < g_playerCount) compares with i in eax in the original. The
// player == g_localPlayer subtraction runs the other way. Stack-slot permutation: color, i and
// line and player.
// FUNCTION: MW2 0x1005253c
void LogPlayerSkillLines(void)
{
	MechS32 color;
	MechS32 i;
	MechChar line[100];
	Player* player;

	if (g_monoEnabled) {
	}

	for (i = 0; i < g_playerCount; i++) {
		player = g_players[i];
		sprintf(
			line,
			"%c%2.2d/%1.1d %1.1d-%1.1d%67.10s",
			player == g_localPlayer ? '>' : ' ',
			player->m_index,
			player->m_team,
			player->m_piloting,
			player->m_gunnery,
			player->m_name
		);

		color = 7;
		if (!(player->m_mech->m_powerState & 4)) {
			color |= 8;
		}

		if (player->m_index + 4 < 25 && g_monoEnabled) {
		}
	}
}

// M_DIST: the farthest of p_target's targets farther than p_arg hundreds (-4: p_player's leash
// range, -2: its alert range).
// Operand order: the final test (limit < farthest) compares the other way in the original.
// Stack-slot permutation: farthest, farthestTarget, limit and result and target.
// FUNCTION: MW2 0x10052617
MechS16 AiMessageDist(Player* p_player, MechS16 p_target, MechS16 p_arg)
{
	MechS16 farthestTarget;
	MechS16 result;
	MechS32 limit;
	MechS32 farthest;
	MechS16 target;

	result = 0;
	farthestTarget = -1;
	target = -1;

	if (p_arg == -3) {
		limit = 0;
	}
	else if (p_arg == -4) {
		limit = p_player->m_leashRange;
	}
	else if (p_arg == -2) {
		limit = p_player->m_alertRange;
	}
	else if (p_arg == 0) {
		limit = GetTargetRange(p_target);
	}
	else {
		limit = p_arg * 100;
	}

	farthest = limit;
	while ((target = NextTarget(p_player, p_target, target)) != -1) {
		if (!IsTargetDone(target, 2) && SetTarget(p_player, target) && farthest < p_player->m_targetInfo.m_distance) {
			farthest = p_player->m_targetInfo.m_distance;
			farthestTarget = target;
		}
	}

	if (limit < farthest && farthestTarget != -1) {
		result = farthestTarget;
	}

	return result;
}

// Operand order: g_debugFirstLine > g_debugObjective compares the other way in the original.
// Stack-slot permutation: activity, color, line, marker, mission, objective, prefix, priority
// and secondsLeft and typeName. (which also moves the jump table targets).
// FUNCTION: MW2 0x1005276a
void LogStarMissionLines(MechS32 p_team)
{
	const MechChar* activity;
	StarMission* mission;
	MechChar prefix[256];
	MechS32 color;
	MechS32 i;
	const MechChar* typeName;
	MechS32 secondsLeft;
	MechChar line[256];
	const MechChar* marker;
	MissionObjective* objective;
	const MechChar* priority;

	mission = &g_objectiveTable[p_team];
	color = 7;
	switch (mission->m_status) {
	case 0:
		if (mission->m_timeLimit > 0) {
			secondsLeft = mission->m_timeLimit - (g_missionTime - mission->m_startTime);
			sprintf(line, "Star %d mission: %d sec left", p_team, secondsLeft);
		}
		else {
			sprintf(line, "Star %d mission: in progress", p_team);
		}
		break;
	case 2:
		sprintf(line, "Star %d mission: Successful at %d sec", p_team, mission->m_endTime);
		break;
	case 3:
		sprintf(line, "Star %d mission: Failed at %d sec", p_team, mission->m_endTime);
		break;
	case 4:
		sprintf(line, "Star %d mission: Out of time at %d sec", p_team, mission->m_endTime);
		break;
	}

	if (g_monoEnabled) {
	}

	if (g_debugFirstLine == -1) {
		g_debugFirstLine = 0;
		g_debugLastLine = mission->m_objectiveCount - 1 < 20 ? mission->m_objectiveCount - 1 : 20;
	}

	if (g_debugFirstLine > g_debugObjective) {
		if (g_debugObjective > 0) {
			g_debugFirstLine = g_debugObjective;
			g_debugLastLine = mission->m_objectiveCount - 1 < g_debugFirstLine + 20 ? mission->m_objectiveCount - 1
																					: g_debugFirstLine + 20;
			ClearAILog();
		}
		else {
			g_debugObjective = g_debugFirstLine;
		}
	}
	else if (g_debugLastLine < g_debugObjective) {
		if (mission->m_objectiveCount > g_debugObjective) {
			g_debugLastLine = g_debugObjective;
			g_debugFirstLine = g_debugLastLine - 20 > 0 ? g_debugLastLine - 20 : 0;
			ClearAILog();
		}
		else {
			g_debugObjective = g_debugLastLine;
		}
	}

	for (i = g_debugFirstLine; i <= g_debugLastLine; i++) {
		objective = &g_objectiveTable[p_team].m_objectives[i];

		color = 7;
		if (objective->m_active) {
			color |= 8;
		}

		switch (objective->m_priority) {
		case 1:
			priority = "Primary  ";
			break;
		case 2:
			priority = "Secondary";
			break;
		case 4:
			priority = "Tertiary ";
			break;
		case 8:
			priority = "Return   ";
			break;
		default:
			priority = "Tertiary ";
			break;
		}

		typeName = FindAIName(objective->m_type, g_objectiveTypeNames, 14);

		if (g_debugObjective == i) {
			marker = "=>";
		}
		else {
			marker = "  ";
		}

		sprintf(prefix, "%2.2s  %2.2d %s: %3.3s", marker, i, priority, typeName);

		switch (objective->m_state) {
		case 5:
			sprintf(line, "%s successful at %3.3d sec   %s", prefix, objective->m_endTime, objective->m_name);
			break;
		case 6:
			sprintf(line, "%s failed     at %3.3d sec   %s", prefix, objective->m_endTime, objective->m_name);
			break;
		default:
			if (objective->m_active) {
				activity = "in progress";
			}
			else {
				activity = "inactive   ";
			}

			if (objective->m_timeLimit > 0 && objective->m_active) {
				secondsLeft = objective->m_timeLimit - (g_missionTime - objective->m_startTime);
				sprintf(line, "%s %s   %3.3d sec   %s", prefix, activity, secondsLeft, objective->m_name);
			}
			else {
				sprintf(line, "%s %s             %s", prefix, activity, objective->m_name);
			}
			break;
		}

		if (g_monoEnabled) {
		}
	}
}

// Whether p_player can detect player p_target: a powered-down target (flag 0x10) within
// p_distance it has no line of sight to is hidden, checked with p_check at most every 0x21f ticks
// (m_nextDetectCheck). Nothing calls it.
// FUNCTION: MW2 0x10052cb7
MechS32 IsTargetDetectable(Player* p_player, MechS16 p_target, MechS16 p_distance, MechS32 p_check)
{
	MechS32 dead;
	MechS32 result;

	dead = g_players[p_target & 0xff]->m_flags & 0x10;
	result = TRUE;

	if (!p_check || g_currentClock <= p_player->m_nextDetectCheck) {
		return dead == 0;
	}

	if (p_player->m_targetInfo.m_distance < p_distance && (p_target & c_aiTargetPlayer) && dead &&
		!CanSeeTarget(p_player, 1)) {
		result = FALSE;
		g_hiddenTargetCount++;
	}

	p_player->m_nextDetectCheck = g_currentClock + 0x21f;

	return result;
}

// Finds the nearest of p_target's targets for p_player (*p_best) and the nearest that no
// teammate already attacks (*p_nearest), both within p_arg. Returns the teammate that does.
// Stack-slot permutation: blocker, closer, count, i, member, mission, nearest, objective and
// target and type.
// FUNCTION: MW2 0x10052d8b
Player* FindNearestTarget(Player* p_player, MechS16 p_target, MechS16 p_arg, MechS16* p_nearest, MechS16* p_best)
{
	MechS32 closer;
	MechS32 count;
	Player* members[8];
	MechS32 objective;
	MechS32 nearest;
	Player* blocker;
	MechS16 i;
	Player* member;
	MechU16 type;
	StarMission* mission;
	MechS16 target;

	blocker = NULL;
	nearest = 100000000;
	target = -1;
	*p_nearest = *p_best = 0;

	count = GetTeamMembers(p_player->m_team, members, TRUE);
	if (!count) {
		return NULL;
	}

	mission = &g_objectiveTable[p_player->m_team];
	objective = g_currentObjective[p_player->m_team];
	if (objective == -1) {
		return NULL;
	}

	type = mission->m_objectives[objective].m_type;
	while ((target = NextTarget(p_player, p_target, target)) != -1) {
		closer = FALSE;
		if (!SetTarget(p_player, target)) {
			continue;
		}

		if (p_player->m_targetInfo.m_distance < nearest) {
			nearest = p_player->m_targetInfo.m_distance;
			*p_best = target;
			closer = TRUE;
			if (!(target & c_aiTargetPlayer)) {
				*p_nearest = target;
			}
		}

		if (target & c_aiTargetPlayer) {
			for (i = 0; i < count; i++) {
				member = members[i];
				if (p_player->m_index == member->m_index) {
					continue;
				}

				if (member->m_flags & 0x16) {
					continue;
				}

				if (member->m_ai.m_goal & 0xf000) {
					continue;
				}

				if (member->m_ai.m_goal == target &&
					(member->m_ai.m_state == c_aiStateAttack || member->m_ai.m_state == c_aiStateTarget) && closer &&
					type != 4) {
					blocker = member;
					closer = FALSE;
					break;
				}
			}

			if (closer) {
				*p_nearest = target;
			}
		}
		else if (target & c_aiTargetThing) {
		}
		else if (target & c_aiTargetNav) {
		}
		else {
		}
	}

	if (*p_nearest && !AiMessageProx(p_player, *p_nearest, p_arg)) {
		*p_nearest = 0;
	}

	if (*p_best && !AiMessageProx(p_player, *p_best, p_arg)) {
		*p_best = 0;
	}

	return blocker;
}

// Logs the AI status page g_debugStar selects: the players (-1), their skills (-2) or a
// star's mission.
// FUNCTION: MW2 0x10053072
void LogAIStatus(void)
{
	if (g_debugListedStar != g_debugStar) {
		g_debugListedStar = g_debugStar;
		g_debugFirstLine = g_debugLastLine = -1;
		g_debugObjective = 0;
		ClearAILog();
	}

	if (g_debugStar == -1) {
		LogPlayerStatusLines();
	}
	else if (g_debugStar == -2) {
		LogPlayerSkillLines();
	}
	else {
		LogStarMissionLines(g_debugStar);
	}
}

// M_TGTABLE: the nearest of p_target's targets within p_arg hundreds (-2: p_player's alert range,
// -1: its resting one).
// FUNCTION: MW2 0x100530f7
MechS16 AiMessageTargetable(Player* p_player, MechS16 p_target, MechS16 p_arg)
{
	MechS16 nearest;
	MechS16 best;

	if (p_player->m_ai.m_flags & 3) {
		return 0;
	}

	if (p_arg == -2) {
		p_arg = p_player->m_alertRange / 100;
	}
	else if (p_arg == -1) {
		p_arg = p_player->m_restAlertRange / 100;
	}
	else if (p_arg == 0) {
		p_arg = GetTargetRange(p_target) / 100;
	}

	FindNearestTarget(p_player, p_target, p_arg, &nearest, &best);
	if (g_aiSpreadTargets) {
		return nearest;
	}
	else {
		return best;
	}
}

// FUNCTION: MW2 0x100531d1
MechS16 FUN_100531d1(void)
{
	return 0;
}

// FUNCTION: MW2 0x100531e4
MechS16 FUN_100531e4(void)
{
	return 0;
}

// M_DESTROY: p_target when it is destroyed, reporting it when the local player leads the star.
// FUNCTION: MW2 0x100531f7
MechS16 AiMessageDestroy(Player* p_player, MechS16 p_target, MechS16 p_arg)
{
	if (!IsTargetDone(p_target, 2)) {
		p_target = 0;
	}
	else if (GetTeamLeader(p_player->m_team) == g_localPlayerId) {
		SayLancemateReport(5, p_player->m_slot);
	}

	return p_target;
}

// The state functions (g_aiStateFns), run each tick with the player's target, which idling
// ignores. The original declared it without one and cast it into the table.
// FUNCTION: MW2 0x10053258
void AiStateIdle(Player* p_player, MechU16 p_target)
{
	p_player->m_targetInfo.m_distance = 0;
	return;
}

// Drives p_player towards p_target, or away from it when fleeing.
// FUNCTION: MW2 0x10053275
void AiStateMove(Player* p_player, MechU16 p_target)
{
	MechS32 heading;
	MechS32 range;

	SetTarget(p_player, p_target);
	BrakeFall(p_player);
	range = GetTargetRange(p_target);
	g_aiStateTime = g_currentClock;

	switch (p_player->m_ai.m_state) {
	case c_aiStateTarget:
		p_player->m_steering->m_weaponFire = 0;
		if (!AvoidObstacles(p_player)) {
			p_player->m_steering->m_throttle = GetApproachThrottle(p_player, range);
			heading = SteerToTarget(p_player);
		}
		else {
			heading = GetTargetBearing(p_player);
		}

		RunAIWeapons(p_player, heading);
		break;
	case c_aiStateRecon:
	case c_aiStatePatrol:
	case c_aiStateGoDirect:
		SweepTorso(p_player);
		if (!AvoidObstacles(p_player)) {
			p_player->m_steering->m_throttle = GetApproachThrottle(p_player, range);
			SteerToTarget(p_player);
		}

		g_aiStateTime += 0xb5;
		break;
	case c_aiStateFollow:
		SweepTorso(p_player);
		if (!AvoidObstacles(p_player)) {
			p_player->m_steering->m_throttle = GetApproachThrottle(p_player, range);
			SteerToTarget(p_player);
		}

		PlaceFormationNav(p_player);
		g_aiStateTime += 0xb5;
		break;
	case c_aiStateFlee:
		p_player->m_targetInfo.m_heading = (p_player->m_targetInfo.m_heading + 0xb40000) % 0x1680000;
		if (!AvoidObstacles(p_player)) {
			p_player->m_steering->m_throttle = 0x400;
			SteerToTarget(p_player);
		}

		g_aiStateTime += 0xb5;
		break;
	default:
		if (p_player->m_aiMode == 2) {
			p_player->m_steering->m_throttle = 0x333;
		}
		break;
	}
}

// Runs p_player's maneuvers against p_target (RunManeuver).
// FUNCTION: MW2 0x100534a0
void AiStateAttack(Player* p_player, MechU16 p_target)
{
	p_player->m_steering->m_weaponFire = 0;
	RunManeuver(p_player, p_target);
}

// The rule transitions (g_aiTransitionFns): whether the rule's new state is to be entered.
// FUNCTION: MW2 0x100534c5
MechS32 AiTransitionNull(Player* p_player, AiRule* p_rule)
{
	return TRUE;
}

// FUNCTION: MW2 0x100534da
MechS32 AiTransitionClearStack(Player* p_player, AiRule* p_rule)
{
	p_player->m_stackCount = 0;
	return TRUE;
}

// FUNCTION: MW2 0x100534fb
MechS32 AiTransitionPush(Player* p_player, AiRule* p_rule)
{
	AiStackEntry entry;

	entry.m_state = p_player->m_ai.m_state;
	entry.m_target = p_player->m_ai.m_goal;
	if (p_player->m_stackCount == 0) {
		p_player->m_stack[p_player->m_stackCount] = entry;
		p_player->m_stackCount++;
	}
	else {
		p_player->m_stack[0] = entry;
	}

	return TRUE;
}

// FUNCTION: MW2 0x10053577
MechS32 AiTransitionPop(Player* p_player, AiRule* p_rule)
{
	AiStackEntry entry;

	if (p_player->m_stackCount == 0) {
		return TRUE;
	}

	p_player->m_stackCount--;
	entry = p_player->m_stack[p_player->m_stackCount];
	SetAIState(p_player, entry.m_state, entry.m_target, 0);
	return FALSE;
}

// Stack-slot permutation: index and players.
// FUNCTION: MW2 0x100535e3
MechU16 FindAttacker(Player* p_player)
{
	Player** players;
	MechS16 i;
	MechS16 index;

	index = p_player->m_index;
	for (i = 0, players = g_players; i < g_playerCount; i++, players++) {
		if (*players && !((*players)->m_flags & 6) && (*players)->m_ai.m_goal == (index | c_aiTargetPlayer) &&
			!OnSameSide((*players)->m_index, index) && (*players)->m_ai.m_state == c_aiStateAttack) {
			return (*players)->m_index | c_aiTargetPlayer;
		}
	}

	return 0xffff;
}

// Resolves a rule's target field: '@' the goal, '%' the target found, '*' the current target.
// FUNCTION: MW2 0x100536b8
MechS16 ResolveRuleTarget(MechU16 p_value, MechS16 p_goal, MechS16 p_found, MechS16 p_target)
{
	MechS16 result;

	result = p_value;
	if (p_value == '@') {
		result = p_goal;
	}
	else if (p_value == '%') {
		result = p_found;
	}
	else if (p_value == '*') {
		result = p_target;
	}

	return result;
}

// Makes p_target p_player's target and works out the distance and bearing to it (UpdateTarget).
// Whether the target is valid.
// FUNCTION: MW2 0x1005372c
MechS32 SetTarget(Player* p_player, MechS16 p_target)
{
	p_player->m_targetInfo.m_target = p_target;
	return UpdateTarget(p_player) > 0;
}

// The range within which p_target counts as reached.
// FUNCTION: MW2 0x10053769
MechS32 GetTargetRange(MechS16 p_target)
{
	MechS32 range;

	switch (p_target & 0xf00) {
	case c_aiTargetNav:
		range = g_navTable[p_target & 0xff].m_radius;
		break;
	case c_aiTargetPlayer:
		range = 15000;
	case c_aiTargetThing:
		range = 10000;
		break;
	default:
		range = 100;
		break;
	}

	if (range < 100) {
		range = 100;
	}

	return range;
}

// The throttle for p_player to close to p_range of its target: full beyond twice the range,
// stopped inside it, slower while turning hard or running hot.
// FUNCTION: MW2 0x10053811
MechS32 GetApproachThrottle(Player* p_player, MechS32 p_range)
{
	MechS32 throttle;

	throttle = p_player->m_steering->m_throttle;
	if (p_range * 2 < p_player->m_targetInfo.m_distance) {
		throttle = 0x400;
	}
	else if (p_player->m_targetInfo.m_distance < p_range) {
		throttle = 0;
	}
	else {
		throttle = 0x333;
	}

	if (throttle && (abs(p_player->m_steering->m_turn) >> 16) > 45) {
		throttle = (0x4000000 - (MechS32) (abs(p_player->m_steering->m_turn) / 1.5)) >> 16;
		if (throttle < 0x66) {
			throttle = 0x66;
		}
	}

	if ((p_player->m_mech->m_heat >> 16) > 65.0) {
		throttle >>= 1;
	}

	if (throttle < 0x66) {
		throttle = 0;
	}

	return throttle;
}

// The torso pan towards p_angle, off by p_delta (AddClamped).
// FUNCTION: MW2 0x1005391f
MechS32 AimTorsoPan(Player* p_player, MechS32 p_angle, MechS32 p_delta)
{
	MechS32 value;

	value = p_angle / 0x2d00;
	return AddClamped(value, p_delta * 2, TRUE);
}

// The torso tilt towards the target, off by p_delta (AddClamped).
// FUNCTION: MW2 0x10053954
MechS32 AimTorsoTilt(Player* p_player, MechS32 p_delta)
{
	MechS32 value;

	value = p_player->m_targetInfo.m_pitch / 0xf00;
	return AddClamped(value, p_delta * 2, TRUE);
}

// Turns p_player towards its target. Returns the bearing to it.
// Stack-slot permutation: degrees and scaled.
// FUNCTION: MW2 0x1005398f
MechS32 SteerToTarget(Player* p_player)
{
	MechS32 delta;
	MechS32 scaled;
	MechS32 degrees;

	delta = GetTargetBearing(p_player);
	degrees = (delta >> 16) % 360;

	if (degrees > 45) {
		p_player->m_steering->m_turn = 0x3333333;
	}
	else if (degrees < -45) {
		p_player->m_steering->m_turn = -0x3333333;
	}
	else {
		scaled = degrees << 16;
		scaled /= 45;
		p_player->m_steering->m_turn = FixedMul16(0x3333333, scaled);
	}

	return delta;
}

// Leaves p_player's AI state: ends its maneuver or releases its navs, and reports the end of an
// order.
// FUNCTION: MW2 0x10053a2e
void LeaveAIState(Player* p_player)
{
	MechS16 flags;

	flags = 0;
	p_player->m_steering->m_weaponFire = 0;

	switch (p_player->m_ai.m_state) {
	case c_aiStatePatrol:
		if (IsTargetDone(p_player->m_ai.m_goal, 4) && (p_player->m_ai.m_flags & 0x10)) {
			SayLancemateReport(10, p_player->m_slot);
		}
	case c_aiStateFollow:
		ReleaseNavPoints(p_player);
		break;
	case c_aiStateAttack:
		EndManeuver(p_player);
		p_player->m_steering->m_turn = 0;
		p_player->m_steering->m_torsoTilt = 0;
		p_player->m_steering->m_torsoPan = 0;
	case c_aiStateRecon:
		if (IsTargetDone(p_player->m_ai.m_goal, 8) && (p_player->m_ai.m_flags & 0x10)) {
			SayLancemateReport(10, p_player->m_slot);
		}
		break;
	case c_aiStateTarget:
		if (!IsTargetDone(p_player->m_ai.m_goal, 2)) {
			flags = p_player->m_ai.m_flags;
		}
		else if (p_player->m_ai.m_flags & 0x10) {
			SayLancemateReport(10, p_player->m_slot);
		}
		break;
	case c_aiStateFlee:
		break;
	}

	p_player->m_ai.m_flags &= ~0x10;
	p_player->m_ai.m_flags |= flags;
}

// Enters AI state p_state for p_player, and logs it.
// Stack-slot permutation: the logging block's targetType, goalType, mech, logged, gt, goalName
// and line and navName.
// FUNCTION: MW2 0x10053be9
void EnterAIState(Player* p_player, MechU16 p_state)
{
	MechU32 nav;

	if (p_player->m_flags & 0x10) {
		p_player->m_flags &= ~0x10;
		p_player->m_mech->m_powerState = 0;
	}

	if (p_state != c_aiStateAttack && g_players[g_localPlayerId]->m_team == p_player->m_team) {
		p_player->m_ai.m_flags &= 3;
	}

	switch (p_state) {
	case c_aiStateGoDirect:
		break;
	case c_aiStateTarget:
		p_player->m_nextFireTime = RandomIntBelow(10) * 0x16 + g_currentClock;
		if (GetTeamLeader(p_player->m_team != g_localPlayerId) && p_player->m_ai.m_state == c_aiStatePatrol &&
			(p_player->m_nav & 0x1000) &&
			(nav = AddNavPoint(
				 p_player->m_index,
				 p_player->m_position.m_x,
				 p_player->m_position.m_y,
				 p_player->m_position.m_z
			 )) != -1) {
			g_navTable[nav].m_flags |= 1;
			g_navTable[nav].m_owner = p_player->m_index | c_aiTargetPlayer;
			p_player->m_nav = nav | c_aiTargetNav;
		}
		break;
	case c_aiStateAttack:
		break;
	case c_aiStateFlee:
		p_player->m_ai.m_flags = 1;
		break;
	case c_aiStateFollow:
		PlaceFormationNav(p_player);
		break;
	case c_aiStatePatrol:
		PlacePatrolNavs(p_player);
		break;
	case c_aiStateShutdown:
		p_player->m_ai.m_flags = 1;
	case c_aiStateRest:
	case -1:
		p_player->m_flags |= 0x10;
		p_player->m_mech->m_powerState |= 3;
	case c_aiStateIdle:
		p_player->m_steering->m_throttle = 0;
		p_player->m_targetInfo.m_distance = 0;
		break;
	case c_aiStateAvoid:
	case c_aiStateRecon:
	case 9:
		break;
	}

	g_aiStateTime = g_currentClock;
	p_player->m_ai.m_state = p_state;

	{
		const MechChar* targetType;
		const MechChar* goalType;
		MechChar mech[] = "mech";
		Mech* logged;
		MechChar gt[] = "gt";
		MechChar goalName[20];
		MechChar line[80];
		MechChar navName[] = "nav";

		logged = p_player->m_mech;

		if (logged->m_player->m_ai.m_target & c_aiTargetPlayer) {
			targetType = mech;
		}
		else {
			targetType = gt;
		}

		if (logged->m_player->m_ai.m_target & c_aiTargetNav) {
			targetType = navName;
		}
		else {
		}

		if (logged->m_player->m_ai.m_goal & c_aiTargetPlayer) {
			goalType = mech;
		}
		else {
			goalType = gt;
		}

		if (logged->m_player->m_ai.m_goal & c_aiTargetNav) {
			goalType = navName;
		}
		else {
		}

		if (logged->m_player->m_ai.m_goal & 0x6000) {
			strcpy(goalName, FindAIName(logged->m_player->m_ai.m_goal, g_aiSymbolicTargetNames, 7));
			goalType += strlen(goalType);
		}
		else {
			sprintf(goalName, "%d", logged->m_player->m_ai.m_goal & 0xff);
		}

		sprintf(
			line,
			"%6d : %2d Mech %2d : state %8s %4s %2d, objective %4s %-12s\n",
			g_currentClock,
			logged->m_player->m_team,
			logged->m_player->m_index,
			g_aiStateNames[logged->m_player->m_ai.m_state].m_name,
			targetType,
			logged->m_player->m_ai.m_target & 0xff,
			goalType,
			goalName
		);
		WriteToMw2Log(line);
	}
}

// The target after p_previous (-1 for the first) of p_target for p_player: a plain target
// once, or each player a symbolic target names. Returns -1 after the last.
// Stack-slot permutation: home and index.
// FUNCTION: MW2 0x10054043
MechS16 NextTarget(Player* p_player, MechS16 p_target, MechS16 p_previous)
{
	MechS16 result;
	MechS32 index;
	MechS16 home;

	result = -1;
	index = p_player->m_index;

	if (!(p_target & 0x6000)) {
		if ((p_target & 0xfff) != (p_previous & 0xfff)) {
			return p_target;
		}
		else {
			return -1;
		}
	}

	if (p_previous != -1) {
		if (p_target & 0x2000) {
			return -1;
		}
		else {
			p_previous &= 0xff;
		}
	}

	if (p_target & c_aiTargetPlayer) {
		if (p_target & 0x4000) {
			switch (p_target) {
			case c_aiTargetEnemy:
				while (++p_previous < g_playerCount) {
					if (!OnSameSide(p_previous, index) && GetPlayerSide(p_previous) != 2) {
						result = p_previous;
						break;
					}
				}
				break;
			case c_aiTargetFriendly:
				while (++p_previous < g_playerCount) {
					if (GetTeamLeader(g_players[p_previous]->m_team) == g_localPlayerId ||
						(g_lairdoCheat && !GetPlayerSide(p_previous))) {
						result = p_previous;
						break;
					}
				}
				break;
			default:
				break;
			}
		}
		else {
			switch (p_target) {
			case c_aiTargetUser:
				result = g_localPlayerId;
				break;
			case c_aiTargetMyLeader:
				result = GetTeamLeader(p_player->m_team);
				break;
			case c_aiTargetMe:
				result = p_player->m_index;
				break;
			default:
				break;
			}
		}
	}
	else if (p_target & c_aiTargetThing) {
	}
	else if (p_target & c_aiTargetNav) {
		switch (p_target) {
		case c_aiTargetHome:
			home = GetTeamHomeTarget(p_player->m_team);
			result = home;
			break;
		case c_aiTargetRbAnchor:
			result = p_player->m_nav;
			break;
		default:
			break;
		}
	}

	if (result != -1 && !(result & 0xf00)) {
		result = (p_target & 0xf00) | result;
	}

	return result;
}

// The bearing to p_player's target, 16.16 degrees either way.
// FUNCTION: MW2 0x1005432f
MechS32 GetTargetBearing(Player* p_player)
{
	MechS32 delta;

	delta = p_player->m_targetInfo.m_heading - p_player->m_heading;
	if (delta > 0xb40000) {
		delta -= 0x1680000;
	}
	else if (delta < -0xb40000) {
		delta += 0x1680000;
	}

	return delta;
}

// Whether p_target is done for an objective of type p_check: its flag 4 (destroyed) or 0x20
// (inspected) is set.
// Stack-slot permutation: flags, index, mask and result and type.
// FUNCTION: MW2 0x10054384
MechS32 IsTargetDone(MechU16 p_target, MechS32 p_check)
{
	MechU32 index;
	MechS16 mask;
	MechS32 result;
	MechU32 type;
	MechS16* flags;

	flags = NULL;
	result = TRUE;
	type = p_target & 0xff00;
	index = p_target & 0xff;

	if (type & 0xf000) {
		return TRUE;
	}

	switch (type) {
	case c_aiTargetPlayer:
		flags = &g_players[index]->m_flags;
		break;
	case c_aiTargetThing:
		flags = &g_gameThings[index].m_flags;
		break;
	case c_aiTargetNav:
		flags = &g_navTable[index].m_flags;
		break;
	default:
		break;
	}

	if (flags) {
		switch (p_check) {
		case 1:
		case 2:
		case 4:
		case 0x1000:
			if (type == c_aiTargetPlayer) {
				mask = 4;
			}
			else if (type == c_aiTargetThing) {
				mask = 4;
			}
			else if (type == c_aiTargetNav) {
				flags = NULL;
				result = FALSE;
			}
			break;
		case 8:
		case 0x100:
			if (type == c_aiTargetPlayer) {
				mask = 0x20;
			}
			else if (type == c_aiTargetThing) {
				mask = 0x20;
			}
			else if (type == c_aiTargetNav) {
				mask = 0x20;
			}
			break;
		default:
			break;
		}

		if (flags && !(mask & *flags)) {
			result = FALSE;
		}
	}

	return result;
}

// Enters p_state for p_player now, or after its pushed state when it has one.
// FUNCTION: MW2 0x10054584
void QueueAIState(Player* p_player, MechS16 p_state, MechU16 p_target)
{
	AiStackEntry entry;

	if (p_player->m_stackCount == 0) {
		SetAIState(p_player, p_state, p_target, 0);
	}
	else {
		entry.m_state = p_state;
		entry.m_target = p_target;
		p_player->m_stack[p_player->m_stackCount] = entry;
	}
}

// Whether one of p_player's scripts has rules for p_state.
// Stack-slot permutation: i and script.
// FUNCTION: MW2 0x100545ea
MechS32 HasAIState(Player* p_player, MechS16 p_state)
{
	MechS32 i;
	MechU16* script;
	MechS32 j;

	for (i = 0; i <= 2; i++) {
		script = p_player->m_ruleSets[i];
		if (!script) {
			continue;
		}

		for (j = 0; j < *script; j++) {
			if (script[j * 2 + 1] == p_state) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

// Makes p_player enter p_state with p_target, pushing the current state with p_push, if its
// scripts have the state.
// FUNCTION: MW2 0x10054684
void SetAIState(Player* p_player, MechS16 p_state, MechS16 p_target, MechS32 p_push)
{
	if (p_state != -1 && HasAIState(p_player, p_state)) {
		if (p_push) {
			AiTransitionPush(p_player, NULL);
		}
		else {
			AiTransitionClearStack(p_player, NULL);
		}

		LeaveAIState(p_player);

		if (p_state == c_aiStateFollow && p_target == c_aiTargetMyLeader &&
			GetTeamLeader(p_player->m_team) == p_player->m_index) {
			p_state = c_aiStateIdle;
			p_target = 0;
		}

		p_player->m_ai.m_target = ResolveTarget(p_player, p_target);
		p_player->m_ai.m_goal = p_player->m_ai.m_target;
		EnterAIState(p_player, p_state);
		CollectAIRules(p_player);
	}
}

// Releases the nav points p_player placed, except its anchor.
// FUNCTION: MW2 0x10054778
void ReleaseNavPoints(Player* p_player)
{
	MechU32 i;

	if (g_navCount != -1) {
		for (i = 0; (MechS32) i < g_navCount; i++) {
			if ((g_navTable[i].m_flags & 1) && g_navTable[i].m_owner == (p_player->m_index | c_aiTargetPlayer)) {
				if (!(p_player->m_nav & 0x1000)) {
					if ((p_player->m_nav & 0xff) != i) {
						RemoveNavPoint(p_player->m_index, i | c_aiTargetNav);
					}
				}
				else {
					RemoveNavPoint(p_player->m_index, i | c_aiTargetNav);
				}
			}
		}
	}
}

// Places four nav points around p_player's target and targets them.
// Stack-slot permutation: found, index, nav, owner, player and range and y.
// FUNCTION: MW2 0x10054851
void PlacePatrolNavs(Player* p_player)
{
	MechU32 index;
	MechS32 range;
	MechS32 x;
	MechU32 owner;
	MechS32 y;
	MechS32 z;
	MechS32 found;
	Player* player;
	NavPoint* nav;

	found = TRUE;
	range = GetTargetRange(p_player->m_ai.m_target);
	owner = p_player->m_index;
	index = p_player->m_ai.m_target & 0xff;

	switch (p_player->m_ai.m_target & 0xf00) {
	case c_aiTargetThing:
		GetStaticObjectPosition(g_gameThings[index].m_staticObject, &x, &y, &z);
		range = 25000;
		break;
	case c_aiTargetPlayer:
		player = g_players[index];
		x = player->m_position.m_x;
		y = player->m_position.m_y;
		z = player->m_position.m_z;
		break;
	case c_aiTargetNav:
		nav = &g_navTable[index];
		x = nav->m_position[0];
		y = nav->m_position[1];
		z = nav->m_position[2];
		break;
	default:
		found = FALSE;
		break;
	}

	if (!found) {
		return;
	}

	if (AddNavPoint(owner, x - range, y, z) == -1) {
	}

	if (AddNavPoint(owner, x, y, z - range) == -1) {
	}

	if (AddNavPoint(owner, x + range, y, z) == -1) {
	}

	if (AddNavPoint(owner, x, y, z + range) == -1) {
	}

	AdvanceNavTarget(p_player, c_aiTargetNav);
}

// Targets p_target and steps p_player's target on to the next of its own nav points, past its
// anchor nav, and makes that its AI target.
// FUNCTION: MW2 0x10054a30
void AdvanceNavTarget(Player* p_player, MechS16 p_target)
{
	p_player->m_targetInfo.m_target = p_target;
	CycleNavTarget(p_player, 1, 1);
	if (p_player->m_nav == p_player->m_targetInfo.m_target) {
		CycleNavTarget(p_player, 1, 1);
	}

	p_player->m_ai.m_target = p_player->m_targetInfo.m_target;
}

// Places a nav point at p_player's place in its star's formation and targets it.
// Stack-slot permutation: heading, nav and x and z.
// FUNCTION: MW2 0x10054a93
void PlaceFormationNav(Player* p_player)
{
	MechS32 z;
	MechS32 heading;
	MechS32 x;
	MechS32 nav;

	ReleaseNavPoints(p_player);
	if (GetTeamSlotPosition(p_player->m_index, &x, &z, &heading)) {
		nav = AddNavPoint(p_player->m_index, x, heading, z);
		if (nav != -1) {
			g_navTable[nav].m_flags |= 1;
			g_navTable[nav].m_owner = p_player->m_index | c_aiTargetPlayer;
			p_player->m_ai.m_target = nav | c_aiTargetNav;
		}
	}
}

// Makes player p_index attack player p_target (logging the first time), when it is the local
// player or on a network game.
// Stack-slot permutation: line and player.
// FUNCTION: MW2 0x10054b50
void RecordAttack(MechS32 p_index, MechU32 p_target)
{
	MechChar line[80];
	Player* player;

	if (p_index == g_localPlayerId || g_netRole) {
		player = g_players[p_index];
		if (player->m_ai.m_target != (p_target | c_aiTargetPlayer) || player->m_ai.m_state != c_aiStateAttack) {
			sprintf(
				line,
				"%6d : %2d Mech %2d has attacked mech %2d\n",
				g_currentClock,
				player->m_team,
				p_index,
				p_target
			);
			WriteToMw2Log(line);
		}

		player->m_ai.m_goal = p_target | c_aiTargetPlayer;
		player->m_ai.m_state = c_aiStateAttack;
	}
}

// Loads the nine AIT scripts.
// FUNCTION: MW2 0x10054c05
MechS32 LoadAIScripts(void)
{
	MechS32 i;

	g_aiScripts[0] = NULL;
	for (i = 0; i < 9; i++) {
		g_aiScripts[i + 1] = LoadCachedResource(g_mw2PrjHandle, i + 1, g_resourceTypeTags[c_resTagAit], 0);
	}

	return TRUE;
}

// FUNCTION: MW2 0x10054c6a
MechS32 AiTransitionNotify(Player* p_player, AiRule* p_rule)
{
	MechS16 leader;

	leader = GetTeamLeader(p_player->m_team);
	if (leader != -1) {
		PostAIMessage(g_players[leader], p_rule->m_message, p_player->m_targetInfo.m_target, p_rule->m_arg);
	}

	return FALSE;
}

// The local player's teammate in formation slot p_slot, or -1.
// Operand order: the loop test (i < g_playerCount) compares with i in eax in the original. The
// team comparison loads the other operand first. Stack-slot permutation: player and team.
// FUNCTION: MW2 0x10054ccc
MechS32 FindStarSlotPlayer(MechS32 p_slot)
{
	MechS32 team;
	Player* player;
	MechS32 i;

	team = g_players[g_localPlayerId]->m_team;
	for (i = 0; i < g_playerCount; i++) {
		player = g_players[i];
		if (player->m_team == team && player->m_slot == p_slot) {
			return i;
		}
	}

	return -1;
}

// Posts a message to p_player unless one is waiting.
// FUNCTION: MW2 0x10054d4c
void PostAIMessage(Player* p_player, MechS16 p_message, MechU16 p_target, MechS16 p_arg)
{
	AiMessage* message;

	message = &p_player->m_ai.m_posted;
	if (message->m_message == 0) {
		message->m_message = p_message;
		message->m_target = p_target;
	}
}

// Orders the players of p_targets into p_state with p_target (the local player's orders to its
// star). Returns how many it skipped.
// Stack-slot permutation: player and skipped and target.
// FUNCTION: MW2 0x10054d88
MechS16 OrderPlayers(Player* p_player, MechS16 p_targets, MechS16 p_state, MechS16 p_target)
{
	MechS16 skipped;
	Player* player;
	MechS16 target;

	target = -1;
	skipped = 0;

	while ((target = NextTarget(p_player, p_targets, target)) != -1) {
		if (target & c_aiTargetPlayer) {
			if (p_target == target) {
				skipped++;
			}
			else {
				player = g_players[target & 0xff];
				if (player->m_index != g_localPlayerId) {
					if (player->m_ai.m_state != c_aiStateDead && HasAIState(player, p_state)) {
						if (p_state == c_aiStateAttack) {
							if (!player->m_engageAtWill) {
								player->m_engageAtWill = 1;
							}
							else {
								player->m_engageAtWill = 0;
							}

							player->m_ai.m_flags &= ~3;
						}
						else {
							player->m_ai.m_flags = 0;
							if (p_state == c_aiStatePatrol) {
								SetAIState(player, p_state, p_target, 0);
							}
							else {
								SetAIState(player, p_state, p_target, 1);
							}

							if (p_state == c_aiStateTarget) {
								player->m_ai.m_flags |= 0x11;
							}
							else {
								if (p_state == c_aiStateFollow) {
									player->m_engageAtWill = 0;
								}

								player->m_ai.m_flags |= 0x12;
							}
						}
					}
					else {
						skipped++;
					}
				}
			}
		}
	}

	return skipped;
}

// Carries out the local player's command p_command to formation slot p_slot (or the star), and
// says it.
// Operand order: index == g_localPlayerId compares the other way in the original. Stack-slot
// permutation: local and speech.
// FUNCTION: MW2 0x10054f50
MechS32 OrderStarSlot(MechS32 p_slot, MechS16 p_command)
{
	MechS16 target;
	MechS32 index;
	MechS16 speech;
	Player* local;

	speech = -1;
	local = g_players[g_localPlayerId];
	index = FindStarSlotPlayer(p_slot);

	if (index == -1) {
		return FALSE;
	}

	target = 0x1000;
	switch (p_command) {
	case c_aiStateTarget:
		if (local->m_targetInfo.m_target) {
			target = local->m_targetInfo.m_target;
		}

		if (target & c_aiTargetNav) {
			target = -1;
			speech = 0;
		}

		speech = 1;
		break;
	case c_aiStateFollow:
		target = c_aiTargetMyLeader;
		speech = 2;
		break;
	case c_aiStatePatrol:
		if (local->m_targetInfo.m_target) {
			target = local->m_targetInfo.m_target;
		}

		speech = 3;
		break;
	case c_aiStateGoDirect:
		target = c_aiTargetHome;
		speech = 4;
		break;
	case c_aiStateShutdown:
		target = 0;
		speech = 9;
		break;
	case c_aiStateAttack:
		target = 0;
		break;
	case c_aiStateFlee:
	case c_aiStateRecon:
	case 9:
	case c_aiStateRest:
		break;
	}

	if (target & 0x1000) {
		return FALSE;
	}

	if (index == g_localPlayerId) {
		if (OrderPlayers(local, c_aiTargetFriendly, p_command, target)) {
		}
	}
	else {
		if (OrderPlayers(local, p_slot | c_aiTargetPlayer, p_command, target)) {
		}
	}

	if (speech != -1) {
		SayLancemateReport(speech, p_slot);
	}

	return TRUE;
}

// Adds p_delta to p_value (with p_value's sign with p_sameSign), clamped to +/-0x400.
// Operand order: the two sign tests evaluate p_value and p_delta in the other order.
// FUNCTION: MW2 0x10055131
MechS32 AddClamped(MechS32 p_value, MechS32 p_delta, MechS32 p_sameSign)
{
	if (p_sameSign && (p_value < 0 ? -1 : 1) != (p_delta < 0 ? -1 : 1)) {
		p_delta = -p_delta;
	}

	p_value += p_delta;
	if (p_value > 0x400) {
		p_value = 0x400;
	}
	else if (p_value < -0x400) {
		p_value = -0x400;
	}

	return p_value;
}

// Gives star p_team its orders for the current objective.
// FUNCTION: MW2 0x100551ad
void ResetStarOrders(MechS32 p_team)
{
	HandleStarOrder(p_team, NULL);
}

// Gives star p_team's AI players the script and state of its current objective: rest, shut down
// or follow the leader.
// Operand order: the (1 << bit) & type test evaluates type first in the original. Stack-slot
// permutation: bit, count, i, leader, member, members, objective and state and type.
// FUNCTION: MW2 0x100551c6
void AssignStarObjective(MechS32 p_team)
{
	MechS16 target;
	MechS16 leader;
	MechS32 count;
	Player* members[8];
	MechS32 objective;
	MechS32 bit;
	MechS32 i;
	MechS32 state;
	MechU16 type;
	StarMission* mission;
	Player* member;
	MechS16 flags;

	target = 0;
	leader = GetTeamLeader(p_team);
	if (leader == -1) {
		leader = ChooseTeamLeader(p_team);
		if (leader == -1) {
			return;
		}
	}

	count = GetTeamMembers(p_team, members, TRUE);
	if (!count) {
		return;
	}

	mission = &g_objectiveTable[p_team];
	objective = g_currentObjective[p_team];
	if (objective != -1) {
		type = mission->m_objectives[objective].m_type;
	}
	else {
		type = 0x200;
	}

	if (type == 0x400) {
		state = c_aiStateRest;
	}
	else if (type == 0x800 || type == 0x10) {
		state = c_aiStateShutdown;
	}
	else {
		target = leader | c_aiTargetPlayer;
		state = c_aiStateFollow;
	}

	flags = 0x40;
	flags = GetEngagementAIFlags(mission->m_objectives[objective].m_engagement) | flags;

	for (bit = 0; bit < 16 && !((1 << bit) & type); bit++) {
	}

	for (i = 0; i < count; i++) {
		member = members[i];
		if (member->m_aiMode != 2) {
			continue;
		}

		SetAIScript(member, 2, bit + 1, leader);
		if (leader == g_localPlayerId && g_localStarAssigned) {
			continue;
		}

		if (member->m_index == leader && state == c_aiStateFollow) {
			QueueAIState(member, c_aiStateIdle, 0);
		}
		else {
			QueueAIState(member, state, 2);
		}

		member->m_ai.m_flags &= ~3;
		member->m_ai.m_flags |= flags;
		ReleaseAnchorNav(member);
	}

	if (GetTeamLeader(p_team) == g_localPlayerId && type != 0x10) {
		g_localStarAssigned = 1;
	}
}

// FUNCTION: MW2 0x10055485
MechS16 GetEngagementAIFlags(MechS32 p_value)
{
	if (p_value == 1) {
		return 2;
	}
	else if (p_value == 2) {
		return 1;
	}
	else {
		return 0;
	}
}

// Sets p_player's script slot p_slot to script p_script of its player type, as its star's
// leader or a follower; -1 clears the slot.
// Index order: each table lookup loads follower before p_script in the original (every case
// differs the same way).
// FUNCTION: MW2 0x100554c8
void SetAIScript(Player* p_player, MechS16 p_slot, MechS16 p_script, MechS16 p_leader)
{
	MechU16 follower;

	if (p_script == -1) {
		p_player->m_ruleSets[p_slot] = NULL;
		return;
	}

	if (p_player->m_index == p_leader) {
		follower = 0;
	}
	else {
		follower = 1;
	}

	switch (p_player->m_type) {
	case c_playerTypeMech:
		p_player->m_ruleSets[p_slot] = (MechU16*) g_aiScripts[g_mechScripts[p_script][(MechS16) follower]];
		break;
	case c_playerTypeArtillery:
		p_player->m_ruleSets[p_slot] = (MechU16*) g_aiScripts[g_artilleryScripts[p_script][(MechS16) follower]];
		break;
	case c_playerTypeWanderer:
		p_player->m_ruleSets[p_slot] = (MechU16*) g_aiScripts[g_wandererScripts[p_script][(MechS16) follower]];
		break;
	case c_playerTypeTruck:
		p_player->m_ruleSets[p_slot] = (MechU16*) g_aiScripts[g_truckScripts[p_script][(MechS16) follower]];
		break;
	case c_playerTypeTank:
		p_player->m_ruleSets[p_slot] = (MechU16*) g_aiScripts[g_tankScripts[p_script][(MechS16) follower]];
		break;
	case c_playerTypeHelicopter:
		p_player->m_ruleSets[p_slot] = (MechU16*) g_aiScripts[g_helicopterScripts[p_script][(MechS16) follower]];
		break;
	case 8:
		p_player->m_ruleSets[p_slot] = (MechU16*) g_aiScripts[g_helicopterScripts[p_script][(MechS16) follower]];
		break;
	case c_playerTypeDoor:
		break;
	}
}

// Target p_index of star p_team's current objective.
// FUNCTION: MW2 0x100556a4
MechU16 GetObjectiveTarget(MechS32 p_team, MechS16 p_objective, MechS32 p_index)
{
	StarMission* mission;

	mission = &g_objectiveTable[p_team];
	p_objective = g_currentObjective[p_team];
	return mission->m_objectives[p_objective].m_targets[p_index];
}

// Fills p_members with star p_team's members that are in play (alive, with p_aliveOnly).
// Returns how many.
// Stack-slot permutation: count and i.
// FUNCTION: MW2 0x100556fe
MechS32 GetTeamMembers(MechS32 p_team, Player** p_members, MechS32 p_aliveOnly)
{
	MechS32 mask;
	MechS32 count;
	MechS32 i;
	Player* player;
	Team* team;

	mask = 6;
	if (!p_aliveOnly) {
		mask |= 0x10;
	}

	team = &g_teams[p_team];
	for (i = 0, count = 0; i < team->m_memberCount; i++) {
		player = g_players[team->m_members[i]];
		if (!(mask & player->m_flags)) {
			p_members[count] = player;
			count++;
		}
	}

	return count;
}

// Handles an order posted to star p_team's leader (M_TGTABLE: attack its target), or with none
// gives the star its objective's orders.
// FUNCTION: MW2 0x1005579a
void HandleStarOrder(MechS32 p_team, AiMessage* p_order)
{
	if (p_order && p_order->m_message) {
		switch (p_order->m_message) {
		case 7:
			AssignStarTarget(p_team, p_order->m_target);
			break;
		default:
			break;
		}
	}
	else {
		AssignStarObjective(p_team);
	}
}

// Sends members of star p_team after p_target: every free one on objective 4, else the free one
// nearest the target's value (or the leader). Whether one went.
// Stack-slot permutation: chosen, count, diff, i, leader, member, members, mission, nearest,
// objective, required, result and slot and team.
// FUNCTION: MW2 0x10055811
MechS32 AssignStarTarget(MechS32 p_team, MechU16 p_target)
{
	MechS32 required;
	StarMission* mission;
	Team* team;
	MissionObjective* objective;
	MechS32 result;
	MechS32 nearest;
	MechS32 leader;
	MechS32 count;
	MechS32 diff;
	Player* members[8];
	MechS32 i;
	MechU32 slot;
	Player* member;
	MechS32 chosen;

	slot = 0;
	if (p_target & 0xf000) {
		return FALSE;
	}

	leader = GetTeamLeader(p_team);
	if (leader == -1) {
		return FALSE;
	}

	count = GetTeamMembers(p_team, members, TRUE);
	if (!count) {
		return FALSE;
	}

	switch (p_target & 0xf00) {
	case c_aiTargetPlayer:
		slot = g_players[p_target & 0xff]->m_ai.m_value;
		break;
	default:
		break;
	}

	if (g_currentObjective[p_team] == 4) {
		result = FALSE;
		team = &g_teams[p_team];
		mission = &g_objectiveTable[p_team];
		objective = &mission->m_objectives[g_currentObjective[p_team]];
		required = objective->m_requiredCount;
		if (!required) {
			required = count;
		}

		for (i = 0; i < count && required; i++) {
			member = members[i];
			if (member->m_ai.m_state != c_aiStateTarget && member->m_ai.m_state != c_aiStateAttack &&
				!IsOutOfAmmo(member->m_mech) && member->m_engageAtWill) {
				SetAIState(member, c_aiStateTarget, p_target, 1);
				result = TRUE;
			}
		}

		return result;
	}
	else {
		for (i = 0, chosen = -1, nearest = 0x7fff; i < count; i++) {
			member = members[i];
			if (member->m_index != leader && !(member->m_ai.m_flags & 3) && member->m_ai.m_state != c_aiStateTarget &&
				member->m_ai.m_state != c_aiStateAttack && !IsOutOfAmmo(member->m_mech) && member->m_engageAtWill) {
				diff = member->m_ai.m_value - slot;
				if (abs(diff) < nearest) {
					nearest = abs(diff);
					chosen = member->m_index;
				}
			}
		}

		if (chosen != -1) {
			member = g_players[chosen];
		}
		else {
			member = g_players[leader];
			if ((member->m_ai.m_flags & 3) || member->m_ai.m_state == c_aiStateTarget ||
				member->m_ai.m_state == c_aiStateAttack || IsOutOfAmmo(member->m_mech) || !member->m_engageAtWill) {
				member = NULL;
			}
		}

		if (member) {
			SetAIState(member, c_aiStateTarget, p_target, 1);
			member->m_ai.m_flags |= 0x20;
			member->m_ai.m_flags &= ~0x40;
			return TRUE;
		}

		return FALSE;
	}
}

// Run by star p_team's leader: hands its posted order on and sends an idle member after the
// first target of the star's objective that isn't covered yet.
// Operand order: g_localPlayerId == leader and the loop tests compare the other way in the
// original. Stack-slot permutation: count, i, j, leader, members, mission, objective,
// objectiveIndex, player, remaining, state, target, targets and team and type.
// FUNCTION: MW2 0x10055bb7
void LeadStar(MechS32 p_team)
{
	MechU16* targets;
	MechS32 leader;
	MechS32 count;
	Player* members[8];
	MechS32 objectiveIndex;
	MechS32 remaining;
	MechS32 j;
	MechS32 i;
	MechU16 target;
	MechS32 type;
	MechS16 state;
	StarMission* mission;
	Player* player;
	Team* team;
	MissionObjective* objective;
	MechS16 flags;

	leader = GetTeamLeader(p_team);
	if (leader == -1) {
		return;
	}

	player = g_players[leader];
	if (player->m_ai.m_posted.m_message) {
		HandleStarOrder(p_team, &player->m_ai.m_posted);
		player->m_ai.m_posted.m_message = 0;
	}

	if (g_localPlayerId == leader) {
		return;
	}

	team = &g_teams[p_team];
	mission = &g_objectiveTable[p_team];
	objectiveIndex = g_currentObjective[p_team];
	if (objectiveIndex == -1) {
		return;
	}

	objective = &mission->m_objectives[objectiveIndex];
	type = objective->m_type;
	if (type == 0 || (type & 0xe10)) {
		return;
	}

	count = GetTeamMembers(p_team, members, FALSE);
	if (!count) {
		return;
	}

	target = 0;
	for (i = 0, targets = objective->m_targets; i < objective->m_targetCount && !target; i++, targets++) {
		remaining = objective->m_requiredCount;
		if (!remaining) {
			remaining = count;
		}

		target = *targets;
		if (IsTargetDone(target, type)) {
			target = 0;
			continue;
		}

		for (j = 0; j < count && remaining; j++) {
			player = members[j];
			if (player->m_ai.m_goal != target) {
				continue;
			}

			switch (type) {
			case 0x1000:
				remaining--;
				break;
			case 1:
			case 2:
				if (player->m_ai.m_state == c_aiStateTarget || player->m_ai.m_state == c_aiStateAttack) {
					remaining--;
				}
				break;
			case 4:
				if (player->m_ai.m_state == c_aiStatePatrol) {
					remaining--;
				}
				break;
			case 8:
				if (player->m_ai.m_state == c_aiStateTarget) {
					remaining--;
				}
				break;
			case 0x20:
			case 0x100:
			case 0x200:
			case 0x400:
				if (player->m_ai.m_state == c_aiStateGoDirect) {
					remaining--;
				}
				break;
			case 0x2000:
				if (player->m_ai.m_state == c_aiStateFlee) {
					remaining--;
				}
				break;
			default:
				break;
			}
		}

		if (!remaining) {
			target = 0;
		}
	}

	if (!target) {
		return;
	}

	for (i = 0, player = NULL; i < count; i++) {
		if ((members[i]->m_ai.m_state == c_aiStateIdle ||
			 (members[i]->m_ai.m_state == c_aiStateFollow && !IsOutOfAmmo(members[i]->m_mech))) &&
			g_players[leader] != members[i]) {
			player = members[i];
			break;
		}
	}

	if (!player) {
		if (!g_players[leader]->m_ai.m_state && !IsOutOfAmmo(g_players[leader]->m_mech)) {
			player = g_players[leader];
		}
		else {
			return;
		}
	}

	for (i = 0; i < 16; i++) {
		if (type & (1 << i)) {
			SetAIScript(player, 2, i + 1, GetTeamLeader(p_team));
			break;
		}
	}

	flags = 0x40;
	flags = GetEngagementAIFlags(mission->m_objectives[objectiveIndex].m_engagement) | flags;

	state = -1;
	switch (type) {
	case 0x1000:
		state = c_aiStateIdle;
		StartNuke(player);
		break;
	case 1:
	case 2:
		state = c_aiStateTarget;
		break;
	case 4:
		state = c_aiStatePatrol;
		break;
	case 8:
		state = c_aiStateTarget;
		break;
	case 0x20:
	case 0x100:
	case 0x200:
	case 0x400:
		state = c_aiStateGoDirect;
		break;
	case 0x2000:
		state = c_aiStateFlee;
		break;
	default:
		break;
	}

	if (state != -1) {
		QueueAIState(player, state, target);
		player->m_ai.m_flags |= flags;
	}
}

// Releases p_player's anchor nav (m_nav). As written it only does so when m_nav is 0, an index
// without the nav type bits, so the anchor stays placed.
// FUNCTION: MW2 0x100561ea
void ReleaseAnchorNav(Player* p_player)
{
	if (p_player->m_nav != 0) {
		return;
	}

	RemoveNavPoint(p_player->m_index, p_player->m_nav);
	p_player->m_nav = 0x1000;
}

// The number of players in the local player's star.
// FUNCTION: MW2 0x10056230
MechS32 GetLocalStarSize(void)
{
	return g_teams[g_players[g_localPlayerId]->m_team].m_memberCount;
}

// Carries out the local player's star command: a formation (-1) or an order to slot p_slot.
// FUNCTION: MW2 0x1005625d
void RunStarCommand(MechS32 p_command, MechS32 p_slot)
{
	if (p_command == -1) {
		SetTeamFormationByName(g_localStar, g_formationTemplates[p_slot].m_name);
		SayFormation(p_slot);
	}
	else {
		OrderStarSlot(p_slot, p_command);
	}
}

// Sweeps a mech's torso pan from side to side, 5 degrees every 0x20 ticks (0x40 while stopped),
// turning back past 45.
// Operand order: period + g_currentClock adds in the other order (commutative).
// FUNCTION: MW2 0x100562b4
void SweepTorso(Player* p_player)
{
	MechS32 period;

	if (p_player->m_steering->m_throttle == 0) {
		period = 0x40;
	}
	else {
		period = 0x20;
	}

	if (p_player->m_type != c_playerTypeMech) {
		return;
	}

	if (p_player->m_maneuverEnd > period + g_currentClock) {
		p_player->m_maneuverEnd = period + g_currentClock;
	}

	if (p_player->m_maneuverEnd > g_currentClock) {
		return;
	}

	if (p_player->m_maneuverParam) {
		p_player->m_steering->m_torsoPan += 0x50000;
	}
	else {
		p_player->m_steering->m_torsoPan -= 0x50000;
	}

	if (abs(p_player->m_steering->m_torsoPan) > 0x2d0000) {
		if (!p_player->m_maneuverParam) {
			p_player->m_maneuverParam = 1;
		}
		else {
			p_player->m_maneuverParam = 0;
		}
	}

	p_player->m_maneuverEnd = period + g_currentClock;
}
