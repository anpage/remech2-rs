#include "team.h"

#include "ai.h"
#include "gamething.h"
#include "object.h"
#include "players.h"
#include "speech.h"
#include "targeting.h"
#include "transform.h"
#include "types.h"

#include <string.h>
#include <strings.h>

// Teams of players: each has a leader and up to eight members, a side, and a formation that
// places each member (by its slot) relative to the leader. Formations are copied from a
// table of named templates.

int32_t g_localStar = -1;

int32_t g_formationTemplateCount = 0;

TeamFormation g_formationTemplates[32];

Team g_teams[16];

int32_t g_starSides[8];

TeamFormation g_teamFormations[16];

void ResetTeams(void)
{
	uint32_t i;

	for (i = 0; i < 16; i++) {
		g_teams[i].m_leader = 0;
		g_teams[i].m_formation = 0;
		g_teamFormations[i] = g_formationTemplates[0];
	}
}

int32_t SetTeamFormationByName(int32_t p_team, const char* p_name)
{
	int32_t result = false;
	int32_t i;

	if (p_team >= 16) {
		return false;
	}

	for (i = 0; i < g_formationTemplateCount; i++) {
		if (strcasecmp(g_formationTemplates[i].m_name, p_name) == 0) {
			g_teams[p_team].m_formation = i;
			g_teamFormations[p_team] = g_formationTemplates[i];
			result = true;
			break;
		}
	}

	return result;
}

int32_t GetTeamFormation(int32_t p_team)
{
	if (p_team >= 16) {
		return 0;
	}

	return g_teams[p_team].m_formation;
}

void SetTeamFormation(int32_t p_team, int32_t p_formation)
{
	if (p_team >= 16 || p_formation >= 32) {
		return;
	}

	if (p_team == g_localStar) {
		SayFormation(p_formation);
	}

	g_teams[p_team].m_formation = p_formation;
	g_teamFormations[p_team] = g_formationTemplates[p_formation];
}

int32_t SetPlayerSlot(uint32_t p_player, uint32_t p_slot)
{
	int32_t result = false;

	if (p_player < g_playerCount && p_slot < 8) {
		g_players[p_player]->m_slot = p_slot;
		result = true;
	}

	return result;
}

// Makes a player the team's leader and moves the formation's origin to the leader's slot.
int32_t SetTeamLeader(int32_t p_team, int32_t p_player)
{
	int32_t result;
	uint16_t i;
	int32_t dz;
	Player* player;
	int32_t dx;

	result = false;
	if (p_player == -1) {
		g_teams[p_team].m_leader = p_player;
	}
	else if (p_player < g_playerCount && p_team < 16) {
		g_teams[p_team].m_leader = p_player;

		player = g_players[p_player];
		dx = g_teamFormations[p_team].m_x[player->m_slot];
		dz = g_teamFormations[p_team].m_z[player->m_slot];
		for (i = 0; i < 8; i++) {
			g_teamFormations[p_team].m_x[i] -= dx;
			g_teamFormations[p_team].m_z[i] -= dz;
		}

		result = true;
	}

	return result;
}

// Places the team's leader, then every member at its formation slot around the leader.
int32_t PlaceTeam(int32_t p_team, int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_heading)
{
	int32_t result;
	int32_t z;
	uint32_t leader;
	Player* player;
	int32_t heading;
	int32_t x;
	uint32_t i;

	result = false;
	if (p_team < 16 && (leader = g_teams[p_team].m_leader) < g_playerCount) {
		player = g_players[leader];
		player->m_position.m_x = p_x;
		player->m_position.m_y = p_y;
		player->m_position.m_z = p_z;
		player->m_heading = p_heading;

		for (i = 0; i < g_playerCount; i++) {
			player = g_players[i];
			if (player->m_team == p_team) {
				GetTeamSlotPosition(i, &x, &z, &heading);
				SetObjPosition(player->m_obj, x, p_y, z);
				SetObjRotation(player->m_obj, 0, heading, 0, 0);
				UpdateObj(player->m_obj);
				player->m_position.m_x = x;
				player->m_position.m_y = p_y;
				player->m_position.m_z = z;
				player->m_heading = heading;
			}
		}

		result = true;
	}

	return result;
}

// Returns where a player's formation slot puts it: the leader's own position, or the slot's
// offset transformed by the leader's orientation.
int32_t GetTeamSlotPosition(uint32_t p_player, int32_t* p_x, int32_t* p_z, int32_t* p_heading)
{
	int32_t slot;
	uint32_t leader;
	Matrix* matrix;
	SceneObject* obj;
	int32_t y;
	int32_t team;

	y = 0;
	if (p_player >= g_playerCount) {
		return false;
	}

	team = g_players[p_player]->m_team;
	leader = GetTeamLeader(team);
	slot = g_players[p_player]->m_slot;
	if (leader != p_player) {
		*p_x = g_teamFormations[team].m_x[slot];
		*p_z = g_teamFormations[team].m_z[slot];
		*p_heading = g_teamFormations[team].m_heading[slot];
		obj = g_players[leader]->m_obj;
		matrix = GetObjWorldMatrix(obj);
		TransformPoint(matrix, p_x, &y, p_z);
	}
	else {
		*p_x = g_players[leader]->m_position.m_x;
		*p_z = g_players[leader]->m_position.m_z;
		*p_heading = g_players[leader]->m_heading;
	}

	return true;
}

int32_t GetTeamLeader(int32_t p_team)
{
	int32_t result = 0;

	if (p_team < 16) {
		result = g_teams[p_team].m_leader;
	}

	return result;
}

// Numbers the team's active members' formation slots from 1, the leader taking slot 0.
int32_t AssignTeamSlots(int32_t p_team)
{
	Team* team;
	Player* player;
	int16_t i;
	int16_t slot;
	int16_t leader;

	team = &g_teams[p_team];
	leader = GetTeamLeader(p_team);
	if (leader == -1) {
		leader = ChooseTeamLeader(p_team);
		if (leader == -1) {
			return false;
		}
	}

	for (i = 0, slot = 1; i < team->m_memberCount; i++) {
		player = g_players[team->m_members[i]];
		if (player->m_type == c_playerTypeMech && !(player->m_flags & 6)) {
			if (player->m_index == leader) {
				player->m_slot = 0;
			}
			else {
				player->m_slot = slot;
				slot++;
			}
		}
	}

	return true;
}

int32_t GetPlayerSide(int32_t p_player)
{
	return g_teams[g_players[p_player]->m_team].m_side;
}

int32_t GetThingSide(int32_t p_thing)
{
	int32_t index;

	index = g_gameThings[p_thing].m_affiliation;
	if (index < 0) {
		return 2;
	}
	else {
		return g_starSides[index];
	}
}

// The side of the team that owns nav p_nav; 2 past the first 16 navs.
int32_t GetNavSide(uint32_t p_nav)
{
	int32_t side;

	side = 2;
	if (p_nav < 16) {
		side = g_teams[g_navTable[p_nav].m_team].m_side;
	}

	return side;
}

int32_t OnSameSide(int32_t p_playerA, int32_t p_playerB)
{
	return GetPlayerSide(p_playerB) == GetPlayerSide(p_playerA);
}
