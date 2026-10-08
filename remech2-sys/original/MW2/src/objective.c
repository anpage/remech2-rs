#include "objective.h"

#include "ai.h"
#include "careerrecord.h"
#include "clock.h"
#include "config.h"
#include "decomp.h"
#include "gamekeys.h"
#include "gamething.h"
#include "geocache.h"
#include "inradius.h"
#include "missionaudio.h"
#include "missionresult.h"
#include "navpoint.h"
#include "network.h"
#include "players.h"
#include "resourcename.h"
#include "shots.h"
#include "simmain.h"
#include "soundfx.h"
#include "speech.h"
#include "speechline.h"
#include "starmission.h"
#include "targeting.h"
#include "team.h"
#include "types.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// Set to end the mission as successful (a cheat).
// GLOBAL: MW2 0x100a3748
MechS32 g_forceMissionSuccess = 0;

// Set once the local team's mission result has been announced (AnnounceMissionResult).
// GLOBAL: MW2 0x100a374c
MechS32 g_missionResultAnnounced = 0;

// The tag of the mission result EndTheMission2 writes.
// GLOBAL: MW2 0x100a37bc
MechU32 g_missionResultTag = 0x4d32574d;

// GLOBAL: MW2 0x10138710
MechS32 g_missionTime;

// GLOBAL: MW2 0x10138720
MechS32 g_currentObjective[16]; // by team

// Whether each objective of the local team has been announced (AnnounceObjective).
// GLOBAL: MW2 0x10138760
MechS32 g_objectiveAnnounced[48];

// GLOBAL: MW2 0x10138820
MechS32 g_objectiveCount;

// GLOBAL: MW2 0x10138830
StarMission g_objectiveTable[16];

// Collapses each run of whitespace after a character of p_text into one space.
// FUNCTION: MW2 0x1001a910
void CollapseWhitespace(MechChar* p_text)
{
	MechChar* src;
	MechChar* dst;

	src = p_text;
	dst = p_text;
	while (*src) {
		*dst = *src++;
		if (isspace(*src)) {
			while (*src && isspace(*src)) {
				src++;
			}

			*++dst = ' ';
		}

		dst++;
	}

	*dst = '\0';
}

// Starts a star's mission: stamps its start time and places the team at the nav of its first
// objective, when that is a nav (or at the origin). Returns whether it was.
// Stack-slot permutation of the locals. The original tests the target's kind by loading its high
// byte and shifting it back ((MechU16) (kind << 8) == 0x100); the mask compiles to a byte compare.
// FUNCTION: MW2 0x1001aa02
MechS32 DoFirstObjtv(StarMission* p_mission, MechS32 p_team)
{
	MechS32 x;
	MechS32 y;
	MechS32 z;
	MechU16 nav;
	MechS32 heading;
	MechS32 found;

	x = 0;
	y = 0;
	z = 0;
	heading = 0;
	found = FALSE;
	g_missionTime = g_currentClock / 181;
	if (p_mission->m_objectives[0].m_targetCount > 0 && (p_mission->m_objectives[0].m_targets[0] & 0xff00) == 0x100) {
		nav = (MechU8) p_mission->m_objectives[0].m_targets[0];
		if (nav < g_navCount) {
			x = g_navTable[nav].m_position[0];
			y = g_navTable[nav].m_position[1];
			z = g_navTable[nav].m_position[2];
			heading = g_navTable[nav].m_heading;
			found = TRUE;
		}
	}

	PlaceTeam(p_team, x, y, z, heading);
	p_mission->m_startTime = g_missionTime;
	p_mission->m_objectives[0].m_startTime = g_missionTime;
	return found;
}

