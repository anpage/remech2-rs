#include "targeting.h"

#include "cockpit.h"
#include "cockpitreadout.h"
#include "decomp.h"
#include "fixeddiv29.h"
#include "fixedtrig.h"
#include "gamething.h"
#include "geocache.h"
#include "mech.h"
#include "object.h"
#include "players.h"
#include "playersteering.h"
#include "recttransition.h"
#include "shape.h"
#include "simmain.h"
#include "soundfx.h"
#include "team.h"
#include "types.h"
#include "weapons.h"

#include <stdlib.h>

// Targeting: the nav points and the AI target ids (0x100 | nav, 0x200 | game piece, 0x400 |
// game thing; 0x1000 marks the target lost), cycling the target through them, and the cockpit's
// map and satellite views. The flags of CycleTarget pick what it may select: 1 nav points only
// (0x100 too: only the player's own), 2 game pieces only, 4 game things only, 8 game pieces and
// things, with 0x10000 those flagged 0x40, 0x20000 friends and 0x40000 enemies (GetPlayerSide).

DECOMP_SIZE_ASSERT(WINDOW, 0x14)
DECOMP_SIZE_ASSERT(PANE, 0x14)
DECOMP_SIZE_ASSERT(NavPoint, 0x54)

// GLOBAL: MW2 0x100aaba4
MechS32 g_navCount = 0;

// What inspecting the local player's target did (UpdateTarget): 1 inspected it, 2 too far, 3
// already inspected by the team, 0 nothing. The target panel reports it.
// GLOBAL: MW2 0x100aaba8
MechS32 g_inspectResult = 0;

// Set once the local player has targeted at the reticle (TargetAtReticle), cleared when they
// cycle targets: while it is clear, the local player's cycling skips game pieces flagged 0x10
// and anything without flag 0x400 or 0x1000.
// GLOBAL: MW2 0x100aabac
MechS32 g_reticleTargeting = 0;

// The cockpit layouts' text buffers and saved viewports.

// GLOBAL: MW2 0x100e9450
MechChar g_satelliteRangeText[0x40];

// GLOBAL: MW2 0x100e9490
MechChar g_satelliteExtraText[0x20];

// GLOBAL: MW2 0x100e94b0
PANE g_satelliteSavedViewport;

// GLOBAL: MW2 0x100e94d0
MechChar g_smallMapRangeText[0x20];

// GLOBAL: MW2 0x100e94f0
MechChar g_smallMapExtraText[0x20];

// GLOBAL: MW2 0x100e9510
MechChar g_readoutText[0x20];

// GLOBAL: MW2 0x100e9530
PANE g_largeMapSavedViewport;

// GLOBAL: MW2 0x100e9550
MechChar g_largeMapRangeText[0x20];

// GLOBAL: MW2 0x100e9570
MechChar g_bearingText[0x20];

// GLOBAL: MW2 0x100e9590
PANE g_smallMapSavedViewport;

// GLOBAL: MW2 0x100e95d0
MechChar g_largeMapExtraText[0x20];

// The cockpit views' layouts (g_cockpitLayouts): the map view of cockpit views 1 and 2 and the
// satellite view (4), with their labels, icons, colors, rectangles and transitions.

// GLOBAL: MW2 0x100aabb0
MechChar g_readoutLabel[8] = "x";

// GLOBAL: MW2 0x100aabb8
CockpitReadout g_readout = {1, 0, -1, g_readoutLabel, g_readoutText, {0x28f, 0x28f}};

// GLOBAL: MW2 0x100aabd4
CockpitReadout* g_cockpitReadout = &g_readout;

// GLOBAL: MW2 0x100aabd8
MechChar g_smallMapExtraLabel[4] = "x";

// GLOBAL: MW2 0x100aabdc
MechChar g_smallMapRangeLabel[4] = "R: ";

// GLOBAL: MW2 0x100aabe0
MechChar g_smallMapBearingLabel[12] = "Bearing: ";

// GLOBAL: MW2 0x100aabec
MechChar g_largeMapExtraLabel[4] = "x";

// GLOBAL: MW2 0x100aabf0
MechChar g_largeMapRangeLabel[8] = "R: ";

// GLOBAL: MW2 0x100aabf8
MechChar g_largeMapBearingLabel[12] = "Bearing: ";

// GLOBAL: MW2 0x100aac04
MechChar g_satelliteExtraLabel[4] = "x";

// GLOBAL: MW2 0x100aac08
MechChar g_satelliteRangeLabel[8] = "Range: ";

// GLOBAL: MW2 0x100aac10
MechChar g_bearingLabel[12] = "Bearing: ";

// GLOBAL: MW2 0x100aac1c
MechChar g_metersUnit[4] = "m";

// GLOBAL: MW2 0x100aac20
MechChar g_kilometersUnit[8] = "km";

