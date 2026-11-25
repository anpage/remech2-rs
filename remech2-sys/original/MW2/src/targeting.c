#include "targeting.h"

#include "cockpit.h"
#include "cockpitreadout.h"
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

int32_t g_navCount = 0;

// What inspecting the local player's target did (UpdateTarget): 1 inspected it, 2 too far, 3
// already inspected by the team, 0 nothing. The target panel reports it.
int32_t g_inspectResult = 0;

// Set once the local player has targeted at the reticle (TargetAtReticle), cleared when they
// cycle targets: while it is clear, the local player's cycling skips game pieces flagged 0x10
// and anything without flag 0x400 or 0x1000.
int32_t g_reticleTargeting = 0;

// The cockpit layouts' text buffers and saved viewports.

char g_satelliteRangeText[0x40];

char g_satelliteExtraText[0x20];

PANE g_satelliteSavedViewport;

char g_smallMapRangeText[0x20];

char g_smallMapExtraText[0x20];

char g_readoutText[0x20];

PANE g_largeMapSavedViewport;

char g_largeMapRangeText[0x20];

char g_bearingText[0x20];

PANE g_smallMapSavedViewport;

char g_largeMapExtraText[0x20];

// The cockpit views' layouts (g_cockpitLayouts): the map view of cockpit views 1 and 2 and the
// satellite view (4), with their labels, icons, colors, rectangles and transitions.

char g_readoutLabel[8] = "x";

CockpitReadout g_readout = {1, -1, g_readoutLabel, g_readoutText, {0x28f, 0x28f}};

CockpitReadout* g_cockpitReadout = &g_readout;

char g_smallMapExtraLabel[4] = "x";

char g_smallMapRangeLabel[4] = "R: ";

char g_smallMapBearingLabel[12] = "Bearing: ";

char g_largeMapExtraLabel[4] = "x";

char g_largeMapRangeLabel[8] = "R: ";

char g_largeMapBearingLabel[12] = "Bearing: ";

char g_satelliteExtraLabel[4] = "x";

char g_satelliteRangeLabel[8] = "Range: ";

char g_bearingLabel[12] = "Bearing: ";

char g_metersUnit[4] = "m";

char g_kilometersUnit[8] = "km";

int32_t g_smallMapColors[7][3] = {
	{0xac, 0xac, 0xac},
	{0x7c, 0x79, 0x7f},
	{0x91, 0x8e, 0x94},
	{0x9a, 0x97, 0x9d},
	{0x85, 0x88, 0x85},
	{0xa0, 0x118, 0xa0},
	{0xa6, 0xa3, 0xa9}
};

int32_t g_largeMapColors[7][3] = {
	{0xac, 0xac, 0xac},
	{0x7c, 0x79, 0x7f},
	{0x91, 0x8e, 0x94},
	{0x9a, 0x97, 0x9d},
	{0x85, 0x88, 0x85},
	{0xa0, 0x118, 0xa0},
	{0xa6, 0xa3, 0xa9}
};

int32_t g_satelliteColors[7][3] = {
	{0xac, 0xac, 0xac},
	{0x7c, 0x79, 0x7f},
	{0x91, 0x8e, 0x94},
	{0x9a, 0x97, 0x9d},
	{0x85, 0x88, 0x85},
	{0xa0, 0x118, 0xa0},
	{0xa6, 0xa3, 0xa9}
};

int32_t g_mapColors[13] = {0xe, 0xa, 6, 0xf, 0xb, 0xf5, 2, 3, 0xf9, 0xff, 0xf0, 1, 2};

int32_t g_smallMapAnims[5] = {-1, -1, -1, -1, -1};

int32_t g_largeMapAnims[5] = {-1, -1, -1, -1, -1};

int32_t g_satelliteAnims[5] = {-1, -1, -1, -1, -1};

PANE g_smallMapTransitionFirst = {NULL, 0x8000, 0x8000, 0x8000, 0x8000};

PANE g_smallMapTransitionSecond = {NULL, 0, 0, 0x10000, 0x10000};

PANE g_smallMapTransitionRect = {NULL, 0, 0, 0, 0};

RectTransitionState g_mapTransitionState = {0, 0, 0};

RectTransitionDef g_smallMapTransitionDef =
	{0xb5, &g_smallMapTransitionFirst, &g_smallMapTransitionSecond, &g_smallMapTransitionRect};

RectTransition g_smallMapTransition = {&g_mapTransitionState, &g_smallMapTransitionDef};

PANE g_largeMapTransitionFirst = {NULL, 0x8000, 0x8000, 0x8000, 0x8000};

PANE g_largeMapTransitionSecond = {NULL, 0, 0, 0x10000, 0x10000};

PANE g_largeMapTransitionRect = {NULL, 0, 0, 0, 0};

RectTransitionDef g_largeMapTransitionDef =
	{0xb5, &g_largeMapTransitionFirst, &g_largeMapTransitionSecond, &g_largeMapTransitionRect};

RectTransition g_largeMapTransition = {&g_mapTransitionState, &g_largeMapTransitionDef};

PANE g_satelliteTransitionFirst = {NULL, 0x599a, 0x599a, 0xa666, 0xa666};

PANE g_satelliteTransitionSecond = {NULL, 0, 0, 0x10000, 0x10000};

PANE g_satelliteTransitionRect = {NULL, 0, 0, 0, 0};

RectTransitionState g_satelliteTransitionState = {0, 0, 0};