// Returns the state bits (0xe) of an objective target: a player's or a game thing's flags. A target
// is an AI target id stored as two bytes, the index and the type.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1001ab4a
MechS32 GetObjectiveTargetState(MechU8* p_target)
{
	MechU16 index;
	MechU16 kind;
	Player* player;
	GameThing* thing;

	index = p_target[0];
	kind = p_target[1] << 8;
	switch (kind) {
	case 0x200:
		player = g_players[index];
		return player->m_flags & 0xe;
	case 0x400:
		thing = &g_gameThings[index];
		return thing->m_flags & 0xe;
	case 0x100:
		return 0;
	}

	return 0;
}

// Returns whether team p_team has reached an objective target.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1001ac06
MechS32 HasTeamReachedTarget(MechU8* p_target, MechS32 p_team)
{
	MechU16 index;
	MechU16 kind;
	MechS32 reached;
	Player* player;
	GameThing* thing;
	NavPoint* nav;

	index = p_target[0];
	kind = p_target[1] << 8;
	reached = FALSE;
	switch (kind) {
	case 0x200:
		player = g_players[index];
		if ((player->m_flags & 0x20) && ((1 << p_team) & player->m_inspectedBy)) {
			reached = TRUE;
		}
		break;
	case 0x400:
		thing = &g_gameThings[index];
		if ((thing->m_flags & 0x20) && ((1 << p_team) & thing->m_teamsReached)) {
			reached = TRUE;
		}
		break;
	case 0x100:
		nav = &g_navTable[index];
		if ((nav->m_flags & 0x20) && ((1 << p_team) & nav->m_teamsReached)) {
			reached = TRUE;
		}
		break;
	}

	return reached;
}

// Returns whether a live member of team p_team is near an objective target (within 20000, or a
// nav's radius). The local player reaching a nav marks it reached and plays sound 0xe7.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1001ad5b
MechS32 IsTeamNearTarget(MechU8* p_target, MechS32 p_team)
{
	MechU16 index;
	MechU16 kind;
	MechS32 i;
	Player* target;
	GameThing* thing;
	MechS32 x;
	MechS32 y;
	MechS32 z;
	MechS32 radius;
	NavPoint* nav;

	index = p_target[0];
	kind = p_target[1] << 8;
	if (g_teams[p_team].m_leader < 0 && !g_isNetworkGame) {
		return FALSE;
	}

	switch (kind) {
	case 0x200:
		target = g_players[index];
		for (i = 0; i < g_playerCount; i++) {
			if (g_players[i]->m_team == p_team && !(g_players[i]->m_flags & 6) &&
				IsWithinRadius(
					g_players[i]->m_position.m_x - target->m_position.m_x,
					g_players[i]->m_position.m_y - target->m_position.m_y,
					g_players[i]->m_position.m_z - target->m_position.m_z,
					20000
				)) {
				return TRUE;
			}
		}
		return FALSE;
	case 0x400:
		thing = &g_gameThings[index];
		GetStaticObjectPosition(thing->m_staticObject, &x, &y, &z);
		for (i = 0; i < g_playerCount; i++) {
			if (g_players[i]->m_team == p_team && IsWithinRadius(
													  g_players[i]->m_position.m_x - x,
													  g_players[i]->m_position.m_y - y,
													  g_players[i]->m_position.m_z - z,
													  20000
												  )) {
				return TRUE;
			}
		}
		return FALSE;
	case 0x100:
		nav = &g_navTable[index];
		radius = nav->m_radius;
		if (radius <= 0) {
			radius = 20000;
		}

		for (i = 0; i < g_playerCount; i++) {
			if (g_players[i]->m_team == p_team && IsWithinRadius(
													  g_players[i]->m_position.m_x - nav->m_position[0],
													  g_players[i]->m_position.m_y - nav->m_position[1],
													  g_players[i]->m_position.m_z - nav->m_position[2],
													  radius
												  )) {
				if (i == g_localPlayerId && (g_players[i]->m_flags & 0x2000) && !(nav->m_flags & 0x20) && nav->m_used) {
					nav->m_flags |= 0x20;
					nav->m_teamsReached |= 1 << p_team;
					PlaySoundEffect(0xe7, 100, 0x40, 5, 0x50);
				}

				return TRUE;
			}
		}
		return FALSE;
	}

	return FALSE;
}