// GLOBAL: MW2 0x100aac28
MechS32 g_smallMapColors[7][3] = {
	{0xac, 0xac, 0xac},
	{0x7c, 0x79, 0x7f},
	{0x91, 0x8e, 0x94},
	{0x9a, 0x97, 0x9d},
	{0x85, 0x88, 0x85},
	{0xa0, 0x118, 0xa0},
	{0xa6, 0xa3, 0xa9}
};

// GLOBAL: MW2 0x100aac80
MechS32 g_largeMapColors[7][3] = {
	{0xac, 0xac, 0xac},
	{0x7c, 0x79, 0x7f},
	{0x91, 0x8e, 0x94},
	{0x9a, 0x97, 0x9d},
	{0x85, 0x88, 0x85},
	{0xa0, 0x118, 0xa0},
	{0xa6, 0xa3, 0xa9}
};

// GLOBAL: MW2 0x100aacd8
MechS32 g_satelliteColors[7][3] = {
	{0xac, 0xac, 0xac},
	{0x7c, 0x79, 0x7f},
	{0x91, 0x8e, 0x94},
	{0x9a, 0x97, 0x9d},
	{0x85, 0x88, 0x85},
	{0xa0, 0x118, 0xa0},
	{0xa6, 0xa3, 0xa9}
};

// GLOBAL: MW2 0x100aad30
MechS32 g_mapColors[13] = {0xe, 0xa, 6, 0xf, 0xb, 0xf5, 2, 3, 0xf9, 0xff, 0xf0, 1, 2};

// GLOBAL: MW2 0x100aad68
MechS32 g_smallMapAnims[5] = {-1, -1, -1, -1, -1};

// GLOBAL: MW2 0x100aad80
MechS32 g_largeMapAnims[5] = {-1, -1, -1, -1, -1};

// GLOBAL: MW2 0x100aad98
MechS32 g_satelliteAnims[5] = {-1, -1, -1, -1, -1};

// GLOBAL: MW2 0x100aadb0
PANE g_smallMapTransitionFirst = {NULL, 0x8000, 0x8000, 0x8000, 0x8000};

// GLOBAL: MW2 0x100aadc8
PANE g_smallMapTransitionSecond = {NULL, 0, 0, 0x10000, 0x10000};

// GLOBAL: MW2 0x100aade0
PANE g_smallMapTransitionRect = {NULL, 0, 0, 0, 0};

// GLOBAL: MW2 0x100aadf8
RectTransitionState g_mapTransitionState = {0, 0, 0};

// GLOBAL: MW2 0x100aae08
RectTransitionDef g_smallMapTransitionDef =
	{0xb5, &g_smallMapTransitionFirst, &g_smallMapTransitionSecond, &g_smallMapTransitionRect};

// GLOBAL: MW2 0x100aae18
RectTransition g_smallMapTransition = {&g_mapTransitionState, &g_smallMapTransitionDef};

// GLOBAL: MW2 0x100aae20
PANE g_largeMapTransitionFirst = {NULL, 0x8000, 0x8000, 0x8000, 0x8000};

// GLOBAL: MW2 0x100aae38
PANE g_largeMapTransitionSecond = {NULL, 0, 0, 0x10000, 0x10000};

// GLOBAL: MW2 0x100aae50
PANE g_largeMapTransitionRect = {NULL, 0, 0, 0, 0};

// GLOBAL: MW2 0x100aae68
RectTransitionDef g_largeMapTransitionDef =
	{0xb5, &g_largeMapTransitionFirst, &g_largeMapTransitionSecond, &g_largeMapTransitionRect};

// GLOBAL: MW2 0x100aae78
RectTransition g_largeMapTransition = {&g_mapTransitionState, &g_largeMapTransitionDef};

// GLOBAL: MW2 0x100aae80
PANE g_satelliteTransitionFirst = {NULL, 0x599a, 0x599a, 0xa666, 0xa666};

// GLOBAL: MW2 0x100aae98
PANE g_satelliteTransitionSecond = {NULL, 0, 0, 0x10000, 0x10000};

// GLOBAL: MW2 0x100aaeb0
PANE g_satelliteTransitionRect = {NULL, 0, 0, 0, 0};

// GLOBAL: MW2 0x100aaec8
RectTransitionState g_satelliteTransitionState = {0, 0, 0};

// GLOBAL: MW2 0x100aaed8
RectTransitionDef g_satelliteTransitionDef =
	{0x21f, &g_satelliteTransitionFirst, &g_satelliteTransitionSecond, &g_satelliteTransitionRect};

// GLOBAL: MW2 0x100aaee8
RectTransition g_satelliteTransition = {&g_satelliteTransitionState, &g_satelliteTransitionDef};

// GLOBAL: MW2 0x100aaef0
PANE g_smallMapViewport = {NULL, 0x51f, 0x51f, 0x428f, 0x570a};

