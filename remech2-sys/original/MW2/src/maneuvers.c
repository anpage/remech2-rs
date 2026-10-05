#include "maneuvers.h"

#include "ai.h"
#include "aiweapons.h"
#include "approxlen.h"
#include "clock.h"
#include "collision.h"
#include "decomp.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "geocache.h"
#include "hud.h"
#include "maneuvertable.h"
#include "mech.h"
#include "mw2log.h"
#include "object.h"
#include "players.h"
#include "playersteering.h"
#include "point.h"
#include "random.h"
#include "ray.h"
#include "shape.h"
#include "simmain.h"
#include "targeting.h"
#include "transform.h"
#include "types.h"
#include "view.h"
#include "weapondata.h"
#include "weapons.h"
#include "weaponslot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Sixteen directions around a mech, as (x, z) divisors of a length: BuildProbeRay's probe rays
// and GetOffsetPoint's offsets.
// GLOBAL: MW2 0x100a2900
Point g_probeDirections[16] = {
	{0, 1},
	{2, 1},
	{1, 1},
	{1, 2},
	{1, 0},
	{1, -2},
	{1, -1},
	{2, -1},
	{0, -1},
	{-2, -1},
	{-1, -1},
	{-1, -2},
	{-1, 0},
	{-1, 2},
	{-1, 1},
	{-2, 1}
};

// The maneuver tables: the maneuvers a mech chooses among and those that may follow each.