// Announces the local team's objective p_objective as successful (p_state 5) or failed (6), once.
// Stack-slot permutation; the original computes the objective's index before the star's (index order)
// and compares p_star with g_localStar in the other operand order.
// FUNCTION: MW2 0x1001b0cb
MechS32 AnnounceObjective(MechS32 p_star, MechS32 p_objective, MechS32 p_state)
{
	MechChar text[100];
	SpeechLine line;
	MissionObjective* objective;

	objective = &g_objectiveTable[p_star].m_objectives[p_objective];
	if (p_star != g_localStar || g_objectiveAnnounced[p_objective]) {
		return TRUE;
	}

	if (p_state == 5) {
		sprintf(text, "%s successful", objective->m_name);
		line.m_id = objective->m_successSpeech;
		line.m_data = ReadSoundFile(objective->m_successSound);
	}
	else if (p_state == 6) {
		sprintf(text, "%s failed", objective->m_name);
		line.m_id = objective->m_failSpeech;
		line.m_data = ReadSoundFile(objective->m_failSound);
	}

	if (text[0] == ' ' || !objective->m_listed) {
		line.m_text = "";
	}
	else {
		// The original collapsed line.m_text after the if, writing into the empty literal too
		line.m_text = text;
		CollapseWhitespace(line.m_text);
	}

	QueueSpeechLine(&line);
	g_objectiveAnnounced[p_objective] = TRUE;
	return TRUE;
}

// In a network game with a single listed objective, a secondary one, picks the player with the
// best kill score (kills of others minus kills of itself) as the winner, -2 on a tie.
// Stack-slot permutation; score > best compares in the other operand order.
// FUNCTION: MW2 0x1001b21a
void ChooseNetworkWinner(void)
{
	MissionObjective* objective;
	MechS32 best;
	MechS32 team;
	MechS32 winner;
	MechS32 i;
	MechS32 player;
	StarMission* mission;
	MechS32 victim;
	MechS32 score;
	MechS32 deathmatch;
	MechS32 secondary;
	MechS32 listed;

	best = -0x7fff;
	deathmatch = FALSE;
	mission = &g_objectiveTable[g_localStar];
	team = g_localStar;
	if (g_isNetworkGame && !g_difficulty->m_teamGame) {
		listed = 0;
		secondary = FALSE;
		for (i = 0; i < mission->m_objectiveCount; i++) {
			objective = &g_objectiveTable[g_localStar].m_objectives[i];
			listed += objective->m_listed;
			if (objective->m_listed && objective->m_priority == 2) {
				secondary = TRUE;
			}
		}

		if (secondary && listed == 1) {
			deathmatch = TRUE;
		}

		if (deathmatch) {
			winner = 0;
			for (player = 0; player < 8; player++) {
				score = 0;
				for (victim = 0; victim < 8; victim++) {
					if (player == victim) {
						score -= g_careerRecord.m_kills[player][victim];
					}
					else {
						score += g_careerRecord.m_kills[player][victim];
					}
				}

				if (score > best) {
					best = score;
					winner = player;
				}
				else if (score == best) {
					winner = -2;
				}
			}

			g_careerRecord.m_winner = winner;
		}
	}
}