// GLOBAL: MW2 0x100aaf08
CockpitLayout g_smallMapLayout = {
	&g_smallMapViewport,
	&g_smallMapSavedViewport,
	8,
	{-1, -1},
	&g_smallMapTransition,
	0x30d40,
	-1,
	0x30d40,
	0xc350,
	0x61a80,
	0,
	1,
	g_smallMapExtraLabel,
	g_smallMapExtraText,
	g_smallMapRangeLabel,
	g_smallMapRangeText,
	g_bearingLabel,
	g_bearingText,
	-1,
	g_metersUnit,
	g_kilometersUnit,
	{0, 0},
	{0, 0xa3d},
	{0, 0},
	g_smallMapColors,
	g_mapColors,
	g_smallMapAnims,
	{(CockpitGaugeFn) 2, (CockpitGaugeFn) 4, (CockpitGaugeFn) 6, (CockpitGaugeFn) 8}
};

// GLOBAL: MW2 0x100aaf98
PANE g_largeMapViewport = {NULL, 0x2148, 0, 0xdeb8, 0x10000};

// GLOBAL: MW2 0x100aafb0
CockpitLayout g_largeMapLayout = {
	&g_largeMapViewport,
	&g_largeMapSavedViewport,
	9,
	{-1, -1},
	&g_largeMapTransition,
	0x30d40,
	-1,
	0x30d40,
	0xc350,
	0x61a80,
	0,
	1,
	g_largeMapExtraLabel,
	g_largeMapExtraText,
	g_largeMapRangeLabel,
	g_largeMapRangeText,
	g_bearingLabel,
	g_bearingText,
	-1,
	g_metersUnit,
	g_kilometersUnit,
	{0, 0},
	{0x147b, 0x2148},
	{0, 0},
	g_largeMapColors,
	g_mapColors,
	g_largeMapAnims,
	{(CockpitGaugeFn) 2, (CockpitGaugeFn) 4, (CockpitGaugeFn) 6, (CockpitGaugeFn) 8}
};

// GLOBAL: MW2 0x100ab040
PANE g_satelliteViewport = {NULL, 0, 0, 0x10000, 0x10000};

// GLOBAL: MW2 0x100ab058
CockpitLayout g_satelliteLayout = {
	&g_satelliteViewport,
	&g_satelliteSavedViewport,
	10,
	{0xe8, 0xe8},
	&g_satelliteTransition,
	0x186a0,
	-1,
	0x186a0,
	0x186a,
	0x186a0,
	0,
	1,
	g_satelliteExtraLabel,
	g_satelliteExtraText,
	g_satelliteRangeLabel,
	g_satelliteRangeText,
	g_bearingLabel,
	g_bearingText,
	-1,
	g_metersUnit,
	g_kilometersUnit,
	{0, 0},
	{0x28f, 0x28f},
	{0x28f, 0xccd},
	g_satelliteColors,
	g_mapColors,
	g_satelliteAnims,
	{(CockpitGaugeFn) 0, (CockpitGaugeFn) 3, (CockpitGaugeFn) 5, (CockpitGaugeFn) 7}
};

// The layout of each cockpit view, NULL where it has none.
// GLOBAL: MW2 0x100ab0e8
CockpitLayout* g_cockpitLayouts[6] = {NULL, &g_smallMapLayout, &g_largeMapLayout, NULL, &g_satelliteLayout, NULL};

// GLOBAL: MW2 0x10177160
NavPoint g_navTable[128];

// Places a nav point for player p_owner at (p_x, p_y, p_z), named "!". Returns its index, or
// -1 if the table is full.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1005ec80
MechS32 AddNavPoint(MechU32 p_owner, MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	MechS32 index;
	NavPoint* nav;

	index = -1;
	if (g_navCount < 0x80 && g_navCount != -1) {
		nav = &g_navTable[g_navCount];
		nav->m_position[0] = p_x;
		nav->m_position[1] = p_y;
		nav->m_position[2] = p_z;
		nav->m_used = 1;
		nav->m_flags = 0x401;
		nav->m_owner = p_owner | 0x200;
		nav->m_team = g_players[p_owner]->m_team;
		nav->m_radius = 3000;
		nav->m_obj = NULL;
		nav->m_name[0] = '!';
		nav->m_name[1] = '\0';
		index = g_navCount;
		g_navCount++;
	}

	return index;
}