// GLOBAL: MW2 0x100a2980
ManeuverEntry g_mechManeuvers[13] = {
	{{0, 6, 0, 1, 2, 3, 4, 4, 0}},
	{{1, 5, 0, 1, 2, 3, 4, 0, 0}},
	{{2, 3, 7, 8, 4, 0, 0, 0, 0}},
	{{3, 2, 7, 8, 0, 0, 0, 0, 0}},
	{{4, 3, 5, 5, 8, 0, 0, 0, 0}},
	{{5, 2, 7, 8, 0, 0, 0, 0, 0}},
	{{6, 1, 6, 0, 0, 0, 0, 0, 0}},
	{{7, 3, 0, 1, 8, 0, 0, 0, 0}},
	{{8, 5, 0, 1, 2, 3, 4, 0, 0}},
	{{9, 3, 0, 8, 7, 0, 0, 0, 0}},
	{{10, 3, 7, 7, 4, 0, 0, 0, 0}},
	{{11, 2, 3, 1, 0, 0, 0, 0, 0}},
	{{0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

// GLOBAL: MW2 0x100a2a70
ManeuverEntry g_stupidManeuvers = {{0, 1, 0, 0, 0, 0, 0, 0, 0}};

// GLOBAL: MW2 0x100a2a88
ManeuverEntry g_circleManeuvers = {{12, 1, 12, 0, 0, 0, 0, 0, 0}};

// GLOBAL: MW2 0x100a2aa0
ManeuverEntry g_behindManeuvers = {{1, 1, 1, 0, 0, 0, 0, 0, 0}};

// The table of a mech whose m_tons is 1 (ChooseManeuver).
// GLOBAL: MW2 0x100a2ab8
ManeuverEntry g_altMechManeuvers[13] = {
	{{0, 7, 0, 1, 2, 3, 4, 4, 4}},
	{{1, 6, 0, 1, 2, 3, 4, 4, 0}},
	{{2, 4, 7, 8, 4, 4, 0, 0, 0}},
	{{3, 4, 7, 8, 4, 4, 0, 0, 0}},
	{{4, 3, 5, 5, 8, 0, 0, 0, 0}},
	{{5, 2, 7, 8, 0, 0, 0, 0, 0}},
	{{6, 1, 6, 0, 0, 0, 0, 0, 0}},
	{{7, 5, 0, 1, 2, 8, 4, 0, 0}},
	{{8, 6, 0, 1, 2, 3, 4, 4, 0}},
	{{9, 3, 0, 8, 7, 0, 0, 0, 0}},
	{{10, 4, 7, 7, 4, 4, 0, 0, 0}},
	{{11, 1, 3, 0, 0, 0, 0, 0, 0}},
	{{0, 0, 0, 0, 0, 0, 0, 0, 0}},
};

// Set once InitializeManeuvers has filled g_maneuverTables.
// GLOBAL: MW2 0x100a2ba4
MechS32 g_maneuverTablesReady = 0;

// Set from the world stream's planet record when positive: the jump jets' drag divisor, and the
// ground slope (16.16) past which a mech on it slides.

// GLOBAL: MW2 0x100a2bdc
MechS32 g_jumpJetDrag = 100000;

// GLOBAL: MW2 0x100a2be0
MechS32 g_slideSlope = 0x2000;

// The maneuver table of each player type (m_type, 1 to 8), and in entry 8 the alternative to the
// first.
// GLOBAL: MW2 0x101748e0
ManeuverTable g_maneuverTables[9];

// Runs p_player's current maneuver (m_maneuver) against p_target for a tick, and ends it when it
// is done, when another is asked for (m_nextManeuver: out of ammunition, the mech jumps in or
// runs at the target, or flees) or when its time is up; with no maneuver, chooses and starts one.
// Stack-slot permutation: done, mech and leader.
// FUNCTION: MW2 0x10013430
void RunManeuver(Player* p_player, MechU16 p_target)
{
	MechS32 done;
	Mech* mech;
	Player* leader;

	done = FALSE;
	mech = p_player->m_mech;
	if (p_player->m_maneuver != -1) {
		switch (p_player->m_maneuver) {
		case c_maneuverStupid:
			BrakeFall(p_player);
			ManeuverStupid(p_player, p_target);
			break;
		case c_maneuverBehind:
			if (p_player->m_maneuverTimer <= g_currentClock) {
				ReleaseNavPoints(p_player);
				leader = g_players[p_player->m_ai.m_goal & 0xff];
				if (p_player->m_maneuverParam) {
					leader->m_placesTaken[p_player->m_maneuverParam / 2]--;
				}

				p_player->m_maneuverTimer = g_currentClock + 543;
				if (p_player->m_maneuverFlag == -1) {
					p_player->m_maneuverParam = ChooseFlankPlace(p_player);
					p_player->m_maneuverFlag = 0;
				}

				PlaceOffsetNav(p_player, p_player->m_ai.m_goal, p_player->m_maneuverParam, 15000);
				leader->m_placesTaken[p_player->m_maneuverParam / 2]++;
			}

			BrakeFall(p_player);
			ManeuverBehind(p_player, p_target);
			break;
		case c_maneuverAchick:
			BrakeFall(p_player);
			if (ManeuverAchick(p_player, p_target)) {
				done = TRUE;
			}
			break;
		case c_maneuverAsrp:
			BrakeFall(p_player);
			if (ManeuverAchick(p_player, p_target)) {
				done = TRUE;
			}

			if (!AvoidObstacles(p_player)) {
				p_player->m_steering->m_turn += p_player->m_maneuverParam * 0x1c20000;
			}

			if (p_player->m_maneuverTimer <= g_currentClock) {
				p_player->m_maneuverParam = -p_player->m_maneuverParam;
				p_player->m_maneuverTimer = g_currentClock + 543;
			}
			break;
		case c_maneuverAjmpin:
			if (ManeuverAjmpin(p_player, p_target)) {
				done = TRUE;
			}
			break;
		case c_maneuverSprint:
			if (ManeuverSprint(p_player, p_target)) {
				done = TRUE;
			}
			break;
		case c_maneuverAdfa:
			if (ManeuverAdfa(p_player, p_target) || p_player->m_mech->m_jumpFuel <= 0) {
				done = TRUE;
			}
			break;
		case c_maneuverKama:
			if (ManeuverKama(p_player, p_target)) {
				done = TRUE;
			}
			break;
		case c_maneuverWchick:
			BrakeFall(p_player);
			if (ManeuverWchick(p_player, p_target)) {
				done = TRUE;
			}
			break;
		case c_maneuverWbackp:
			BrakeFall(p_player);
			if (ManeuverWbackp(p_player, p_target)) {
				done = TRUE;
			}
			break;
		case c_maneuverWpeek:
			if (ManeuverWpeek(p_player, p_target)) {
				done = TRUE;
			}
			break;
		case c_maneuverAvoid:
			ManeuverAvoid(p_player, p_target);
			break;
		case c_maneuverCircle:
			BrakeFall(p_player);
			if (ManeuverCircle(p_player, p_target)) {
				done = TRUE;
			}
			break;
		}

		if (mech->m_topSpeed && IsOutOfAmmo(mech) && !p_player->m_controlsJets &&
			p_player->m_maneuver != c_maneuverKama && p_player->m_ai.m_state != c_aiStateFlee) {
			if (RandomIntBelow(4) && p_player->m_lastManeuver != c_maneuverAdfa && CanJump(p_player, 40)) {
				p_player->m_nextManeuver = c_maneuverAjmpin;
				p_player->m_maneuverFlag = 1;
			}
			else {
				if (RandomIntBelow(2)) {
					p_player->m_nextManeuver = c_maneuverKama;
				}
				else {
					p_player->m_nextManeuver = -2;
				}

				if (!p_player->m_skillFlag5) {
					p_player->m_nextManeuver = -2;
				}
			}
		}

		if (p_player->m_type == c_playerTypeMech && IsStuck(mech)) {
			done = TRUE;
		}

		if (p_player->m_maneuverEnd && p_player->m_maneuverEnd <= g_currentClock) {
			done = TRUE;
		}

		if (p_player->m_nextManeuver) {
			done = TRUE;
		}

		if (done) {
			EndManeuver(p_player);
		}
	}
	else {
		p_player->m_maneuver = ChooseManeuver(p_player);
		if (p_player->m_maneuver != -1) {
			StartManeuver(p_player);
		}
	}
}

// Resets p_player's maneuver state and sets the maneuvers its piloting (1 to 4) allows, and fills
// the maneuver tables the first time.
// FUNCTION: MW2 0x100139e9
void InitializeManeuvers(Player* p_player)
{
	MechS16 i;

	p_player->m_maneuver = -1;
	p_player->m_lastManeuver = -1;
	p_player->m_nextManeuver = 0;
	p_player->m_avoidShape = NULL;
	p_player->m_avoidSide = 0;
	p_player->m_probeScale = 1;
	p_player->m_nextAvoidCheck = 0;
	memset(p_player->m_placesTaken, 0, sizeof(p_player->m_placesTaken));
	if (p_player->m_piloting < 1 || p_player->m_piloting > 4) {
		p_player->m_piloting = 1;
	}

	if (p_player->m_piloting <= 3) {
		p_player->m_skillFlag0 = 1;
	}
	else {
		p_player->m_skillFlag0 = 0;
	}

	if (p_player->m_piloting <= 3) {
		p_player->m_skillFlag1 = 1;
	}
	else {
		p_player->m_skillFlag1 = 0;
	}

	if (p_player->m_piloting <= 2) {
		p_player->m_skillFlag2 = 1;
	}
	else {
		p_player->m_skillFlag2 = 0;
	}

	if (p_player->m_type == c_playerTypeMech && p_player->m_piloting <= 5) {
		p_player->m_skillFlag3 = 1;
	}
	else {
		p_player->m_skillFlag3 = 0;
	}

	if (p_player->m_piloting <= 4) {
		p_player->m_skillFlag4 = 1;
	}
	else {
		p_player->m_skillFlag4 = 0;
	}

	if (p_player->m_type == c_playerTypeMech && p_player->m_piloting <= 1) {
		p_player->m_skillFlag5 = 1;
	}
	else {
		p_player->m_skillFlag5 = 0;
	}

	if (p_player->m_piloting <= 5) {
		p_player->m_skillFlag6 = 1;
	}
	else {
		p_player->m_skillFlag6 = 0;
	}

	if (!g_maneuverTablesReady) {
		for (i = 0; i < 8; i++) {
			switch (i + 1) {
			case 1:
				g_maneuverTables[i].m_count = 13;
				g_maneuverTables[i].m_followOnly = 7;
				g_maneuverTables[i].m_entries = g_mechManeuvers;
				g_maneuverTables[8] = g_maneuverTables[i];
				g_maneuverTables[8].m_entries = g_altMechManeuvers;
				break;
			case 8:
				g_maneuverTables[i].m_count = 1;
				g_maneuverTables[i].m_followOnly = 0;
				g_maneuverTables[i].m_entries = &g_behindManeuvers;
				break;
			case 5:
				g_maneuverTables[i].m_count = 1;
				g_maneuverTables[i].m_followOnly = 0;
				g_maneuverTables[i].m_entries = &g_circleManeuvers;
				break;
			default:
				g_maneuverTables[i].m_count = 1;
				g_maneuverTables[i].m_followOnly = 0;
				g_maneuverTables[i].m_entries = &g_stupidManeuvers;
				break;
			}
		}

		g_maneuverTablesReady = 1;
	}
}

// Picks p_player's next maneuver: one an order asked for, an AI player's escape, attack or chase
// when it applies, or else one its maneuver table lets follow the previous one (any one at
// first), redrawn until its conditions hold.
// Stack-slot permutation: mech, table, choice and index.
// FUNCTION: MW2 0x10013d81
MechS32 ChooseManeuver(Player* p_player)
{
	Mech* mech;
	ManeuverTable* table;
	MechS16 choice;
	MechS16 index;

	choice = -1;
	table = &g_maneuverTables[p_player->m_type - 1];
	mech = p_player->m_mech;
	if (mech->m_tons == 1) {
		table = &g_maneuverTables[8];
	}

	do {
		SetTarget(p_player, p_player->m_ai.m_target);
		if (p_player->m_nextManeuver) {
			if (p_player->m_nextManeuver == -2) {
				SetAIState(p_player, 4, p_player->m_ai.m_goal, 0);
				p_player->m_ai.m_flags = 1;
			}
			else {
				choice = p_player->m_nextManeuver;
			}

			p_player->m_nextManeuver = 0;
			break;
		}

		if (p_player->m_type == c_playerTypeMech) {
			if (IsStuck(mech)) {
				choice = c_maneuverAvoid;
				break;
			}

			if (!(p_player->m_ai.m_goal & 0x200)) {
				choice = c_maneuverStupid;
				break;
			}

			if (CanJump(p_player, 0x14) && p_player->m_skillFlag1 && IsBelowHiddenTarget(p_player) &&
				HasLineToTarget(p_player, p_player->m_targetInfo.m_position.m_y)) {
				if (!p_player->m_lastManeuver) {
					choice = c_maneuverWpeek;
				}
				else {
					choice = c_maneuverStupid;
				}

				if (!choice && RandomIntBelow(2)) {
					choice = c_maneuverWbackp;
				}
				break;
			}
		}

		if (p_player->m_lastManeuver == -1) {
			index = RandomIntBelow(table->m_count - table->m_followOnly);
			choice = table->m_entries[index].m_list[0];
		}
		else {
			index = FindManeuver(table, p_player->m_lastManeuver);
			if (index == -1) {
				index = 0;
			}

			choice = table->m_entries[index].m_list[2 + RandomIntBelow(table->m_entries[index].m_list[1])];
		}

		if (p_player->m_lastManeuver == c_maneuverAjmpin && p_player->m_maneuverFlag == 1) {
			choice = c_maneuverAdfa;
			break;
		}

		if (choice == c_maneuverAjmpin && (!CanJump(p_player, 0x14) || !p_player->m_skillFlag1)) {
			choice = -1;
		}

		if (choice == c_maneuverAjmpin && GetTargetBearing(p_player) > mech->m_maxTorsoTwist) {
			choice = c_maneuverStupid;
		}

		if (choice == c_maneuverAsrp && mech->m_maxTorsoTwist < 0xa0000) {
			choice = c_maneuverAchick;
		}

		if (choice == c_maneuverAdfa && p_player->m_onGround) {
			choice = c_maneuverStupid;
		}
	} while (choice == -1);

	return choice;
}

// Returns the index of maneuver p_id in p_table, or -1.
// The original loads the index before m_entries (index order).
// FUNCTION: MW2 0x100140e4
MechS16 FindManeuver(ManeuverTable* p_table, MechS16 p_id)
{
	MechS16 i;

	for (i = 0; i < p_table->m_count; i++) {
		if (p_table->m_entries[i].m_list[0] == p_id) {
			return i;
		}
	}

	return -1;
}

// Starts p_player's maneuver (m_maneuver): resets its state and sets it up, with the time it
// ends (m_maneuverEnd).
// FUNCTION: MW2 0x10014149
void StartManeuver(Player* p_player)
{
	p_player->m_ai.m_goal = p_player->m_ai.m_target;
	p_player->m_maneuverTimer = 0;
	p_player->m_lastTargetDistance = p_player->m_maneuverParam = p_player->m_maneuverFlag = 0;
	p_player->m_controlsJets = 0;
	p_player->m_maneuverEnd = g_currentClock + 0x235a;
	switch (p_player->m_maneuver) {
	case c_maneuverStupid:
		p_player->m_maneuverEnd = (RandomIntBelow(5) + 8) * 181 + g_currentClock;
		break;
	case c_maneuverBehind:
		p_player->m_maneuverEnd = g_currentClock + 0xe24;
		p_player->m_maneuverFlag = -1;
		break;
	case c_maneuverAsrp:
		if (RandomIntBelow(2)) {
			p_player->m_maneuverParam = 1;
		}
		else {
			p_player->m_maneuverParam = -1;
		}
		break;
	case c_maneuverAjmpin:
		p_player->m_steering->m_throttle = 0;
		p_player->m_steering->m_turn = 0;
		p_player->m_controlsJets = 1;
		SetJumpJets(p_player, 1);
		p_player->m_maneuverEnd = g_currentClock + 0x389;
		break;
	case c_maneuverSprint:
		p_player->m_steering->m_turn = 0;
		p_player->m_controlsJets = 1;
		if (!p_player->m_maneuverParam || p_player->m_maneuverParam == 2) {
			p_player->m_steering->m_jumpJetFireLeft = 1;
		}
		else {
			p_player->m_steering->m_jumpJetFireForward = 1;
		}

		SetJumpJets(p_player, 1);
		p_player->m_maneuverEnd = g_currentClock + 0x16a;
		break;
	case c_maneuverAdfa:
		GetClosingRate(p_player);
		p_player->m_controlsJets = 1;
		break;
	case c_maneuverWbackp:
		p_player->m_maneuverEnd = (RandomIntBelow(11) + 10) * 181 + g_currentClock;
		p_player->m_steering->m_reverse = 1;
		break;
	case c_maneuverWchick:
		if (p_player->m_targetInfo.m_distance < 4000) {
			PlaceOffsetNav(p_player, p_player->m_index | 0x200, RandomIntBelow(2) ? 4 : 12, 10000);
		}
		else {
			PlaceOffsetNav(p_player, p_player->m_index | 0x200, RandomIntBelow(2) ? 3 : 15, 10000);
		}
		break;
	case c_maneuverWpeek:
		p_player->m_maneuverParam = 0;
		p_player->m_steering->m_turn = 0;
		p_player->m_controlsJets = 1;
		break;
	case c_maneuverAvoid:
		if (!p_player->m_mech->m_collisionTicks || p_player->m_lastManeuver != c_maneuverAvoid ||
			p_player->m_collidedWith != -1) {
			p_player->m_steering->m_reverse = 1;
			p_player->m_maneuverEnd = g_currentClock + 0x5a8;
		}
		else {
			p_player->m_maneuverEnd = g_currentClock + 0x2d4;
		}
		break;
	case c_maneuverCircle:
		p_player->m_maneuverEnd = g_currentClock + 0x46b4;
		PlacePatrolNavs(p_player);
		break;
	default:
		break;
	}
}

// Ends p_player's maneuver (m_maneuver): undoes what it set up, records it as the previous one
// and makes the goal the target again.
// FUNCTION: MW2 0x1001450e
void EndManeuver(Player* p_player)
{
	switch (p_player->m_maneuver) {
	case c_maneuverBehind:
		ReleaseNavPoints(p_player);
		if (p_player->m_ai.m_goal & 0x200) {
			g_players[p_player->m_ai.m_goal & 0xff]->m_placesTaken[p_player->m_maneuverParam / 2]--;
		}
		break;
	case c_maneuverAjmpin:
		SetJumpJets(p_player, 0);
		break;
	case c_maneuverSprint:
		p_player->m_steering->m_jumpJetFireForward = 0;
		p_player->m_steering->m_jumpJetFireBackward = 0;
		p_player->m_steering->m_jumpJetFireLeft = 0;
		p_player->m_steering->m_jumpJetFireRight = 0;
		SetJumpJets(p_player, 0);
		break;
	case c_maneuverAdfa:
		SetJumpJets(p_player, 0);
		p_player->m_steering->m_jumpJetFireForward = 0;
		p_player->m_steering->m_jumpJetFireBackward = 0;
		break;
	case c_maneuverWbackp:
	case c_maneuverAvoid:
		p_player->m_steering->m_reverse = 0;
		break;
	case c_maneuverWchick:
		ReleaseNavPoints(p_player);
		break;
	case c_maneuverWpeek:
		SetJumpJets(p_player, 0);
		break;
	case c_maneuverCircle:
		ReleaseNavPoints(p_player);
		break;
	case c_maneuverAchick:
	case c_maneuverAsrp:
	case c_maneuverKama:
		break;
	}

	SetJumpJets(p_player, 0);
	p_player->m_lastManeuver = p_player->m_maneuver;
	p_player->m_maneuver = -1;
	p_player->m_maneuverEnd = p_player->m_maneuverTimer = 0;
	p_player->m_ai.m_target = p_player->m_ai.m_goal;
	p_player->m_maneuverParam = p_player->m_maneuverFlag = 0;
	SetTarget(p_player, p_player->m_ai.m_goal);
}

// Places a nav point for p_player where GetOffsetPoint puts it, and makes it the player's target.
// The only diff is a stack-slot permutation of x, y, z and nav.
// FUNCTION: MW2 0x10014723
void PlaceOffsetNav(Player* p_player, MechU32 p_target, MechS16 p_direction, MechS16 p_distance)
{
	MechS32 z;
	MechS32 y;
	MechS32 x;
	MechS32 nav;

	GetOffsetPoint(p_target, p_direction, &x, &z, &y, p_distance);
	nav = AddNavPoint(p_player->m_index, x, y, z);
	if (nav != -1) {
		g_navTable[nav].m_flags |= 1;
		g_navTable[nav].m_owner = p_player->m_index | 0x200;
		AdvanceNavTarget(p_player, 0x100);
	}
}

// Finds the point p_distance units out in direction p_direction (of g_probeDirections's 16) from
// target p_target, a player (0x200) or a game thing (0x400), in world coordinates; for a player
// *p_y takes its heading. A game thing without an object is offset from its position.
// The empty else arms give the original's jmp to the next statement after each offset. The only
// other diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x100147d0
void GetOffsetPoint(MechU32 p_target, MechS16 p_direction, MechS32* p_x, MechS32* p_z, MechS32* p_y, MechS16 p_distance)
{
	MechU32 index;
	MechS32 thing;
	Matrix* matrix;
	struct SceneObject* obj;
	MechS32 y;

	y = 0;
	index = p_target & 0xff;
	switch (p_target & 0xf00) {
	case 0x200:
		*p_y = g_players[index]->m_heading;
		obj = g_players[index]->m_obj;
		break;
	case 0x400:
		thing = g_gameThings[index].m_staticObject;
		obj = GetStaticSceneObject(thing);
		if (!obj) {
			GetStaticObjectPosition(thing, p_x, p_y, p_z);
			*p_y = 0;
			if (g_probeDirections[p_direction].m_x) {
				*p_x += p_distance / g_probeDirections[p_direction].m_x;
			}
			else {
			}

			if (g_probeDirections[p_direction].m_y) {
				*p_z += p_distance / g_probeDirections[p_direction].m_y;
			}
			else {
			}

			return;
		}
		break;
	default:
		break;
	}

	if (g_probeDirections[p_direction].m_x) {
		*p_x = p_distance / g_probeDirections[p_direction].m_x;
	}
	else {
		*p_x = 0;
	}

	if (g_probeDirections[p_direction].m_y) {
		*p_z = p_distance / g_probeDirections[p_direction].m_y;
	}
	else {
		*p_z = 0;
	}

	matrix = GetObjWorldMatrix(obj);
	TransformPoint(matrix, p_x, &y, p_z);
}

// Whether p_turn (16.16 degrees) is a sharp turn, past 5 degrees either way; a stopped player
// then creeps forward.
// FUNCTION: MW2 0x1001498c
MechS32 IsSharpTurn(Player* p_player, MechS32 p_turn)
{
	MechS32 sharp;

	sharp = 0;
	if (p_turn > 0x50000 || p_turn < -0x50000) {
		sharp = 1;
		if (!p_player->m_steering->m_throttle) {
			p_player->m_steering->m_throttle = 0x66;
		}
	}

	return sharp;
}

// Steers p_player at p_target and closes to 15000, asking for c_maneuverAvoid within 4500.
// FUNCTION: MW2 0x100149e7
void ManeuverStupid(Player* p_player, MechS16 p_target)
{
	MechS32 range;
	MechS32 heading;

	SetTarget(p_player, p_target);
	if (!AvoidObstacles(p_player)) {
		range = 15000;
		p_player->m_steering->m_throttle = GetApproachThrottle(p_player, range);
		heading = SteerToTarget(p_player);
		IsSharpTurn(p_player, heading);
	}
	else {
		heading = GetTargetBearing(p_player);
	}

	if (p_player->m_targetInfo.m_distance < 4500) {
		p_player->m_nextManeuver = c_maneuverAvoid;
	}

	RunAIWeapons(p_player, heading);
	JumpToTurn(p_player);
}

// Turns p_player toward its goal and closes on its target; once stopped and turned more than 5
// degrees away, turns in place (IsSharpTurn) until the target is more than 4500 away.
// FUNCTION: MW2 0x10014aa8
void ManeuverBehind(Player* p_player, MechS16 p_target)
{
	MechS32 heading;

	SetTarget(p_player, p_player->m_ai.m_goal);
	heading = GetTargetBearing(p_player);
	RunAIWeapons(p_player, heading);
	if (!p_player->m_maneuverFlag) {
		SetTarget(p_player, p_player->m_ai.m_target);
		if (!AvoidObstacles(p_player)) {
			SteerToTarget(p_player);
			p_player->m_steering->m_throttle = GetApproachThrottle(p_player, 4000);
		}

		if (!p_player->m_steering->m_throttle && abs(heading) > 0x50000) {
			p_player->m_maneuverFlag = 1;
		}

		JumpToTurn(p_player);
	}

	if (p_player->m_maneuverFlag == 1) {
		if (AvoidObstacles(p_player)) {
			heading = GetTargetBearing(p_player);
		}
		else {
			heading = SteerToTarget(p_player);
		}

		if (!IsSharpTurn(p_player, heading)) {
			p_player->m_steering->m_throttle = 0;
		}
		else {
			JumpToTurn(p_player);
		}

		SetTarget(p_player, p_player->m_ai.m_target);
		if (p_player->m_targetInfo.m_distance > 4500) {
			p_player->m_maneuverFlag = 0;
		}
	}
}

// Closes on p_target. Whether it is within 8000, or more when closing fast (GetClosingRate, per
// 500000).
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10014c3d
MechS32 ManeuverAchick(Player* p_player, MechS16 p_target)
{
	MechDouble scale;
	MechS32 heading;
	MechS32 result;

	result = FALSE;
	SetTarget(p_player, p_target);
	if (!AvoidObstacles(p_player)) {
		heading = SteerToTarget(p_player);
		p_player->m_steering->m_throttle = GetApproachThrottle(p_player, 5500);
	}
	else {
		heading = GetTargetBearing(p_player);
	}

	RunAIWeapons(p_player, heading);
	scale = FixedDiv16(GetClosingRate(p_player) << 16, 500000) / 65536.0;
	if (scale < 1.0) {
		scale = 1.0;
	}

	if (p_player->m_targetInfo.m_distance <= scale * 8000.0) {
		result = TRUE;
	}

	JumpToTurn(p_player);
	return result;
}

// Runs at p_target, even when the shape AvoidObstacles steers around is the target's own, and
// self-destructs within 2000 of it. Whether it did.
// FUNCTION: MW2 0x10014d4e
MechS32 ManeuverKama(Player* p_player, MechS16 p_target)
{
	MechS32 result;

	result = FALSE;
	SetTarget(p_player, p_target);
	if (!AvoidObstacles(p_player) || GetTargetShape(p_target) == p_player->m_avoidShape) {
		SteerToTarget(p_player);
		p_player->m_steering->m_throttle = GetApproachThrottle(p_player, 0);
	}

	if (p_player->m_targetInfo.m_range <= 2000) {
		p_player->m_steering->m_selfDestruct = 1;
		result = TRUE;
	}

	return result;
}

// Backs p_player away from p_target at full throttle, facing it. Whether its mech collides.
// The only diff is a stack-slot permutation of heading and result.
// FUNCTION: MW2 0x10014df1
MechS32 ManeuverWbackp(Player* p_player, MechS16 p_target)
{
	MechS32 heading;
	MechS32 result;

	SetTarget(p_player, p_target);
	heading = SteerToTarget(p_player);
	RunAIWeapons(p_player, heading);
	result = p_player->m_mech->m_collisionTicks;
	p_player->m_steering->m_throttle = 0x400;
	JumpToTurn(p_player);
	return result;
}

// Turns p_player toward its goal and closes on its target. Whether it is within 3000.
// FUNCTION: MW2 0x10014e5e
MechS32 ManeuverWchick(Player* p_player, MechS16 p_target)
{
	MechS32 heading;

	SetTarget(p_player, p_player->m_ai.m_goal);
	heading = GetTargetBearing(p_player);
	RunAIWeapons(p_player, heading);
	SetTarget(p_player, p_player->m_ai.m_target);
	if (!AvoidObstacles(p_player)) {
		p_player->m_steering->m_throttle = GetApproachThrottle(p_player, 3000);
		SteerToTarget(p_player);
	}

	JumpToTurn(p_player);
	return p_player->m_targetInfo.m_distance <= 3000;
}

// Turns p_player toward p_target and runs an attack in three steps (m_maneuverParam): steer at it
// while it can fire and has a line to it, then for a second, then BrakeFall. Whether that ended.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10014f23
MechS32 ManeuverWpeek(Player* p_player, MechS16 p_target)
{
	MechS32 heading;
	MechS32 result;

	result = FALSE;
	SetTarget(p_player, p_target);
	heading = GetTargetBearing(p_player);
	RunAIWeapons(p_player, heading);
	switch (p_player->m_maneuverParam) {
	case 0:
		if (CanJump(p_player, 40) && IsBelowHiddenTarget(p_player)) {
			SetJumpJets(p_player, 1);
		}
		else {
			SetJumpJets(p_player, 0);
			p_player->m_maneuverParam = 1;
			if (CanJump(p_player, 20) && !RandomIntBelow(4)) {
				p_player->m_nextManeuver = c_maneuverAdfa;
			}

			p_player->m_maneuverTimer = g_currentClock + 181;
		}
		break;
	case 1:
		if (p_player->m_maneuverTimer < g_currentClock) {
			p_player->m_maneuverParam = 2;
			SetJumpJets(p_player, 0);
		}
		else {
			SetJumpJets(p_player, 1);
		}

		SteerToTarget(p_player);
		break;
	case 2:
		if (BrakeFall(p_player)) {
			result = TRUE;
		}
		break;
	}

	return result;
}

// Turns p_player toward p_target. Whether the player is at least 2000 above it.
// FUNCTION: MW2 0x100150c1
MechS32 ManeuverAjmpin(Player* p_player, MechS16 p_target)
{
	MechS32 heading;

	SetTarget(p_player, p_target);
	heading = GetTargetBearing(p_player);
	RunAIWeapons(p_player, heading);
	if (p_player->m_position.m_y >= p_player->m_targetInfo.m_position.m_y + 2000) {
		return 1;
	}
	else {
		return 0;
	}
}

// Turns p_player toward p_target.
// FUNCTION: MW2 0x1001512e
MechS32 ManeuverSprint(Player* p_player, MechS16 p_target)
{
	MechS32 heading;

	SetTarget(p_player, p_target);
	heading = GetTargetBearing(p_player);
	RunAIWeapons(p_player, heading);
	return 0;
}

// Turns p_player toward p_target and jumps at it while more than 1000 away, firing the jets
// forward or backward by how fast it closes; closer in, it cuts them, counting the ticks it was
// still jumping (m_maneuverParam; it stops at 2). Whether it is done: on the ground, colliding, or unable to
// jump on.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10015172
MechS32 ManeuverAdfa(Player* p_player, MechS16 p_target)
{
	MechS32 rate;
	MechS32 result;
	MechS32 turn;
	MechS16 side;
	Mech* mech;
	Mech* targetMech;

	SetTarget(p_player, p_target);
	turn = GetTargetBearing(p_player);
	RunAIWeapons(p_player, turn);
	if (p_player->m_onGround || p_player->m_mech->m_collisionTicks) {
		result = TRUE;
	}
	else {
		result = FALSE;
	}

	if (p_player->m_maneuverParam == 2) {
		return result;
	}

	turn = abs(turn);
	targetMech = g_players[p_target & 0xff]->m_mech;
	mech = p_player->m_mech;
	if (turn > mech->m_maxTorsoTwist && turn < mech->m_maxTorsoTwist * 3) {
		side = 1;
	}
	else {
		side = 0;
	}

	if (p_player->m_targetInfo.m_distance > 1000) {
		rate = GetClosingRate(p_player);
		if (rate > (p_player->m_targetInfo.m_distance <= 6000 ? 9 : 36)) {
			SetJumpDirection(p_player, side);
		}
		else {
			SetJumpDirection(p_player, !side);
		}

		SetJumpJets(p_player, 1);
		if (!CanJump(p_player, 40)) {
			result = TRUE;
		}
	}
	else {
		if (p_player->m_steering->m_jumpJetEnabled) {
			p_player->m_maneuverParam++;
		}

		SetJumpJets(p_player, 0);
		p_player->m_steering->m_jumpJetFireForward = 0;
		p_player->m_steering->m_jumpJetFireBackward = 0;
	}

	return result;
}

// Drives p_player at full throttle (backwards while StartManeuver set m_reverse), steering
// towards p_target.
// FUNCTION: MW2 0x10015342
void ManeuverAvoid(Player* p_player, MechS16 p_target)
{
	MechS32 heading;

	SetTarget(p_player, p_target);
	if (!p_player->m_steering->m_reverse) {
		if (!AvoidObstacles(p_player)) {
			heading = SteerToTarget(p_player);
		}
		else {
			heading = GetTargetBearing(p_player);
		}
	}
	else {
		heading = SteerToTarget(p_player);
	}

	RunAIWeapons(p_player, heading);
	p_player->m_steering->m_throttle = 0x400;
	JumpToTurn(p_player);
}

// Turns p_player toward its goal and closes on its target, giving up the goal (ReleaseNavPoints,
// PlacePatrolNavs) now and then while far from the target, and handing the target to AdvanceNavTarget
// within 3000.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x100153e6
MechS32 ManeuverCircle(Player* p_player, MechS16 p_target)
{
	MechS32 heading;
	MechS32 result;

	result = 0;
	SetTarget(p_player, p_player->m_ai.m_goal);
	if (p_player->m_maneuverTimer < g_currentClock) {
		if (p_player->m_targetInfo.m_distance > 15000.0) {
			ReleaseNavPoints(p_player);
			PlacePatrolNavs(p_player);
		}

		p_player->m_maneuverTimer = g_currentClock + 0x5a8;
	}

	heading = GetTargetBearing(p_player);
	RunAIWeapons(p_player, heading);
	SetTarget(p_player, p_player->m_ai.m_target);
	if (!AvoidObstacles(p_player)) {
		p_player->m_steering->m_throttle = GetApproachThrottle(p_player, 3000);
		SteerToTarget(p_player);
	}

	JumpToTurn(p_player);
	if (p_player->m_targetInfo.m_distance <= 3000) {
		AdvanceNavTarget(p_player, p_player->m_ai.m_target);
	}

	return result;
}

// Whether p_player, targeting a player it has no clear line to, is more than 800 below it, with
// jump jets (its jump fuel not negative).
// The only diff is a stack-slot permutation of index and below.
// FUNCTION: MW2 0x10015520
MechS32 IsBelowHiddenTarget(Player* p_player)
{
	MechS16 index;
	MechS32 below;

	below = 0;
	index = p_player->m_targetInfo.m_target & 0xff;
	switch (p_player->m_targetInfo.m_target & 0xf00) {
	default:
		break;
	case 0x200:
		if (!HasLineToTarget(p_player, p_player->m_position.m_y)) {
			if (p_player->m_targetInfo.m_position.m_y - 800 > p_player->m_position.m_y &&
				p_player->m_mech->m_jumpFuel >= 0) {
				below = 1;
			}
			else {
				below = 0;
			}
		}
		break;
	}

	return below;
}

// Near the ground, fires the player's jump jets while its mech falls faster than 85% of the fall
// damage speed and clears it once it is slower than 75%, and logs a fall faster than that speed.
// Returns whether the player is on the ground.
// Stack-slot permutation: line, value and mech.
// FUNCTION: MW2 0x100155e1
MechS32 BrakeFall(Player* p_player)
{
	MechChar line[80];
	MechS16 value;
	Mech* mech;

	mech = p_player->m_mech;
	if (mech->m_jumpFuel < 0 || !mech->m_jumpThrust) {
		return p_player->m_onGround;
	}

	value = p_player->m_steering->m_jumpJetEnabled;
	if (p_player->m_position.m_y < 20000) {
		if (mech->m_velocityY < -0x102762 * 0.85) {
			value = 1;
		}
		else if (mech->m_velocityY > -0x102762 * 0.75) {
			value = 0;
		}
	}

	SetJumpJets(p_player, value);
	if (mech->m_velocityY < -0x102762) {
		sprintf(
			line,
			"%6d : %2d Mech %2d has exceded fall damage speed.\n",
			g_currentClock,
			p_player->m_team,
			p_player->m_index
		);
		WriteToMw2Log(line);
	}

	return p_player->m_onGround;
}

// Fires (p_value set) or cuts p_player's jump jets.
// FUNCTION: MW2 0x100156f2
void SetJumpJets(Player* p_player, MechS8 p_value)
{
	p_player->m_steering->m_jumpJetEnabled = p_value;
}

// Steers p_player's mech around what lies ahead: casts up to four probe rays (BuildProbeRay) on
// the avoiding side m_avoidSide, as long as the mech's speed (m_probeScale), and turns away from what
// they hit, slowing down if the first one hits. Returns whether it steered. Between checks (every
// 10 ticks for the local player, 90 or 181 for others) it returns whether it is avoiding a shape.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10015709
MechS32 AvoidObstacles(Player* p_player)
{
	MechS32 length;
	Shape* hit;
	Ray ray;
	Mech* mech;
	MechS16 i;
	MechS32 turn;
	MechS32 throttle;

	turn = 0;
	mech = p_player->m_mech;
	if (!mech->m_topSpeed || (!p_player->m_avoidSide && !p_player->m_steering->m_throttle) ||
		p_player->m_steering->m_reverse) {
		return FALSE;
	}

	if (p_player->m_nextAvoidCheck > g_currentClock) {
		return p_player->m_avoidShape ? TRUE : FALSE;
	}

	if (!p_player->m_avoidSide) {
		length = ApproximateVectorLength(mech->m_velocityX, 0, mech->m_velocityZ);
		p_player->m_probeScale = FixedDiv16(length, 0x7a120);
		if (p_player->m_type != c_playerTypeMech) {
			p_player->m_probeScale >>= 1;
		}

		if (p_player->m_probeScale < 0.3 * 0x10000) {
			p_player->m_probeScale = 0x4ccc;
		}
	}

	for (i = 0; i < 4; i++) {
		BuildProbeRay(
			p_player,
			&ray,
			p_player->m_avoidSide,
			i,
			FixedMul16(p_player->m_probeScale, 0x13880000) >> 16,
			0
		);
		if (TestSegmentCollision(&ray, &hit, p_player->m_index)) {
			if (IsStandableShape(hit)) {
				break;
			}

			if (!p_player->m_avoidSide) {
				p_player->m_avoidSide = GetAvoidSide(
					p_player,
					hit,
					p_player->m_position.m_x,
					p_player->m_position.m_y,
					p_player->m_position.m_z
				);
				p_player->m_avoidShape = hit;
			}

			turn += (MechS32) (p_player->m_avoidSide * (0.2 * 0x10000000) / 4);
		}
		else {
			if (i == 0 && abs(GetTargetBearing(p_player)) <= 0x10000) {
				p_player->m_avoidShape = NULL;
				p_player->m_avoidSide = 0;
			}

			if (i == 0 && p_player->m_avoidSide) {
				BuildProbeRay(
					p_player,
					&ray,
					-p_player->m_avoidSide,
					1,
					FixedMul16(p_player->m_probeScale, 0x13880000) >> 16,
					1
				);
				if (TestSegmentCollision(&ray, &hit, p_player->m_index)) {
					if (IsStandableShape(hit)) {
						break;
					}

					turn += (MechS32) (p_player->m_avoidSide * (0.1 * 0x10000000));
				}
			}

			break;
		}
	}

	if (turn) {
		turn = ClampMagnitude(turn, 0x3333333);
		p_player->m_steering->m_turn = turn;
		if (p_player->m_index == g_localPlayerId) {
			throttle = p_player->m_maneuverParam;
		}
		else {
			throttle = 0x400;
		}

		if (i == 0) {
			p_player->m_steering->m_throttle = throttle;
		}
		else {
			p_player->m_steering->m_throttle = 0x100;
		}

		if (p_player->m_type == c_playerTypeWanderer) {
			p_player->m_steering->m_throttle = 0;
			p_player->m_steering->m_turn = 0;
		}
	}

	if (p_player->m_index == g_localPlayerId) {
		p_player->m_nextAvoidCheck = g_currentClock + 10;
	}
	else {
		p_player->m_nextAvoidCheck = g_currentClock + (p_player->m_avoidShape ? 90 : 181);
	}

	return turn ? TRUE : FALSE;
}

// Whether p_shape is solid ground to stand on: a flat enough face or a shape of type 0x50.
// FUNCTION: MW2 0x10015b40
MechS32 IsStandableShape(Shape* p_shape)
{
	if (HasHeightTest(p_shape) && g_segmentNormalY >= 0xc41b) {
		return 1;
	}

	return (p_shape->m_kind & 0xf0) == 0x50;
}

// Builds a probe ray for p_player in p_ray: p_length along direction p_step (mirrored for a
// negative p_side) in the mech's frame, from its position or, with p_fromEdge, from its side.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10015b9f
void BuildProbeRay(Player* p_player, Ray* p_ray, MechS32 p_side, MechS16 p_step, MechS32 p_length, MechS32 p_fromEdge)
{
	Matrix* matrix;
	MechS32 x;
	MechS32 y;
	MechS32 dx;
	MechS32 z;
	MechS32 dz;
	MechS32 dy;
	Mech* mech;

	dy = 0;
	mech = p_player->m_mech;
	matrix = GetObjWorldMatrix(p_player->m_obj);
	dx = g_probeDirections[p_side >= 0 ? p_step : (0x10 - p_step) % 16].m_x;
	dz = g_probeDirections[p_side >= 0 ? p_step : (0x10 - p_step) % 16].m_y;
	if (dx) {
		dx = p_length / dx;
	}

	if (dz) {
		dz = p_length / dz;
	}

	TransformPoint(matrix, &dx, &dy, &dz);
	if (p_fromEdge) {
		if (p_side >= 0) {
			x = mech->m_radius - 1;
		}
		else {
			x = -mech->m_radius + 1;
		}

		z = 0;
		TransformPoint(matrix, &x, &dy, &z);
		y = p_player->m_position.m_y;
	}
	else {
		y = p_player->m_position.m_y;
		x = p_player->m_position.m_x;
		z = p_player->m_position.m_z;
	}

	BuildRayFromSegment(p_ray, x, y, z, dx, y, dz);
}

// Which side of p_player the point (p_x, p_y, p_z) is, seen from p_shape: 1 or -1.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10015d2a
MechS16 GetAvoidSide(Player* p_player, Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	MechS32 dz;
	MechS32 unused;
	MechU32 distance;
	MechS32 heading;
	MechS32 length;
	MechS32 dx;
	MechS32 dy;

	dx = p_shape->m_centerX - p_x;
	dy = p_shape->m_centerY - p_y;
	dz = p_shape->m_centerZ - p_z;
	GetBearingAndRange(dx, dy, dz, &heading, &length, &distance, &unused);
	heading -= p_player->m_heading;
	if (heading > 0xb40000) {
		heading -= 0x1680000;
	}
	else if (heading < -0xb40000) {
		heading += 0x1680000;
	}

	return heading >= 0 ? -1 : 1;
}

// The heading from p_player to p_target (16.16 degrees, 0 to 360), keeping its target.
// FUNCTION: MW2 0x10015dd6
MechS32 GetHeadingTo(Player* p_player, MechS16 p_target)
{
	MechS32 target;
	MechS32 heading;

	target = p_player->m_targetInfo.m_target;
	SetTarget(p_player, p_target);
	heading = (GetTargetBearing(p_player) + 0x1680000) % 0x1680000;
	SetTarget(p_player, target);
	return heading;
}

// Clamps p_value to +/- p_limit.
// FUNCTION: MW2 0x10015e34
MechS32 ClampMagnitude(MechS32 p_value, MechS32 p_limit)
{
	if (p_value > p_limit) {
		p_value = p_limit;
	}
	else if (p_value < -p_limit) {
		p_value = -p_limit;
	}

	return p_value;
}

// Whether the segment from p_player at height p_y to its target is clear, or hits the target.
// The only diff is a stack-slot permutation of ray, hit, target and flags.
// FUNCTION: MW2 0x10015e74
MechS32 HasLineToTarget(Player* p_player, MechS32 p_y)
{
	Ray ray;
	Shape* hit;
	MechU32 target;
	MechU16 flags;

	hit = NULL;
	BuildRayFromSegment(
		&ray,
		p_player->m_position.m_x,
		p_y,
		p_player->m_position.m_z,
		p_player->m_targetInfo.m_position.m_x,
		p_player->m_targetInfo.m_position.m_y,
		p_player->m_targetInfo.m_position.m_z
	);
	BuildRayFixed(&ray);
	if (TestSegmentCollision(&ray, &hit, p_player->m_index) && hit) {
		flags = hit->m_kind;
		target = p_player->m_targetInfo.m_target;
		if ((flags & 0x100) && (target & 0x200)) {
			return hit->m_owner == (target & 0xff);
		}
		else if ((flags & 0x200) && (target & 0x400)) {
			return hit->m_owner == (target & 0xff);
		}
		else {
			return 0;
		}
	}

	return 1;
}

// Whether p_mech has weapons but none left that can fire: every one is out of ammunition.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10015fa8
MechS32 IsOutOfAmmo(Mech* p_mech)
{
	MechS16 i;
	WeaponSlot* slot;
	MechS32 found;

	found = FALSE;
	for (i = 0; i < p_mech->m_weaponCount && !found; i++) {
		slot = &p_mech->m_weapons[i];
		if (slot->m_state != c_weaponEmpty && slot->m_ammo) {
			found = TRUE;
			break;
		}
	}

	return !found && p_mech->m_weaponCount;
}

// Fires p_player's jump jets forward (p_value set) or backward.
// FUNCTION: MW2 0x10016057
void SetJumpDirection(Player* p_player, MechS16 p_value)
{
	p_player->m_steering->m_jumpJetFireForward = p_value;
	if (!p_value) {
		p_player->m_steering->m_jumpJetFireBackward = 1;
	}
	else {
		p_player->m_steering->m_jumpJetFireBackward = 0;
	}
}

// How fast p_player closes on its target since the last call, per tick.
// FUNCTION: MW2 0x10016093
MechS32 GetClosingRate(Player* p_player)
{
	MechS32 rate;

	if (!g_deltaTime) {
		return 0;
	}

	rate = (p_player->m_lastTargetDistance - p_player->m_targetInfo.m_distance) / g_deltaTime;
	p_player->m_lastTargetDistance = p_player->m_targetInfo.m_distance;
	return rate;
}

// Fires p_player's jump jets to turn faster in the air while its target is outside the torso's
// twist, and stops them once it is inside, unless the maneuver works them itself. Only with the
// piloting skill for it, and not while avoiding.
// FUNCTION: MW2 0x100160eb
void JumpToTurn(Player* p_player)
{
	MechS32 turn;
	Mech* mech;

	mech = p_player->m_mech;
	if (!p_player->m_skillFlag0 || p_player->m_avoidSide || p_player->m_maneuver == c_maneuverAvoid) {
		return;
	}

	SetTarget(p_player, p_player->m_ai.m_target);
	turn = abs(GetTargetBearing(p_player));
	if (turn >= mech->m_maxTorsoTwist || (turn <= -mech->m_maxTorsoTwist && p_player->m_steering->m_throttle)) {
		if (CanJump(p_player, 20) && p_player->m_onGround && !p_player->m_steering->m_jumpJetEnabled) {
			SetJumpJets(p_player, 1);
		}
	}
	else if (!p_player->m_controlsJets && p_player->m_steering->m_jumpJetEnabled && p_player->m_onGround) {
		SetJumpJets(p_player, 0);
	}
}

// Whether p_player's mech can jump: jump fuel left (at least 6), jump jets that lift it and its
// heat (16.16) under p_limit.
// FUNCTION: MW2 0x10016222
MechS32 CanJump(Player* p_player, MechS32 p_limit)
{
	Mech* mech;

	mech = p_player->m_mech;
	return mech->m_jumpFuel >= 6 && mech->m_jumpThrust && mech->m_heat >> 16 < p_limit;
}

// The shape of the player or game thing an AI target id names, or NULL.
// The only diff is a stack-slot permutation of index, id and obj.
// FUNCTION: MW2 0x1001627f
Shape* GetTargetShape(MechS16 p_target)
{
	MechS16 index;
	MechS32 id;
	SceneObject* obj;

	obj = NULL;
	index = p_target & 0xff;
	switch (p_target & 0xf00) {
	case 0x200:
		obj = g_players[index]->m_obj;
		break;
	case 0x400:
		id = g_gameThings[index].m_staticObject;
		obj = GetStaticSceneObject(id);
		break;
	}

	return obj ? obj->m_shape : NULL;
}

// When p_mech's player fires the weapon in p_slot at another player, the target may dodge: an
// AI player that sees the shot coming (a guided weapon or type 21, and the target inside its
// sights within 3 units and 15 degrees) sidesteps along a clear path (state 11), or else braces
// (state 4).
// Stack-slot permutation; pitch < range and bearing < maxAngle compare in the other operand order.
// FUNCTION: MW2 0x1001632c
void DodgeShot(WeaponSlot* p_slot, Mech* p_mech)
{
	MechS32 maxAngle;
	MechS32 index;
	MechS32 pitch;
	MechS32 kind;
	Player* target;
	MechU32 id;
	MechS32 bearing;
	MechS32 x;
	MechS32 y;
	MechS32 range;
	MechS32 sx;
	MechS32 z;
	MechS32 sy;
	Shape* hit;
	MechS32 heading;
	Ray ray;
	MechS16 side;
	MechS16 j;
	MechS16 step;

	id = 0;
	target = NULL;
	if (!p_mech->m_player->m_skillFlag2 || !RandomIntBelow(3)) {
		return;
	}

	if (!g_weaponDefs[p_slot->m_type].m_guided && p_slot->m_type != 21) {
		return;
	}

	if (p_mech->m_player->m_aiMode == 2) {
		id = p_mech->m_player->m_targetInfo.m_target;
	}
	else if (p_mech->m_player->m_ai.m_goal & 0x200) {
		id = p_mech->m_player->m_ai.m_goal;
	}
	else {
		id = p_mech->m_player->m_targetInfo.m_target;
	}

	kind = id & 0xf00;
	index = id & 0xff;
	if (!index || kind != 0x200 || g_players[index]->m_nextManeuver == c_maneuverAjmpin) {
		return;
	}

	if (p_mech->m_player->m_aiMode == 2) {
		target = g_players[index];
	}

	if (!target && ProjectAimPoint(p_mech, &sx, &sy)) {
		x = g_players[index]->m_position.m_x;
		y = g_players[index]->m_position.m_y;
		z = g_players[index]->m_position.m_z;
		if (ProjectWorldPoint(&x, &y, &z)) {
			range = 0x30000;
			maxAngle = 15;
			pitch = p_mech->m_player->m_targetInfo.m_pitch / 0xf00;
			bearing = (GetTargetBearing(p_mech->m_player) >> 16) % 360;
			if (pitch < range && -range < pitch && bearing < maxAngle && -maxAngle < bearing) {
				target = g_players[index];
			}
		}
	}

	if (target && CanJump(target, 0x41)) {
		if (RandomIntBelow(3)) {
			heading = GetHeadingTo(target, p_mech->m_player->m_index);
			step = FixedDiv16(heading, 0x5a0000) >> 16;
			if (RandomIntBelow(2)) {
				side = -1;
			}
			else {
				side = 1;
			}

			step = ((step + 1) % 4) * 4;
			for (j = 0; j < 2; j++) {
				BuildProbeRay(target, &ray, side, step, 5000, 0);
				if (!TestSegmentCollision(&ray, &hit, target->m_index)) {
					target->m_nextManeuver = c_maneuverSprint;
					target->m_maneuverParam = step;
					break;
				}

				side = -side;
			}
		}

		if (target->m_nextManeuver == 0 && target->m_skillFlag1) {
			target->m_nextManeuver = c_maneuverAjmpin;
		}
	}
}

// Picks p_player's place around its goal player (eight places, 45 degrees apart): the one it is
// nearest, moved off the front and back, or else the first free one on its side. Returns twice
// the place.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x100166b1
MechS16 ChooseFlankPlace(Player* p_player)
{
	MechS32 angle;
	MechS32 i;
	MechS32 place;
	MechS32 found;
	Player* leader;

	leader = g_players[p_player->m_ai.m_goal & 0xff];
	angle = FixedDiv16(GetHeadingTo(leader, p_player->m_index | 0x200), 0x2d0000);
	place = (angle + 0x8000) >> 16;
	if (place >= 8) {
		place = 0;
	}

	if (place == 0 || place == 1 || place == 7) {
		if (place == 0) {
			if (RandomIntBelow(2)) {
				place = 2;
			}
			else {
				place = 6;
			}

			if (p_player->m_placesTaken[place]) {
				if (place == 2) {
					place = 6;
				}
				else {
					place = 2;
				}
			}
		}

		if (place == 1) {
			place = 3;
		}

		if (place == 7) {
			place = 5;
		}
	}
	else {
		found = FALSE;
		if (place > 4) {
			for (i = 6; i > 3 && !found; i--) {
				if (!leader->m_placesTaken[i]) {
					found = TRUE;
				}
			}

			i++;
		}
		else {
			for (i = 2; i < 5 && !found; i++) {
				if (!leader->m_placesTaken[i]) {
					found = TRUE;
				}
			}

			i--;
		}

		place = i;
	}

	return place * 2;
}

// Whether p_mech is stuck: colliding, not reversing, and not in a maneuver that jumps or avoids.
// FUNCTION: MW2 0x10016880
MechS32 IsStuck(Mech* p_mech)
{
	return p_mech->m_collisionTicks && p_mech->m_player->m_steering->m_reverse != 1 &&
		   !p_mech->m_player->m_controlsJets && p_mech->m_player->m_maneuver != c_maneuverAvoid;
}