// Announces the local team's mission result: successful (2), failed (3) or out of time (4). In a
// network game, only the first result; a successful one names the local player the winner.
// FUNCTION: MW2 0x1001b3f4
MechS32 AnnounceMissionResult(MechS32 p_star, MechS32 p_status)
{
	MechChar text[100];
	SpeechLine line;

	if (p_star != g_localStar) {
		return TRUE;
	}

	if (g_missionResultAnnounced && g_isNetworkGame) {
		return TRUE;
	}
	else {
		g_missionResultAnnounced = 1;
	}

	if (p_status == 2) {
		sprintf(text, "Mission successful");
		line.m_id = g_objectiveTable[p_star].m_successSpeech;
		line.m_data = ReadSoundFile(g_objectiveTable[p_star].m_successSound);
		if (g_isNetworkGame) {
			g_careerRecord.m_winner = g_localPlayerId;
			SendSuccessMsg();
		}
	}
	else if (p_status == 3) {
		sprintf(text, "Mission failed");
		line.m_id = g_objectiveTable[p_star].m_failSpeech;
		line.m_data = ReadSoundFile(g_objectiveTable[p_star].m_failSound);
	}
	else if (p_status == 4) {
		sprintf(text, "Mission time exceeded");
		line.m_id = FindResourceIdByName(0xb, "BET68");
		line.m_data = ReadSoundFile("BET68");
	}

	line.m_text = text;
	QueueSpeechLine(&line);
	return TRUE;
}

// Returns whether condition p_condition of star p_star's objective p_objective holds.
// Stack-slot permutation of objective, other, star, state and kind; the original scales star
// before other in the objective's address (index order).
// FUNCTION: MW2 0x1001b580
MechS32 TestObjectiveCondition(MechS32 p_star, MechS32 p_objective, MechS32 p_condition)
{
	MissionObjective* objective;
	MechS32 other;
	MechS32 star;
	MechS32 state;
	MechS32 kind;

	objective = &g_objectiveTable[p_star].m_objectives[p_objective];
	kind = objective->m_conditions[p_condition].m_kind;
	other = objective->m_conditions[p_condition].m_objective;
	star = objective->m_conditions[p_condition].m_star;
	state = g_objectiveTable[star].m_objectives[other].m_state;
	return (kind == 1 && (state == 5 || state == 6)) || (kind == 2 && state == 5) || (kind == 3 && state == 6) ? TRUE
																											   : FALSE;
}

// Returns whether the conditions of star p_star's objective p_objective hold: one of them, or
// every one. A finished objective's don't; a type 0x10 objective's always do.
// Stack-slot permutation of all, objective, result, i and holds; the original computes the
// objective's index before the star's (index order).
// FUNCTION: MW2 0x1001b66c
MechS32 ObjectiveConditionsHold(MechS32 p_star, MechS32 p_objective)
{
	MechS32 all;
	MissionObjective* objective;
	MechU32 result;
	MechS32 i;
	MechU32 holds;

	objective = &g_objectiveTable[p_star].m_objectives[p_objective];
	all = objective->m_allConditions;
	result = TRUE;
	if (objective->m_state == 5 || objective->m_state == 6) {
		return FALSE;
	}

	if (objective->m_type == 0x10) {
		return TRUE;
	}

	for (i = 0; i < 8; i++) {
		if (!objective->m_conditions[i].m_kind) {
			break;
		}

		holds = TestObjectiveCondition(p_star, p_objective, i);
		if (!all) {
			if (holds) {
				return TRUE;
			}
		}
		else {
			result &= holds;
			if (!result) {
				return FALSE;
			}
		}
	}

	if (!all) {
		return FALSE;
	}
	else {
		return TRUE;
	}
}