// Removes nav p_nav (an AI target id, 0x100 | index) if player p_owner placed it: the later navs
// move down, and so do the players' targets, goals and navs pointing past it (not the local
// player's own target). Returns the new nav count, or -1.
// Stack-slot permutation of the locals. Operand order: the second loop test (i < g_playerCount)
// compares with i in eax in the original.
// FUNCTION: MW2 0x1005ed4f
MechS32 RemoveNavPoint(MechU32 p_owner, MechU32 p_nav)
{
	MechS32 index;
	MechS32 i;
	Player* player;

	if (!(p_nav & 0x100)) {
		return -1;
	}

	index = p_nav & 0xff;
	if (index >= g_navCount) {
		return -1;
	}

	if (!(g_navTable[index].m_flags & 1) || g_navTable[index].m_owner != (p_owner | 0x200)) {
		return -1;
	}

	for (i = index; i < g_navCount - 1; i++) {
		g_navTable[i] = g_navTable[i + 1];
	}

	for (i = 0; i < g_playerCount; i++) {
		player = g_players[i];
		if (!player) {
			continue;
		}

		if (player->m_ai.m_target & 0x100 && (player->m_ai.m_target & 0xff) > index) {
			player->m_ai.m_target--;
		}

		if (player->m_ai.m_goal & 0x100 && (player->m_ai.m_goal & 0xff) > index) {
			player->m_ai.m_goal--;
		}

		if (player->m_targetInfo.m_target & 0x100 && (player->m_targetInfo.m_target & 0xff) > index &&
			player->m_index != g_localPlayerId) {
			player->m_targetInfo.m_target--;
		}

		if (player->m_nav & 0x100 && (player->m_nav & 0xff) > index) {
			player->m_nav--;
		}
	}

	g_navCount--;
	return g_navCount;
}

// Steps p_player's target p_step places through the nav points, the players and the game
// things, in that cycle; with p_step 0, or no target, it starts over from the first player. Marks
// the target missing (0x1000) when none qualifies, and a change of target outside the nav points
// turns the autopilot's steering over.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1005ef5e
void CycleTarget(Player* p_player, MechS32 p_step, MechU32 p_flags)
{
	MechS32 target;
	MechS32 result;
	MechS32 found;
	MechS32 index;
	MechS32 limit;
	MechS32 tries;
	MechS32 kind;

	found = FALSE;
	if (p_player->m_index == g_localPlayerId) {
		g_reticleTargeting = 0;
	}

	target = p_player->m_targetInfo.m_target;
	kind = target & 0xf00;
	index = target & 0xff;
	if (p_step == 0 || (target & 0x1000)) {
		target = kind = 0x200;
		index = 0;
		p_step = 1;
	}
	else {
		index += p_step;
	}

	limit = g_playerCount + g_navCount + g_gameThingCount + 3;
	tries = 0;
	while (tries++ < limit && !found) {
		switch (kind) {
		case 0x100:
			result = TargetNavPoint(p_player->m_index, index, p_flags);
			switch (result) {
			case -1:
				index = g_gameThingCount - 1;
				kind = 0x400;
				break;
			case -2:
				index = 0;
				kind = 0x200;
				break;
			case 1:
				found = TRUE;
				break;
			default:
				index += p_step;
				break;
			}
			break;
		case 0x200:
			result = TargetGamePiece(p_player->m_index, index, p_flags);
			switch (result) {
			case -1:
				index = g_navCount - 1;
				kind = 0x100;
				break;
			case -2:
				index = 0;
				kind = 0x400;
				break;
			case 1:
				found = TRUE;
				break;
			default:
				index += p_step;
				break;
			}
			break;
		case 0x400:
			result = TargetGameThing(p_player->m_index, index, p_flags);
			switch (result) {
			case -1:
				index = g_playerCount - 1;
				kind = 0x200;
				break;
			case -2:
				index = 0;
				kind = 0x100;
				break;
			case 1:
				found = TRUE;
				break;
			default:
				index += p_step;
				break;
			}
			break;
		default:
			index = 0;
			kind = 0x200;
			break;
		}
	}

	if (found) {
		p_player->m_targetInfo.m_target = kind | index;
	}
	else {
		p_player->m_targetInfo.m_target |= 0x1000;
	}

	if (kind != 0x100 && p_player->m_mech->m_autopilot) {
		p_player->m_steering->m_autopilot = 1;
	}
}

// Marks the local player's target (bit 0x1000).
// FUNCTION: MW2 0x1005f284
void ResetTargeting(void)
{
	Player* player;

	player = g_players[g_localPlayerId];
	player->m_targetInfo.m_target |= 0x1000;
}

