#ifndef TEAM_H
#define TEAM_H

#include "teamformation.h"
#include "types.h"

typedef struct Team {
	int32_t m_leader;       // 0x00 — a player index, or -1
	int32_t m_memberCount;  // 0x04
	uint32_t m_affiliation; // 0x08
	int32_t m_side;         // 0x10
	int32_t m_formation;    // 0x14 — the template g_teamFormations copies
	int32_t m_members[8];   // 0x18 — player indices
} Team;

// The functions and globals of team.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_localStar;
	extern int32_t g_formationTemplateCount;
	extern TeamFormation g_formationTemplates[32];
	extern Team g_teams[16];
	extern int32_t g_starSides[8];
	extern TeamFormation g_teamFormations[16];

	void ResetTeams(void);
	int32_t SetTeamFormationByName(int32_t p_team, const char* p_name);
	int32_t GetTeamFormation(int32_t p_team);
	void SetTeamFormation(int32_t p_team, int32_t p_formation);
	int32_t SetPlayerSlot(uint32_t p_player, uint32_t p_slot);
	int32_t SetTeamLeader(int32_t p_team, int32_t p_player);
	int32_t PlaceTeam(int32_t p_team, int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_heading);
	int32_t GetTeamSlotPosition(uint32_t p_player, int32_t* p_x, int32_t* p_z, int32_t* p_heading);
	int32_t GetTeamLeader(int32_t p_team);
	int32_t AssignTeamSlots(int32_t p_team);
	int32_t GetPlayerSide(int32_t p_player);
	int32_t GetThingSide(int32_t p_thing);
	int32_t GetNavSide(uint32_t p_nav);
	int32_t OnSameSide(int32_t p_playerA, int32_t p_playerB);

#ifdef __cplusplus
}
#endif

#endif // TEAM_H