// Updates star p_star's objective p_objective while its conditions hold: whether its targets are
// done (by its type: destroyed, reached, ...), whether its time is up, and what it does when done
// (types 0x10000 and up end or reset other objectives and missions). Sets its state: 5 successful,
// 6 failed, 8 failed for another star, 3 still going.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1001b79a
void UpdateObjective(MechS32 p_star, MechS32 p_objective)
{
	MechS32 done;
	MissionObjective* objective;
	MechS32 expired;
	MechS32 i;
	MechS32 untouched;
	MechU8 state;
	MechS32 alive;
	StarMission* won;
	StarMission* lost;
	MissionObjective* failTarget;
	MissionObjective* reset;
	StarMission* restarted;
	MechS32 j;
	MissionObjective* restartTarget;
	MissionObjective* succeedTarget;

	done = TRUE;
	untouched = TRUE;
	state = 3;
	objective = &g_objectiveTable[p_star].m_objectives[p_objective];
	if (g_isNetworkGame && objective->m_state == 5 && objective->m_type == 2) {
		for (i = 0; i < objective->m_targetCount; i++) {
			if (done && GetObjectiveTargetState((MechU8*) &objective->m_targets[i])) {
				done = TRUE;
			}
			else {
				done = FALSE;
			}
		}

		if (!done) {
			objective->m_startTime = -1;
			objective->m_endTime = -1;
			objective->m_state = 3;
		}
	}

	if (objective->m_state == 5 || objective->m_state == 6 || !ObjectiveConditionsHold(p_star, p_objective)) {
		objective->m_active = 0;
		return;
	}

	if (objective->m_startTime == -1) {
		objective->m_startTime = g_missionTime;
		objective->m_active = 1;
	}

	switch (objective->m_type) {
	case 1:
	case 2:
		for (i = 0; i < objective->m_targetCount; i++) {
			if (done && GetObjectiveTargetState((MechU8*) &objective->m_targets[i])) {
				done = TRUE;
			}
			else {
				done = FALSE;
			}
		}
		break;
	case 4:
		for (i = 0; i < objective->m_targetCount; i++) {
			alive = GetObjectiveTargetState((MechU8*) &objective->m_targets[i]);
			if (done && !alive) {
				done = TRUE;
			}
			else {
				done = FALSE;
			}

			if (untouched && alive) {
				untouched = TRUE;
			}
			else {
				untouched = FALSE;
			}
		}
		break;
	case 8:
		for (i = 0; i < objective->m_targetCount; i++) {
			if (done && HasTeamReachedTarget((MechU8*) &objective->m_targets[i], p_star)) {
				done = TRUE;
			}
			else {
				done = FALSE;
			}
		}
		break;
	case 0x20:
		for (i = 0; i < objective->m_targetCount; i++) {
			if (done && HasTeamReachedTarget((MechU8*) &objective->m_targets[i], p_star)) {
				done = TRUE;
			}
			else {
				done = FALSE;
			}
		}

		for (i = 0; i < g_objectiveTable[p_star].m_objectiveCount; i++) {
			if (g_objectiveTable[p_star].m_objectives[i].m_mandatory && i != p_objective) {
				if (done && g_objectiveTable[p_star].m_objectives[i].m_state == 5) {
					done = TRUE;
				}
				else {
					done = FALSE;
				}
			}
		}
		break;
	case 0x100:
		if (g_localStar != p_star && !g_isNetworkGame) {
			if (done && g_currentObjective[p_star] == p_objective) {
				done = TRUE;
			}
			else {
				done = FALSE;
			}
		}

		for (i = 0; i < objective->m_targetCount; i++) {
			if (done && IsTeamNearTarget((MechU8*) &objective->m_targets[i], p_star)) {
				done = TRUE;
			}
			else {
				done = FALSE;
			}
		}
		break;
	case 0x2000:
		for (i = 0; i < objective->m_targetCount; i++) {
			if (done && !IsTeamNearTarget((MechU8*) &objective->m_targets[i], p_star)) {
				done = TRUE;
			}
			else {
				done = FALSE;
			}
		}
		break;
	case 0:
	case 0x10:
	case 0x40:
	case 0x80:
	case 0x200:
	case 0x400:
	case 0x800:
	case 0x1000:
	case 0x10000:
	case 0x20000:
	case 0x40000:
	case 0x80000:
	case 0x100000:
	case 0x200000:
	case 0x400000:
		// An empty block: these types have nothing to check here, but the original still dispatches
		// them to their own break.
		{
		}
		break;
	}

	if (objective->m_timeLimit >= 0 && objective->m_startTime >= 0) {
		if (g_missionTime - objective->m_startTime > objective->m_timeLimit) {
			expired = TRUE;
		}
		else {
			expired = FALSE;
		}
	}
	else {
		expired = FALSE;
	}

	switch (objective->m_type) {
	case 0x10000:
		if (objective->m_targetStar > g_objectiveCount) {
			break;
		}

		won = &g_objectiveTable[objective->m_targetStar];
		won->m_status = 2;
		AnnounceMissionResult(objective->m_targetStar, won->m_status);
		won->m_endTime = g_missionTime;
		state = 5;
		AnnounceObjective(p_star, p_objective, state);
		break;
	case 0x20000:
		if (objective->m_targetStar > g_objectiveCount) {
			break;
		}

		lost = &g_objectiveTable[objective->m_targetStar];
		lost->m_status = 3;
		AnnounceMissionResult(objective->m_targetStar, lost->m_status);
		lost->m_endTime = g_missionTime;
		state = 5;
		AnnounceObjective(p_star, p_objective, state);
		break;
	case 0x40000:
		if (objective->m_targetStar > g_objectiveCount ||
			g_objectiveTable[objective->m_targetStar].m_objectiveCount < objective->m_targetObjective) {
			break;
		}

		if (!g_objectiveTable[objective->m_targetStar].m_objectives[objective->m_targetObjective].m_listed) {
			g_objectiveTable[objective->m_targetStar].m_objectives[objective->m_targetObjective].m_listed = 1;
		}
		else {
			g_objectiveTable[objective->m_targetStar].m_objectives[objective->m_targetObjective].m_listed = 0;
		}

		state = 5;
		AnnounceObjective(p_star, p_objective, state);
		break;
	case 0x80000:
		if (objective->m_targetStar > g_objectiveCount ||
			g_objectiveTable[objective->m_targetStar].m_objectiveCount < objective->m_targetObjective) {
			break;
		}

		failTarget = &g_objectiveTable[objective->m_targetStar].m_objectives[objective->m_targetObjective];
		if (failTarget->m_state == 6 || failTarget->m_state == 5) {
			break;
		}

		failTarget->m_state = 6;
		failTarget->m_endTime = g_missionTime;
		AnnounceObjective(objective->m_targetStar, objective->m_targetObjective, failTarget->m_state);
		state = 5;
		AnnounceObjective(p_star, p_objective, state);
		break;
	case 0x200000:
		if (objective->m_targetStar > g_objectiveCount) {
			break;
		}

		restarted = &g_objectiveTable[objective->m_targetStar];
		for (j = 0; j < restarted->m_objectiveCount; j++) {
			reset = &restarted->m_objectives[j];
			reset->m_state = 3;
			reset->m_startTime = -1;
			reset->m_endTime = -1;
			if (reset->m_type != 0x10) {
				g_objectiveAnnounced[j] = 0;
			}
		}

		restarted->m_status = 0;
		restarted->m_endTime = -1;
		state = 5;
		AnnounceObjective(p_star, p_objective, state);
		break;
	case 0x400000:
		if (objective->m_targetStar > g_objectiveCount ||
			g_objectiveTable[objective->m_targetStar].m_objectiveCount < objective->m_targetObjective) {
			break;
		}

		// The original clears the announcement of objective i, the last loop's counter.
		restartTarget = &g_objectiveTable[objective->m_targetStar].m_objectives[objective->m_targetObjective];
		restartTarget->m_state = 3;
		restartTarget->m_startTime = -1;
		restartTarget->m_endTime = -1;
		if (restartTarget->m_type != 0x10) {
			g_objectiveAnnounced[i] = 0;
		}

		state = 5;
		AnnounceObjective(p_star, p_objective, state);
		break;
	case 0x100000:
		if (objective->m_targetStar > g_objectiveCount ||
			g_objectiveTable[objective->m_targetStar].m_objectiveCount < objective->m_targetObjective) {
			break;
		}

		succeedTarget = &g_objectiveTable[objective->m_targetStar].m_objectives[objective->m_targetObjective];
		if (succeedTarget->m_state == 6 || succeedTarget->m_state == 5) {
			break;
		}

		succeedTarget->m_state = 5;
		succeedTarget->m_endTime = g_missionTime;
		AnnounceObjective(objective->m_targetStar, objective->m_targetObjective, succeedTarget->m_state);
		state = 5;
		AnnounceObjective(p_star, p_objective, state);
		break;
	case 0:
	case 0x200:
	case 0x400:
	case 0x800:
		if (expired) {
			if (g_players[g_teams[p_star].m_leader] && (g_players[g_teams[p_star].m_leader]->m_flags & 2) &&
				g_isNetworkGame) {
				return;
			}

			state = 5;
			AnnounceObjective(p_star, p_objective, state);
		}
		break;
	case 4:
		if (expired) {
			if (done) {
				state = 5;
				AnnounceObjective(p_star, p_objective, state);
			}
			else {
				state = 6;
			}
		}
		else {
			if (!done) {
				if (g_localStar == p_star) {
					state = 6;
				}
				else {
					state = 8;
				}

				AnnounceObjective(p_star, p_objective, 6);
			}

			if (untouched) {
				state = 6;
			}
		}
		break;
	default:
		if (done) {
			state = 5;
			AnnounceObjective(p_star, p_objective, state);
		}
		else if (expired) {
			state = 6;
			AnnounceObjective(p_star, p_objective, state);
		}
		break;
	}

	objective->m_state = state;
	if (state != 3) {
		objective->m_endTime = g_missionTime;
	}
}