// Makes nav point p_nav player p_player's target: its position, heading and distance. Returns 1,
// or a negative code: -1 and -2 for an index out of range, -9 for flags 0xe, -3 for a free nav,
// -4 when flag 0x10000 asks for a nav with flag 0x40, and -6 for a nav of another owner or team
// (flag 0x100 accepts only a nav its owner placed).
// Stack-slot permutation of the locals; p_nav >= g_navCount compares in the other operand order.
// FUNCTION: MW2 0x1005f2ae
MechS32 TargetNavPoint(MechU32 p_player, MechS32 p_nav, MechU32 p_flags)
{
	Player* player;
	NavPoint* nav;
	MechS32 x;
	MechS32 y;
	MechS32 z;
	MechS32 dx;
	MechS32 dy;
	MechS32 dz;
	MechS32 heading;
	MechS32 range;
	MechU32 distance;
	MechS32 pitch;

	if (p_nav < 0) {
		return -1;
	}

	if (p_nav >= g_navCount) {
		return -2;
	}

	if (p_flags & 0xe) {
		return -9;
	}

	player = g_players[p_player];
	nav = &g_navTable[p_nav];
	if (!nav->m_used) {
		return -3;
	}

	if ((p_flags & 0x10000) && !(nav->m_flags & 0x40)) {
		return -4;
	}

	if (nav->m_flags & 1) {
		if (nav->m_owner != (p_player | 0x200)) {
			return -6;
		}
	}
	else if (p_flags & 0x100) {
		return -6;
	}

	if (player->m_team != nav->m_team) {
		return -6;
	}

	if (nav->m_obj) {
		GetObjPosition(nav->m_obj, &nav->m_position[0], &nav->m_position[1], &nav->m_position[2]);
	}

	x = nav->m_position[0];
	y = nav->m_position[1];
	z = nav->m_position[2];
	dx = x - player->m_position.m_x;
	dy = y - player->m_position.m_y;
	dz = z - player->m_position.m_z;
	GetBearingAndRange(dx, dy, dz, &heading, &range, &distance, &pitch);
	player->m_targetInfo.m_position.m_x = x;
	player->m_targetInfo.m_position.m_y = y;
	player->m_targetInfo.m_position.m_z = z;
	player->m_targetInfo.m_heading = heading;
	player->m_targetInfo.m_range = range;
	player->m_targetInfo.m_distance = distance;
	player->m_targetInfo.m_pitch = pitch;
	return 1;
}

// Makes player p_index player p_player's target, like TargetNavPoint for a nav point. Returns 1, or
// a negative code: -1 and -2 for an index out of range, -9 for flags 0x15 or the wrong side
// (flags 0x20000 and 0x40000), -3 for a player that can't be targeted or is p_player, -4 when
// flag 0x10000 asks for a player with flag 0x40, -5 and -7 for targets the local player may not
// pick, and -8 for a player without a shape.
// Stack-slot permutation of the locals; p_player == g_localPlayerId compares in the other operand
// order.
// FUNCTION: MW2 0x1005f4ac
MechS32 TargetGamePiece(MechS32 p_player, MechS32 p_index, MechU32 p_flags)
{
	MechS32 breakpoint;
	Player* player;
	Player* target;
	MechS32 x;
	MechS32 y;
	MechS32 z;
	MechS32 dx;
	MechS32 dy;
	MechS32 dz;
	MechS32 heading;
	MechS32 range;
	MechU32 distance;
	MechS32 pitch;

	if (p_index == 1 && p_player == g_localPlayerId) {
		breakpoint = 0;
	}

	if (p_index < 0) {
		return -1;
	}

	if (p_index >= g_playerCount) {
		return -2;
	}

	if (p_flags & 0x15) {
		return -9;
	}

	player = g_players[p_player];
	target = g_players[p_index];
	if (!g_reticleTargeting && (target->m_flags & 0x10) && p_player == g_localPlayerId) {
		return -3;
	}

	if (target->m_flags & 6) {
		return -3;
	}

	if (target->m_index == p_player) {
		return -3;
	}

	if ((p_flags & 0x10000) && !(target->m_flags & 0x40)) {
		return -4;
	}

	if ((p_flags & 0x20000) && GetPlayerSide(p_index)) {
		return -9;
	}

	if ((p_flags & 0x40000) && GetPlayerSide(p_index) != 1) {
		return -9;
	}

	if (!g_reticleTargeting && !(target->m_flags & 0x1400) && p_player == g_localPlayerId) {
		return -5;
	}

	if ((target->m_flags & 0x800) && p_player == g_localPlayerId) {
		return -5;
	}

	if (!target->m_obj) {
		return -8;
	}

	if (!GetObjShape(target->m_obj)) {
		return -8;
	}

	x = target->m_position.m_x;
	y = target->m_position.m_y;
	z = target->m_position.m_z;
	dx = x - player->m_position.m_x;
	dy = y - player->m_position.m_y;
	dz = z - player->m_position.m_z;
	GetBearingAndRange(dx, dy, dz, &heading, &range, &distance, &pitch);
	if (!(target->m_flags & 0x1000) && range > 0x2ab98 && p_player == g_localPlayerId) {
		return -7;
	}

	player->m_targetInfo.m_position.m_x = x;
	player->m_targetInfo.m_position.m_y = y;
	player->m_targetInfo.m_position.m_z = z;
	player->m_targetInfo.m_heading = heading;
	player->m_targetInfo.m_range = range;
	player->m_targetInfo.m_distance = distance;
	player->m_targetInfo.m_pitch = pitch;
	return 1;
}