RectTransitionDef g_satelliteTransitionDef =
	{0x21f, &g_satelliteTransitionFirst, &g_satelliteTransitionSecond, &g_satelliteTransitionRect};

RectTransition g_satelliteTransition = {&g_satelliteTransitionState, &g_satelliteTransitionDef};

PANE g_smallMapViewport = {NULL, 0x51f, 0x51f, 0x428f, 0x570a};

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

PANE g_largeMapViewport = {NULL, 0x2148, 0, 0xdeb8, 0x10000};

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

PANE g_satelliteViewport = {NULL, 0, 0, 0x10000, 0x10000};

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
CockpitLayout* g_cockpitLayouts[6] = {NULL, &g_smallMapLayout, &g_largeMapLayout, NULL, &g_satelliteLayout, NULL};

NavPoint g_navTable[128];

// Places a nav point for player p_owner at (p_x, p_y, p_z), named "!". Returns its index, or
// -1 if the table is full.
int32_t AddNavPoint(uint32_t p_owner, int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t index;
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
int32_t RemoveNavPoint(uint32_t p_owner, uint32_t p_nav)
{
	int32_t index;
	int32_t i;
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
void CycleTarget(Player* p_player, int32_t p_step, uint32_t p_flags)
{
	int32_t target;
	int32_t result;
	int32_t found;
	int32_t index;
	int32_t limit;
	int32_t tries;
	int32_t kind;

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
int32_t TargetNavPoint(uint32_t p_player, int32_t p_nav, uint32_t p_flags)
{
	Player* player;
	NavPoint* nav;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t dx;
	int32_t dy;
	int32_t dz;
	int32_t heading;
	int32_t range;
	uint32_t distance;
	int32_t pitch;

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
int32_t TargetGamePiece(int32_t p_player, int32_t p_index, uint32_t p_flags)
{
	Player* player;
	Player* target;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t dx;
	int32_t dy;
	int32_t dz;
	int32_t heading;
	int32_t range;
	uint32_t distance;
	int32_t pitch;

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
int32_t TargetGameThing(int32_t p_player, int32_t p_index, uint32_t p_flags)
{
	Player* player;
	GameThing* thing;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t dx;
	int32_t dy;
	int32_t dz;
	int32_t heading;
	int32_t range;
	uint32_t distance;
	int32_t pitch;

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
int32_t UpdateTarget(Player* p_player)
{
	int32_t isLocal;
	int32_t claim;
	int32_t lost;
	int32_t kind;
	int32_t index;

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
int32_t GetLocalTargetGamePiece(void)
{
	int32_t index;
	Player* player;
	int32_t kind;

	player = g_players[g_localPlayerId];
	kind = player->m_targetInfo.m_target & 0xf00;
	index = player->m_targetInfo.m_target & 0xff;
	if (kind != 0x200) {
		index = -1;
	}

	return index;
}

// Returns the game thing the local player targets, or -1.
int32_t GetLocalTargetGameThing(void)
{
	int32_t index;
	Player* player;
	int32_t kind;

	player = g_players[g_localPlayerId];
	kind = player->m_targetInfo.m_target & 0xf00;
	index = player->m_targetInfo.m_target & 0xff;
	if (kind != 0x400) {
		index = -1;
	}

	return index;
}

// Returns the shape of the local player's target, or NULL.
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
SceneObject* GetLocalTargetObject(void)
{
	int32_t index;
	Player* player;
	SceneObject* obj;
	int32_t kind;
	int32_t id;

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
void TargetAtReticle(void)
{
	int32_t autopilot;
	Player* player;
	int32_t target;
	int32_t previous;
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
void GetBearingAndRange(
	int32_t p_dx,
	int32_t p_dy,
	int32_t p_dz,
	int32_t* p_heading,
	int32_t* p_length,
	uint32_t* p_distance,
	int32_t* p_pitch
)
{
	int32_t pitch;
	int32_t cosine;
	int32_t ground;
	int32_t heading;
	int32_t sine;
	int32_t length;
	int32_t pitchCosine;

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
void CycleNavTarget(Player* p_player, int32_t p_step, int32_t p_ownOnly)
{
	uint32_t flags;

	flags = 1;
	if (p_ownOnly) {
		flags |= 0x100;
	}

	CycleTarget(p_player, p_step, flags);
}

void CycleGameThingTarget(int32_t p_step)
{
	Player* player;

	player = g_players[g_localPlayerId];
	CycleTarget(player, p_step, 4);
}

void CycleGamePieceTarget(int32_t p_step)
{
	Player* player;

	player = g_players[g_localPlayerId];
	CycleTarget(player, p_step, 2);
}

void CycleFriendlyTarget(int32_t p_step)
{
	Player* player;

	player = g_players[g_localPlayerId];
	CycleTarget(player, p_step, 0x20008);
}

void CycleEnemyTarget(int32_t p_step)
{
	Player* player;

	player = g_players[g_localPlayerId];
	CycleTarget(player, p_step, 0x40008);
}

// Selects the local player's nearest target within 0x2ab98, cycling through them all; keeps the
// current one if there is none or UpdateTarget refuses it, and then clears the autopilot's
// steering flag (PlayerSteering::m_autopilot).
void TargetNearestEnemy(void)
{
	int32_t autopilot;
	int32_t target;
	Player* player;
	int32_t best;
	int32_t saved;
	int32_t distance;
	int32_t bestDistance;

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