// Updates every star's mission each frame: its objectives (UpdateObjective), then, while it is in
// progress, whether it succeeded (every listed objective done), failed or ran out of time; and
// its current objective, the first whose conditions hold (ResetStarOrders hears of changes).
// FUNCTION: MW2 0x1001c69e
void UpdateObjectives(void)
{
	MechS32 star;
	MechS32 i;
	MechS32 current;
	StarMission* mission;
	MechS32 success;
	MechS32 failed;

	g_missionTime = g_currentClock / 181;
	for (star = 0; star < g_objectiveCount; star++) {
		mission = &g_objectiveTable[star];
		for (i = 0; i < mission->m_objectiveCount; i++) {
			UpdateObjective(star, i);
		}

		if (mission->m_status == 0) {
			success = TRUE;
			failed = FALSE;
			for (i = 0; i < mission->m_objectiveCount; i++) {
				if (mission->m_objectives[i].m_mandatory) {
					switch (mission->m_objectives[i].m_state) {
					case 5:
						if (success) {
							success = TRUE;
						}
						else {
							success = FALSE;
						}
						break;
					case 6:
					case 8:
						success = FALSE;
						failed = TRUE;
						break;
					default:
						success = FALSE;
						break;
					}
				}
			}

			if (success || g_forceMissionSuccess) {
				mission->m_status = 2;
				AnnounceMissionResult(star, mission->m_status);
				mission->m_endTime = g_missionTime;
			}
			else if (failed) {
				mission->m_status = 3;
				AnnounceMissionResult(star, mission->m_status);
				mission->m_endTime = g_missionTime;
			}
			else if (mission->m_timeLimit > 0 && g_missionTime - mission->m_startTime >= mission->m_timeLimit) {
				mission->m_status = 4;
				AnnounceMissionResult(star, mission->m_status);
				mission->m_endTime = g_missionTime;
			}
		}
		else if (star == g_localStar && !g_missionTimerStopped && !g_speechQueue && !g_missionResolved) {
			g_missionResolved = 1;
			g_missionEnded = 0;
			g_missionEndTime = 0;
		}

		current = -1;
		for (i = 0; i < mission->m_objectiveCount; i++) {
			if (ObjectiveConditionsHold(star, i) && !(g_objectiveTable[star].m_objectives[i].m_type & 0xffff0000)) {
				current = i;
				break;
			}
		}

		if (g_currentObjective[star] != current) {
			g_currentObjective[star] = current;
			ResetStarOrders(star);
		}
	}
}