// Makes game thing p_index player p_player's target, like TargetGamePiece for a player. Returns 1,
// or the same negative codes; its test of flag 0x10000 can never succeed.
// Stack-slot permutation of the locals; p_player == g_localPlayerId compares in the other operand
// order.
// FUNCTION: MW2 0x1005f798
MechS32 TargetGameThing(MechS32 p_player, MechS32 p_index, MechU32 p_flags)
{
	Player* player;
	GameThing* thing;
	MechS32 x;
	MechS32 y;
	MechS32 z;
	MechS32 dx;
	MechS32 dy;
	MechS32 dz;
	MechS32 heading;
	MechS32 range;
	MechU32 distance;
	MechS32 pitch;

	if (p_index < 0) {
		return -1;
	}

	if (p_index >= g_gameThingCount) {
		return -2;
	}

	if (p_flags & 0x23) {
		return -9;
	}

	player = g_players[p_player];
	thing = &g_gameThings[p_index];
	if (thing->m_flags & 4) {
		return -3;
	}

	if (p_flags & 0x10000 & !(thing->m_flags & 0x40)) {
		return -4;
	}

	if ((p_flags & 0x20000) && GetThingSide(p_index)) {
		return -9;
	}

	if ((p_flags & 0x40000) && GetThingSide(p_index) != 1) {
		return -9;
	}

	if (!g_reticleTargeting && !(thing->m_flags & 0x1400) && p_player == g_localPlayerId) {
		return -5;
	}

	if ((thing->m_flags & 0x800) && p_player == g_localPlayerId) {
		return -5;
	}

	if (!GetStaticSceneObject(thing->m_staticObject)) {
		return -8;
	}

	if (!GetStaticObjectShape(thing->m_staticObject)) {
		return -8;
	}

	GetStaticObjectPosition(thing->m_staticObject, &x, &y, &z);
	dx = x - player->m_position.m_x;
	dy = y - player->m_position.m_y;
	dz = z - player->m_position.m_z;
	GetBearingAndRange(dx, dy, dz, &heading, &range, &distance, &pitch);
	if (!(thing->m_flags & 0x1000) && range > 0x2ab98 && p_player == g_localPlayerId) {
		return -7;
	}

	player->m_targetInfo.m_position.m_x = x;
	player->m_targetInfo.m_position.m_y = y;
	player->m_targetInfo.m_position.m_z = z;
	player->m_targetInfo.m_heading = heading;
	player->m_targetInfo.m_range = range;
	player->m_targetInfo.m_distance = distance;
	player->m_targetInfo.m_pitch = pitch;
	return 1;
}

// Revalidates p_player's target (m_targetInfo.m_target: a nav, player or game thing index) and
// updates the target info. Reaching a nav target's radius marks the nav reached by the team; with
// the inspect key (PlayerSteering::m_inspectTarget) pressed, a player or game thing within 20000
// of its radius is inspected for the team. Returns 0, flagging the target lost (0x1000), when it no longer
// qualifies.
// The only diff is a stack-slot permutation of lost, claim, index and kind.
// FUNCTION: MW2 0x1005fa22
MechS32 UpdateTarget(Player* p_player)
{
	MechS32 isLocal;
	MechS32 claim;
	MechS32 lost;
	MechS32 kind;
	MechS32 index;

	lost = TRUE;
	claim = 0;
	isLocal = FALSE;
	if (p_player->m_index == g_localPlayerId) {
		isLocal = TRUE;
	}

	index = p_player->m_targetInfo.m_target & 0xff;
	kind = p_player->m_targetInfo.m_target & 0xf00;
	switch (kind) {
	case 0x200:
		if (TargetGamePiece(p_player->m_index, index, 0) >= 0) {
			lost = FALSE;
		}
		break;
	case 0x400:
		if (TargetGameThing(p_player->m_index, index, 0) >= 0) {
			lost = FALSE;
		}
		break;
	case 0x100:
		if (TargetNavPoint(p_player->m_index, index, 0) >= 0) {
			lost = FALSE;
			if (g_navTable[index].m_radius > p_player->m_targetInfo.m_distance && p_player->m_mech->m_autopilot != 1 &&
				p_player->m_index == g_localPlayerId) {
				if (!(g_navTable[index].m_flags & 0x20)) {
					g_navTable[index].m_flags |= 0x20;
					g_navTable[index].m_teamsReached |= 1 << p_player->m_team;
					PlaySoundEffect(0xe7, 100, 0x40, 5, 0x50);
				}

				CycleNavTarget(p_player, 1, 0);
			}
		}
		break;
	default:
		break;
	}

	if (lost) {
		p_player->m_targetInfo.m_target |= 0x1000;
		return 0;
	}

	if (isLocal) {
		g_inspectResult = 0;
	}

	claim = p_player->m_steering->m_inspectTarget;
	if (claim) {
		p_player->m_steering->m_inspectTarget = 0;
		switch (kind) {
		case 0x200:
			if (!(g_players[index]->m_inspectedBy & (1 << p_player->m_team))) {
				if (g_players[index]->m_mech->m_radius + 20000 > p_player->m_targetInfo.m_distance) {
					if (isLocal) {
						g_inspectResult = 1;
					}

					g_players[index]->m_flags |= 0x20;
					g_players[index]->m_inspectedBy |= 1 << p_player->m_team;
				}
				else if (isLocal) {
					g_inspectResult = 2;
				}
			}
			else if (isLocal) {
				g_inspectResult = 3;
			}
			break;
		case 0x400:
			if (!(g_gameThings[index].m_teamsReached & (1 << p_player->m_team))) {
				if (g_gameThings[index].m_radius + 20000 > p_player->m_targetInfo.m_distance) {
					if (isLocal) {
						g_inspectResult = 1;
					}

					g_gameThings[index].m_flags |= 0x20;
					g_gameThings[index].m_teamsReached |= 1 << p_player->m_team;
				}
				else if (isLocal) {
					g_inspectResult = 2;
				}
			}
			else if (isLocal) {
				g_inspectResult = 3;
			}
			break;
		default:
			break;
		}
	}

	return 1;
}

