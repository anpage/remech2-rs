#ifndef TEAM_H
#define TEAM_H

#include "decomp.h"
#include "teamformation.h"
#include "types.h"

// SIZE 0x38
typedef struct Team {
	MechS32 m_leader;         // 0x00 — a player index, or -1
	MechS32 m_memberCount;    // 0x04
	undefined4 m_affiliation; // 0x08
	undefined4 m_unk0x0c;     // 0x0c
	MechS32 m_side;           // 0x10
	MechS32 m_formation;      // 0x14 — the template g_teamFormations copies
	MechS32 m_members[8];     // 0x18 — player indices
} Team;

// The functions and globals of team.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_localStar;
	extern MechS32 g_formationTemplateCount;
	extern TeamFormation g_formationTemplates[32];
	extern Team g_teams[16];
	extern MechS32 g_starSides[8];
	extern TeamFormation g_teamFormations[16];

	void ResetTeams(void);
	MechS32 SetTeamFormationByName(MechS32 p_team, const MechChar* p_name);
	MechS32 GetTeamFormation(MechS32 p_team);
	void SetTeamFormation(MechS32 p_team, MechS32 p_formation);
	MechS32 SetPlayerSlot(MechU32 p_player, MechU32 p_slot);
	MechS32 SetTeamLeader(MechS32 p_team, MechS32 p_player);
	MechS32 PlaceTeam(MechS32 p_team, MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_heading);
	MechS32 GetTeamSlotPosition(MechU32 p_player, MechS32* p_x, MechS32* p_z, MechS32* p_heading);
	MechS32 GetTeamLeader(MechS32 p_team);
	MechS32 AssignTeamSlots(MechS32 p_team, MechS32 p_unk0x04);
	MechS32 GetPlayerSide(MechS32 p_player);
	MechS32 GetThingSide(MechS32 p_thing);
	MechS32 GetNavSide(MechU32 p_nav);
	MechS32 OnSameSide(MechS32 p_playerA, MechS32 p_playerB);

#ifdef __cplusplus
}
#endif

#endif // TEAM_H