// Counts the mission time in seconds.
// FUNCTION: MW2 0x1001c9d5
void EndTheMission1(void)
{
	g_missionTime = g_currentClock / 181;
	return;
}

// Hands the local team's mission result to the shell. Returns whether it could.
// FUNCTION: MW2 0x1001c9f7
MechS32 EndTheMission2(void)
{
	MissionResult result;
	StarMission* mission;
	MechS32 i;
	MechS32 count;

	count = 0;
	mission = &g_objectiveTable[g_localStar];
	memset(&result, 0, sizeof(result));
	result.m_tag = g_missionResultTag;
	result.m_startTime = mission->m_startTime;
	result.m_endTime = mission->m_endTime;
	result.m_outcome = mission->m_status;
	for (i = 0; i < mission->m_objectiveCount; i++) {
		if (!mission->m_objectives[i].m_listed) {
			continue;
		}

		if (mission->m_objectives[i].m_state == 5) {
			result.m_objectives[count].m_succeeded = 1;
		}
		else {
			result.m_objectives[count].m_succeeded = 0;
		}

		result.m_objectives[count].m_type = mission->m_objectives[i].m_priority;
		result.m_objectives[count].m_startTime = mission->m_objectives[i].m_startTime;
		result.m_objectives[count].m_endTime = mission->m_objectives[i].m_endTime;
		result.m_objectives[count].m_mandatory = mission->m_objectives[i].m_mandatory;
		// The original strcpy'd, running a longer name into the next objective's fields
		strncpy(
			result.m_objectives[count].m_name,
			mission->m_objectives[i].m_name,
			sizeof(result.m_objectives[count].m_name) - 1
		);
		count++;
	}

	result.m_objectiveCount = count;

	if (g_missionReport == NULL) {
		return FALSE;
	}

	g_missionReport->m_result = result;
	return TRUE;
}