// Returns the player the local player targets, or -1.
// The only diff is a stack-slot permutation of index, player and kind.
// FUNCTION: MW2 0x1005fe63
MechS32 GetLocalTargetGamePiece(void)
{
	MechS32 index;
	Player* player;
	MechS32 kind;

	player = g_players[g_localPlayerId];
	kind = player->m_targetInfo.m_target & 0xf00;
	index = player->m_targetInfo.m_target & 0xff;
	if (kind != 0x200) {
		index = -1;
	}

	return index;
}

// Returns the game thing the local player targets, or -1.
// The only diff is a stack-slot permutation of index, player and kind.
// FUNCTION: MW2 0x1005febe
MechS32 GetLocalTargetGameThing(void)
{
	MechS32 index;
	Player* player;
	MechS32 kind;

	player = g_players[g_localPlayerId];
	kind = player->m_targetInfo.m_target & 0xf00;
	index = player->m_targetInfo.m_target & 0xff;
	if (kind != 0x400) {
		index = -1;
	}

	return index;
}

// Returns the shape of the local player's target, or NULL.
// FUNCTION: MW2 0x1005ff19
Shape* GetLocalTargetShape(void)
{
	SceneObject* obj;

	obj = GetLocalTargetObject();
	if (obj) {
		return GetObjShape(obj);
	}
	else {
		return NULL;
	}
}

// Returns the scene object of the local player's target: a player's or a game thing's.
// The only diff is a stack-slot permutation of index, player, obj, kind and id.
// FUNCTION: MW2 0x1005ff56
SceneObject* GetLocalTargetObject(void)
{
	MechS32 index;
	Player* player;
	SceneObject* obj;
	MechS32 kind;
	MechS32 id;

	obj = NULL;
	player = g_players[g_localPlayerId];
	kind = player->m_targetInfo.m_target & 0xf00;
	index = player->m_targetInfo.m_target & 0xff;
	switch (kind) {
	case 0x200:
		obj = g_players[index]->m_obj;
		break;
	case 0x400:
		id = g_gameThings[index].m_staticObject;
		obj = GetStaticSceneObject(id);
		break;
	default:
		break;
	}

	return obj;
}

// Targets the shape the local player points at (g_aimedShape): a player's mech (0x100) or a
// game thing (0x200), when TargetGamePiece or TargetGameThing allows it. When UpdateTarget rejects the
// new target, the old one comes back, with the autopilot.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10060010
void TargetAtReticle(void)
{
	MechS32 autopilot;
	Player* player;
	MechS32 target;
	MechS32 previous;
	Shape* shape;

	target = -1;
	autopilot = FALSE;
	player = g_players[g_localPlayerId];
	previous = player->m_targetInfo.m_target;
	if (player->m_mech->m_autopilot == 1) {
		autopilot = TRUE;
	}

	shape = g_aimedShape;
	if (shape) {
		if (shape->m_kind & 0x100) {
			if (TargetGamePiece(g_localPlayerId, shape->m_owner, 0) >= 0) {
				target = shape->m_owner | 0x200;
			}
		}
		else if (shape->m_kind & 0x200) {
			if (TargetGameThing(g_localPlayerId, shape->m_owner, 0) >= 0) {
				target = shape->m_owner | 0x400;
			}
		}

		if (target != -1 && player->m_targetInfo.m_target != target) {
			player->m_targetInfo.m_target = target;
			if (player->m_mech->m_autopilot) {
				player->m_steering->m_autopilot = 1;
			}

			if (!UpdateTarget(player)) {
				player->m_targetInfo.m_target = previous;
				if (autopilot) {
					player->m_steering->m_autopilot = 0;
					player->m_mech->m_autopilot = 1;
				}
			}
		}
	}

	g_reticleTargeting = 1;
}