// Restarts star p_star's mission (a player's own, in a network game): every objective goes back
// to state 3 with no times. A completed objective of type 0x40000 toggles whether the objective
// it names is listed on the objectives panel.
// FUNCTION: MW2 0x1001cc5c
void RestartStarMission(MechS32 p_star)
{
	MissionObjective* objective;
	MechS32 i;
	StarMission* mission;

	g_missionTime = g_currentClock / 181;
	mission = &g_objectiveTable[p_star];
	for (i = 0; i < mission->m_objectiveCount; i++) {
		objective = &mission->m_objectives[i];
		if (objective->m_type == 0x40000 && objective->m_state == 5) {
			if (!g_objectiveTable[objective->m_targetStar].m_objectives[objective->m_targetObjective].m_listed) {
				g_objectiveTable[objective->m_targetStar].m_objectives[objective->m_targetObjective].m_listed = 1;
			}
			else {
				g_objectiveTable[objective->m_targetStar].m_objectives[objective->m_targetObjective].m_listed = 0;
			}
		}

		objective->m_state = 3;
		objective->m_startTime = -1;
		objective->m_endTime = -1;
	}

	mission->m_status = 0;
	mission->m_startTime = g_missionTime;
	mission->m_endTime = -1;
}

// FUNCTION: MW2 0x1001cdd1
void FUN_1001cdd1(void)
{
	return;
}

// FUNCTION: MW2 0x1001cde1
MechS32 FUN_1001cde1(undefined4 p_unk0x00)
{
	return 1;
}

// The target of team p_team's return objective (type 0x20), or else its first objective's
// first target: the AI's "home" nav.
// Stack-slot permutation: mission and i.
// FUNCTION: MW2 0x1001cdf6
MechU16 GetTeamHomeTarget(MechS32 p_team)
{
	StarMission* mission;
	MechS32 i;

	mission = &g_objectiveTable[p_team];
	for (i = 0; i < mission->m_objectiveCount; i++) {
		if (mission->m_objectives[i].m_type == 0x20) {
			return mission->m_objectives[i].m_targets[0];
		}
	}

	return mission->m_objectives[0].m_targets[0];
}