// Turns the vector (p_dx, p_dy, p_dz) into its heading (*p_heading), its length along the
// ground (*p_distance), its full length (*p_length) and its pitch (*p_pitch).
// The abs(p_dx)/abs(p_dz) comparison evaluates its operands in the opposite order (one attempt at
// swapping them didn't flip it), and stack-slot permutation: every local.
// FUNCTION: MW2 0x10060197
void GetBearingAndRange(
	MechS32 p_dx,
	MechS32 p_dy,
	MechS32 p_dz,
	MechS32* p_heading,
	MechS32* p_length,
	MechU32* p_distance,
	MechS32* p_pitch
)
{
	MechS32 pitch;
	MechS32 cosine;
	MechS32 ground;
	MechS32 heading;
	MechS32 sine;
	MechS32 length;
	MechS32 pitchCosine;

	heading = FixedAtan2(p_dx, p_dz);
	if (abs(p_dx) > abs(p_dz)) {
		sine = FixedSin(heading);
		if (sine) {
			ground = FixedDiv29(p_dx, sine);
		}
		else {
			ground = 0;
		}
	}
	else {
		cosine = FixedCos(heading);
		if (cosine) {
			ground = FixedDiv29(p_dz, cosine);
		}
		else {
			ground = 0;
		}
	}

	pitch = FixedAtan2(p_dy, ground);
	pitchCosine = FixedCos(pitch);
	if (pitchCosine) {
		length = FixedDiv29(ground, pitchCosine);
	}
	else {
		length = 0;
	}

	*p_heading = heading;
	*p_distance = ground;
	*p_length = length;
	*p_pitch = pitch;
}

// Steps p_player's nav point by p_step; with p_ownOnly, through the navs p_player placed only.
// FUNCTION: MW2 0x100602b2
void CycleNavTarget(Player* p_player, MechS32 p_step, MechS32 p_ownOnly)
{
	MechU32 flags;

	flags = 1;
	if (p_ownOnly) {
		flags |= 0x100;
	}

	CycleTarget(p_player, p_step, flags);
}

// FUNCTION: MW2 0x100602ec
void CycleGameThingTarget(MechS32 p_step)
{
	Player* player;

	player = g_players[g_localPlayerId];
	CycleTarget(player, p_step, 4);
}

// FUNCTION: MW2 0x1006031b
void CycleGamePieceTarget(MechS32 p_step)
{
	Player* player;

	player = g_players[g_localPlayerId];
	CycleTarget(player, p_step, 2);
}

// FUNCTION: MW2 0x1006034a
void CycleFriendlyTarget(MechS32 p_step)
{
	Player* player;

	player = g_players[g_localPlayerId];
	CycleTarget(player, p_step, 0x20008);
}

// FUNCTION: MW2 0x1006037c
void CycleEnemyTarget(MechS32 p_step)
{
	Player* player;

	player = g_players[g_localPlayerId];
	CycleTarget(player, p_step, 0x40008);
}

// Selects the local player's nearest target within 0x2ab98, cycling through them all; keeps the
// current one if there is none or UpdateTarget refuses it, and then clears the autopilot's
// steering flag (PlayerSteering::m_autopilot).
// The distance/bestDistance comparison loads its operands in the opposite order (one attempt at
// swapping them didn't flip it), and stack-slot permutation: every local.
// FUNCTION: MW2 0x100603ae
void TargetNearestEnemy(void)
{
	MechS32 autopilot;
	MechS32 target;
	Player* player;
	MechS32 best;
	MechS32 saved;
	MechS32 distance;
	MechS32 bestDistance;

	target = 0;
	best = -1;
	bestDistance = 0x7fffffff;
	autopilot = FALSE;
	player = g_players[g_localPlayerId];
	saved = player->m_targetInfo.m_target;
	if (player->m_mech->m_autopilot == 1) {
		autopilot = TRUE;
	}

	CycleEnemyTarget(0);
	while (best != target) {
		target = player->m_targetInfo.m_target;
		if (target & 0x1000) {
			break;
		}

		distance = player->m_targetInfo.m_range;
		if (distance < bestDistance) {
			best = target;
			bestDistance = distance;
			target = 0;
		}

		CycleEnemyTarget(1);
	}

	if (best == -1 || bestDistance > 0x2ab98) {
		player->m_targetInfo.m_target = saved;
		if (autopilot) {
			player->m_steering->m_autopilot = 0;
		}
	}
	else {
		player->m_targetInfo.m_target = best;
		if (!UpdateTarget(player)) {
			player->m_targetInfo.m_target = saved;
			if (autopilot) {
				player->m_steering->m_autopilot = 0;
			}
		}
	}
}
