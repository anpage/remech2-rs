#include "mechdamage.h"

#include "ai.h"
#include "ammobin.h"
#include "classtable.h"
#include "clock.h"
#include "config.h"
#include "decomp.h"
#include "eyepoint.h"
#include "maneuvers.h"
#include "mech.h"
#include "mechclass.h"
#include "mechsection.h"
#include "mw2log.h"
#include "network.h"
#include "object.h"
#include "objective.h"
#include "players.h"
#include "playersteering.h"
#include "polydraw.h"
#include "random.h"
#include "shape.h"
#include "shots.h"
#include "simmain.h"
#include "soundfx.h"
#include "speech.h"
#include "targeting.h"
#include "team.h"
#include "types.h"
#include "weapons.h"
#include "weaponslot.h"

#include <stdio.h>

// A game-key toggle (GetSystemSetting's setting 0x3c).
// GLOBAL: MW2 0x100a1590
MechS32 g_autoEject = 0;

// The armor per damage level of other players' sections (the local player's: g_localArmorPerLevel).
// GLOBAL: MW2 0x100a1594
MechS32 g_otherArmorPerLevel = 4;

// GLOBAL: MW2 0x100a1598
MechS32 g_localArmorPerLevel = 4;

// GLOBAL: MW2 0x100a159c
MechS32 g_localMechHidden = 0;

// The kill count the cockpit shows in a network game.
// GLOBAL: MW2 0x100a15a0
MechS32 g_killCount = 0;

// Set while the local player has been warned of critical heat (CalculateHeat).
// GLOBAL: MW2 0x100a15a4
MechS32 g_criticalHeatWarned = 0;

// When the warning was given.
// GLOBAL: MW2 0x100bdff0
MechS32 g_criticalHeatWarningTime;

// Runs the autopilot (m_autopilot): mode 1 follows the nav points in order, skipping the ones
// already reached and marking each one it reaches (turning off after the last); then the AI
// steers, the throttle saved while AvoidObstacles has it.
// Stack-slot permutation: index and first.
// FUNCTION: MW2 0x100079d0
void RunAutopilot(Mech* p_mech)
{
	MechS32 index;
	MechS32 first;

	if (p_mech->m_autopilot == 0) {
		return;
	}

	if (p_mech->m_autopilot == 1) {
		if (!(p_mech->m_player->m_targetInfo.m_target & 0x100) || (p_mech->m_player->m_targetInfo.m_target & 0x1000)) {
			CycleNavTarget(p_mech->m_player, 0, 0);
			index = p_mech->m_player->m_targetInfo.m_target & 0xff;
			first = index;
			while (g_navTable[index].m_flags & 0x20) {
				CycleNavTarget(p_mech->m_player, 1, 0);
				index = p_mech->m_player->m_targetInfo.m_target & 0xff;
				if (index == first) {
					p_mech->m_player->m_targetInfo.m_target |= 0x1000;
					return;
				}
			}
		}

		if (p_mech->m_player->m_targetInfo.m_target & 0x100) {
			index = p_mech->m_player->m_targetInfo.m_target & 0xff;
			first = index;
			if (g_navTable[index].m_radius > p_mech->m_player->m_targetInfo.m_distance &&
				p_mech->m_player->m_index == g_localPlayerId) {
				if (!(g_navTable[index].m_flags & 0x20)) {
					g_navTable[index].m_flags |= 0x20;
					g_navTable[index].m_teamsReached |= 1 << p_mech->m_player->m_team;
					if (g_navTable[index].m_flags & 0x40) {
						FUN_1001cdd1();
					}

					PlaySoundEffect(0xe7, 100, 0x40, 5, 0x50);
				}

				CycleNavTarget(p_mech->m_player, 1, 0);
				if ((p_mech->m_player->m_targetInfo.m_target & 0xff) < first) {
					p_mech->m_autopilot = 0;
					p_mech->m_player->m_targetInfo.m_target |= 0x1000;
					p_mech->m_player->m_steering->m_throttle = 0;
					p_mech->m_player->m_steering->m_throttleSet = 1;
					return;
				}
			}
		}
	}

	if (!AvoidObstacles(p_mech->m_player)) {
		SteerToTarget(p_mech->m_player);
		if (p_mech->m_player->m_maneuverFlag == 1) {
			p_mech->m_player->m_steering->m_throttle = p_mech->m_player->m_maneuverParam;
			p_mech->m_player->m_maneuverFlag = 0;
		}

		p_mech->m_player->m_maneuverParam = p_mech->m_player->m_steering->m_throttle;
	}
	else {
		p_mech->m_player->m_maneuverFlag = 1;
	}
}

// The PUNCH_IN_AUTO_HDG game key: sets the player's target heading (the HUD's bearing marker,
// which the autopilot steers by) to where the torso faces, unless the autopilot is on.
// FUNCTION: MW2 0x10007cb5
void PunchInAutoHeading(Mech* p_mech)
{
	MechS32 heading;

	if (p_mech->m_autopilot == 1) {
		return;
	}

	heading = (p_mech->m_player->m_heading + 0x1680000 + p_mech->m_torsoTwist.m_value) % 0x1680000;
	p_mech->m_player->m_targetInfo.m_heading = heading;
}

// Destroys p_mech on behalf of player p_killer, unless g_mechPoweredUp is clear: an intact
// section holding an ammunition bin with ammunition left blows up first (DestroyCriticalSlot) instead.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10007d06
void DestroyMech(MechS32 p_killer, Mech* p_mech)
{
	WeaponSlot* weapon;
	MechS32 i;
	MechS32 j;
	MechS32 k;
	MechSection* section;
	AmmoBin* bin;

	if (!g_mechPoweredUp) {
		return;
	}

	if (p_mech->m_ammoBins) {
		for (i = 0; i < 8; i++) {
			section = &p_mech->m_sections[i];
			if (section->m_flags & 0x2000) {
				continue;
			}

			for (j = 0; j < section->m_slotCount; j++) {
				if (section->m_slots[j] > 10000) {
					bin = p_mech->m_ammoBins;
					for (k = 0; k < p_mech->m_ammoBinCount; k++) {
						if (section->m_slots[j] == bin->m_id) {
							weapon = &p_mech->m_weapons[bin->m_weapon];
							if (weapon && weapon->m_ammo > 0) {
								DestroyCriticalSlot(p_killer, p_mech, i + 1, j, 0);
								return;
							}
						}

						bin++;
					}
				}
			}
		}
	}

	if (p_mech->m_player->m_index == g_localPlayerId) {
		PlayCockpitSound(0, -1);
	}

	KillMech(p_killer, p_mech);
}

// Accumulates the mech's heat each frame (m_deltaHeat less its cooling, doubled while shut down)
// in m_heat, 16.16 percent: overheating (bit 4, above 80) shuts the mech down after 6 seconds
// (state 3), and above 100 the ammunition may explode (with bit 8) or the mech is destroyed after
// 25 seconds; the local player hears the warnings. Cooling below 65 ends the shutdown.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10007e86
void CalculateHeat(Mech* p_mech)
{
	MechS32 delta;
	MechS32 cooling;
	MechS32 heat;
	MechS32 shift;

	shift = 0;
	if ((p_mech->m_player->m_flags & 2) || (p_mech->m_player->m_flags & 4)) {
		return;
	}

	if (g_isNetworkGame && p_mech->m_player->m_index != g_localPlayerId) {
		return;
	}

	if (!g_difficulty->m_heatTracking && p_mech->m_player->m_index == g_localPlayerId) {
		p_mech->m_heat = 0;
		p_mech->m_deltaHeat = 0;
		return;
	}

	if (p_mech->m_powerState == 3) {
		shift = 1;
	}

	cooling = p_mech->m_cooling * g_deltaTime << shift;
	delta = p_mech->m_deltaHeat - cooling;
	p_mech->m_heat += delta;
	if (p_mech->m_heat < 0) {
		p_mech->m_heat = 0;
	}

	heat = p_mech->m_heat >> 16;
	if ((p_mech->m_flags & 4) && !(p_mech->m_flags & 8) && p_mech->m_powerState != 3 &&
		g_currentClock - p_mech->m_stateTime > 1086) {
		p_mech->m_powerState = 3;
		p_mech->m_stateTime = g_currentClock;
		if (p_mech->m_player->m_index == g_localPlayerId) {
			PlayCockpitSound(13, -1);
		}
	}

	if (heat > 100) {
		if (p_mech->m_player->m_index == g_localPlayerId && g_difficulty->m_invulnerable) {
			return;
		}

		if (p_mech->m_flags & 8) {
			if (g_deltaTime && RandomIntBelow(4000 / g_deltaTime) < 3) {
				DestroyMech(p_mech->m_player->m_index, p_mech);
			}
		}
		else if (g_currentClock - p_mech->m_stateTime > 4525) {
			KillMech(p_mech->m_player->m_index, p_mech);
		}
	}
	else if (heat > 80.0) {
		if (!(p_mech->m_flags & 4)) {
			p_mech->m_stateTime = g_currentClock;
			p_mech->m_flags |= 4;
			if (!(p_mech->m_flags & 0x1000) && p_mech->m_player->m_index == g_localPlayerId) {
				p_mech->m_flags |= 0x1000;
				PlayCockpitSound(3, -1);
				PlaySoundEffect(0xe9, 100, 0x40, 5, 0x50);
			}
		}
	}
	else if (heat > 65.0) {
		if (p_mech->m_player->m_index == g_localPlayerId && !g_criticalHeatWarned && delta > 0) {
			g_criticalHeatWarningTime = g_currentClock;
			g_criticalHeatWarned = 1;
			PlayCockpitSound(0, -1);
		}
	}
	else if (p_mech->m_flags & 4) {
		if (p_mech->m_powerState == 3 && g_currentClock - p_mech->m_stateTime > 724) {
			p_mech->m_flags &= ~4;
			p_mech->m_powerState = 0;
		}

		if (p_mech->m_flags & 8) {
			p_mech->m_flags &= ~8;
			p_mech->m_flags &= ~4;
		}
	}
	else {
		if (g_criticalHeatWarned && g_currentClock - g_criticalHeatWarningTime > 724 &&
			p_mech->m_player->m_index == g_localPlayerId) {
			g_criticalHeatWarned = 0;
		}

		p_mech->m_flags &= ~0x1000;
	}
}

// Destroys p_mech, on behalf of player p_killer (-2: its player left the game).
// Stack-slot permutation: next and text. The original compares p_killer with g_localPlayerId and
// indexes m_unk0x52 in the other operand order.
// FUNCTION: MW2 0x1000832b
void KillMech(MechS32 p_killer, Mech* p_mech)
{
	MechS32 leader;
	MechS32 next;
	MechChar text[80];

	if (p_mech->m_player->m_index == g_localPlayerId && g_difficulty->m_invulnerable) {
		return;
	}

	if ((p_mech->m_player->m_flags & 2) || (p_mech->m_player->m_flags & 4)) {
		return;
	}

	if (g_aimedShape && (g_aimedShape->m_kind & 0x100) && p_mech->m_player->m_index == g_aimedShape->m_owner) {
		g_aimedShape = NULL;
	}

	p_mech->m_player->m_killer = p_killer;
	if (p_mech->m_player->m_killer >= 0 && p_mech->m_player->m_killer < 8 && p_mech->m_player->m_index < 8) {
		g_careerRecord.m_kills[p_mech->m_player->m_killer][p_mech->m_player->m_index]++;
	}

	ChooseNetworkWinner();
	if (p_mech->m_powerState == 5) {
		if (g_hostileAtmosphere) {
			if (p_mech->m_player->m_index == g_localPlayerId) {
				g_careerRecord.m_outcome = 4;
				PlayCockpitSound(0x20, -1);
			}
			p_mech->m_powerState = 4;
		}
		else if (p_mech->m_player->m_index == g_localPlayerId) {
			g_careerRecord.m_outcome = 2;
			PlayCockpitSound(0xc, -1);
		}
	}

	if (p_killer == g_localPlayerId) {
		if (p_mech->m_player->m_type == c_playerTypeMech) {
			if (p_mech->m_player->m_index == g_localPlayerId) {
				g_killCount--;
			}
			else {
				g_killCount++;
			}

			switch (GetPlayerSide(p_mech->m_player->m_index)) {
			case 0:
				g_careerRecord.m_directFriendlyMechKills++;
				break;
			case 1:
				g_careerRecord.m_directMechKills++;
				break;
			case 2:
				g_careerRecord.m_directNeutralMechKills++;
				break;
			}
		}
		else {
			switch (GetPlayerSide(p_mech->m_player->m_index)) {
			case 0:
				g_careerRecord.m_directFriendlyVehicleKills++;
				break;
			case 1:
				g_careerRecord.m_directVehicleKills++;
				break;
			case 2:
				g_careerRecord.m_directNeutralVehicleKills++;
				break;
			}
		}
	}
	else {
		next = p_mech->m_player->m_index;
		next++;
	}

	if (p_mech->m_player->m_flags & 0x1400) {
		if (p_mech->m_player->m_type == c_playerTypeMech) {
			switch (GetPlayerSide(p_mech->m_player->m_index)) {
			case 0:
				g_careerRecord.m_friendlyMechKills++;
				break;
			case 1:
				g_careerRecord.m_mechKills++;
				break;
			case 2:
				g_careerRecord.m_neutralMechKills++;
				break;
			}
		}
		else {
			switch (GetPlayerSide(p_mech->m_player->m_index)) {
			case 0:
				g_careerRecord.m_friendlyVehicleKills++;
				break;
			case 1:
				g_careerRecord.m_vehicleKills++;
				break;
			case 2:
				g_careerRecord.m_neutralVehicleKills++;
				break;
			}
		}
	}

	if (g_players[g_localPlayerId]->m_team == p_mech->m_player->m_team) {
		if (p_mech->m_powerState == 5) {
			g_careerRecord.m_wingmenEjected++;
		}
		else {
			g_careerRecord.m_wingmenLost++;
		}
	}

	if (GetPlayerSide(p_mech->m_player->m_index) == 1) {
		switch (p_mech->m_player->m_type) {
		case 1:
			PlayCockpitSound(0x15, -1);
			break;
		case 2:
			PlayCockpitSound(0x16, -1);
			break;
		case 3:
			PlayCockpitSound(0x19, -1);
			break;
		case 4:
			PlayCockpitSound(0x16, -1);
			break;
		case 5:
			PlayCockpitSound(0x17, -1);
			break;
		case 8:
			PlayCockpitSound(0x17, -1);
			break;
		case 6:
		case 7:
			break;
		}
	}

	p_mech->m_player->m_flags |= 6;
	p_mech->m_stateTime = 0;
	if (!g_isNetworkGame) {
		if (p_mech->m_powerState != 5 || p_mech->m_player->m_index != g_localPlayerId) {
			p_mech->m_powerState = 4;
		}

		ResetAI(p_mech->m_player);
		if (GetTeamLeader(p_mech->m_player->m_team) == p_mech->m_player->m_index) {
			leader = ChooseTeamLeader(p_mech->m_player->m_team);
			RetargetGoals(p_mech->m_player->m_team, p_mech->m_player->m_index, leader);
			sprintf(text, "%6ld : New leader for group %d : %d\n", g_currentClock, p_mech->m_player->m_team, leader);
			WriteToMw2Log(text);
		}
	}
	else {
		p_mech->m_powerState = 4;
	}

	if (!g_isNetworkGame && p_mech->m_player->m_index != g_localPlayerId &&
		GetTeamLeader(p_mech->m_player->m_team) == g_localPlayerId) {
		SayLancemateReport(6, p_mech->m_player->m_slot);
	}

	LoadClassLevel(p_mech->m_player->m_index, 1);
}

// Calls DestroyCriticalSlot once for each of section p_section's m_slotCount.
// Stack-slot permutation of i, section and count; the loop test compares with i in eax in the
// original (operand order).
// FUNCTION: MW2 0x10008938
void DestroySectionSlots(MechS32 p_attacker, Mech* p_mech, MechU32 p_section)
{
	MechS32 i;
	MechSection* section;
	MechS32 count;

	section = p_mech->m_sections + p_section - 1;
	count = section->m_slotCount;
	for (i = 0; i < count; i++) {
		DestroyCriticalSlot(p_attacker, p_mech, p_section, 0, 1);
	}
}

// Destroys section p_section of p_mech, on behalf of player p_attacker.
// Destroying section 3 takes sections 1, 2 and 4 to 6 with it; section 2 takes 5, and 4 takes 6.
// Of sections 7 and 8, the first to go only marks the mech (0x20); the second destroys 1 and 3.
// FUNCTION: MW2 0x1000899d
void DestroySection(MechS32 p_attacker, Mech* p_mech, MechU32 p_section)
{
	MechSection* section;

	if (!p_section) {
		return;
	}

	section = p_mech->m_sections + p_section - 1;
	if (section->m_flags & 0x2000) {
		return;
	}

	section->m_internal = 0;
	section->m_flags |= 0x2000;
	DestroySectionSlots(p_attacker, p_mech, p_section);
	switch (p_section) {
	case 2:
		DestroySection(p_attacker, p_mech, 5);
		section->m_flags &= ~0x2000;
		return;
	case 4:
		DestroySection(p_attacker, p_mech, 6);
		section->m_flags &= ~0x2000;
		return;
	case 7:
	case 8:
		if (!(p_mech->m_flags & 0x20)) {
			BlowOffPart(p_mech->m_player->m_obj, p_section);
			p_mech->m_mobility = 0;
			p_mech->m_flags |= 0x20;
			return;
		}
		else {
			DestroySection(p_attacker, p_mech, 1);
			DestroySection(p_attacker, p_mech, 3);
		}
		break;
	case 3:
		DestroySection(p_attacker, p_mech, 5);
		DestroySection(p_attacker, p_mech, 2);
		DestroySection(p_attacker, p_mech, 4);
		DestroySection(p_attacker, p_mech, 6);
		DestroySection(p_attacker, p_mech, 1);
		break;
	case 5:
	case 6:
		BlowOffPart(p_mech->m_player->m_obj, p_section);
		return;
	}

	BlowOffPart(p_mech->m_player->m_obj, p_section);
	if (p_mech->m_powerState != 4 && p_mech->m_powerState != 5 &&
		(p_mech->m_player->m_index == g_localPlayerId || !g_isNetworkGame)) {
		KillMech(p_attacker, p_mech);
	}
}

// Destroys critical slot p_slot of section p_section of p_mech, on behalf of player p_attacker,
// and drops it from the section: a weapon (ids below 5000) stops working, an ammunition bin (above
// 10000) explodes with its shots, damaging the section, and the equipment loses its function (the
// jump jets, the heat sinks, the engine, the gyro...). The local player hears which, unless
// p_recursing; 8000 and 9000 hit another, random slot instead.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10008c0f
void DestroyCriticalSlot(MechS32 p_attacker, Mech* p_mech, MechU32 p_section, MechS32 p_slot, MechS32 p_recursing)
{
	MechS32 id;
	WeaponSlot* weapon;
	MechS32 x;
	MechS32 y;
	MechS32 i;
	MechS32 z;
	struct SceneObject* obj;
	MechS32 kind;
	MechSection* section;
	AmmoBin* bin;
	MechS32 damage;

	section = p_mech->m_sections + p_section - 1;
	if (!section) {
		return;
	}

	if (p_slot >= section->m_slotCount || p_slot < 0) {
		return;
	}

	id = section->m_slots[p_slot];
	kind = id / 100;
	if (kind < 50) {
		weapon = p_mech->m_weapons;
		for (i = 0; i < 10; i++) {
			if (weapon->m_type < 0) {
				weapon++;
				continue;
			}

			if (weapon->m_slotId == id) {
				if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId && weapon->m_status) {
					switch (kind) {
					case 0:
					case 1:
					case 2:
					case 3:
						SayCriticalHit(0x16);
						break;
					case 4:
					case 5:
					case 6:
						SayCriticalHit(0x17);
						break;
					case 7:
					case 8:
					case 9:
						SayCriticalHit(0x18);
						break;
					case 10:
						SayCriticalHit(0x19);
						break;
					case 11:
						SayCriticalHit(0x1a);
						break;
					case 12:
					case 13:
					case 14:
					case 15:
						SayCriticalHit(0x1b);
						break;
					case 16:
					case 17:
					case 18:
					case 19:
						SayCriticalHit(0x1c);
						break;
					case 20:
						break;
					case 21:
						SayCriticalHit(0x1d);
						break;
					case 22:
						SayCriticalHit(0x1e);
						break;
					case 23:
						SayCriticalHit(0x1f);
						break;
					case 24:
						SayCriticalHit(0x20);
						break;
					case 25:
						SayCriticalHit(0x21);
						break;
					case 26:
						SayCriticalHit(0x22);
						break;
					case 27:
						SayCriticalHit(0x23);
						break;
					}

					if (p_mech->m_powerState == 2) {
						PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
					}

					SayCriticalHit(0);
				}

				weapon->m_status = 0;
				weapon->m_state = c_weaponEmpty;
				break;
			}

			weapon++;
		}
	}
	else if (kind > 100) {
		bin = p_mech->m_ammoBins;
		for (i = 0; i < p_mech->m_ammoBinCount; i++) {
			if (bin->m_id == id) {
				weapon = &p_mech->m_weapons[bin->m_weapon];
				if (weapon->m_ammo < 1) {
					break;
				}

				if (weapon) {
					weapon->m_ammo -= bin->m_shots;
					if (!weapon->m_ammo) {
						weapon->m_status = 0;
						weapon->m_state = c_weaponEmpty;
					}
				}

				if (!p_recursing && bin->m_shots) {
					if (p_mech->m_player->m_index == g_localPlayerId) {
						if (p_mech->m_powerState == 2) {
							PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
						}

						x = p_mech->m_player->m_position.m_x - g_eyepoint->m_x;
						y = p_mech->m_player->m_position.m_y - g_eyepoint->m_y;
						z = p_mech->m_player->m_position.m_z - g_eyepoint->m_z;
						g_hitFadePending = 1;
						PlaySoundAt(x, y, z, 0xbc, g_inCockpitView);
						SayCriticalHit(1);
					}

					obj = FindObjByPart(p_mech->m_player->m_obj, p_section);
					if (!obj) {
						obj = FindObjByPart(p_mech->m_player->m_obj, 3);
					}

					if (obj) {
						GetShapeBounds(GetObjShape(obj), &x, &y, &z);
						SpawnEffect(p_attacker, 7, x, y, z, x, y, z);
					}

					damage = bin->m_shots * bin->m_damage;
					section->m_internal -= damage << 16;
					if (section->m_internal < 0) {
						section->m_internal = 0;
						section->m_slots[p_slot] = 0;
						DestroySection(p_attacker, p_mech, p_section);
					}

					if (g_autoEject && (p_mech->m_player->m_index == g_localPlayerId || !g_isNetworkGame)) {
						p_mech->m_powerState = 5;
						KillMech(p_attacker, p_mech);
					}
				}

				bin->m_shots = 0;
			}

			bin++;
		}
	}
	else {
		kind = id / 10 * 10;
		switch (kind) {
		case 9000:
			if (!p_recursing) {
				DestroyCriticalSlot(p_attacker, p_mech, p_section, RandomIntBelow(section->m_slotCount), 1);
				return;
			}
			else {
				break;
			}
		case 8000:
			if (!p_recursing) {
				DestroyCriticalSlot(p_attacker, p_mech, p_section, RandomIntBelow(section->m_slotCount), 1);
				return;
			}
			else {
				break;
			}
		case 7000:
			if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId && p_mech->m_jumpFuel != 2) {
				if (p_mech->m_powerState == 2) {
					PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
				}

				SayCriticalHit(4);
			}

			if (p_mech->m_jumpJets > 0) {
				p_mech->m_jumpThrust -= p_mech->m_jumpThrust / p_mech->m_jumpJets;
			}
			else {
				p_mech->m_jumpThrust = 0;
			}

			if (p_mech->m_jumpJets > 0) {
				p_mech->m_jumpJets--;
			}

			if (!p_mech->m_jumpJets) {
				p_mech->m_jumpFuel = -2;
			}
			break;
		case 6000:
			if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId && p_mech->m_cooling) {
				if (p_mech->m_powerState == 2) {
					PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
				}

				SayCriticalHit(5);
			}

			if (p_mech->m_cooling > 0) {
				p_mech->m_cooling -= 50;
			}
			else {
				p_mech->m_cooling = 0;
			}
			break;
		case 5900:
			if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId) {
				if (p_mech->m_powerState == 2) {
					PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
				}

				SayCriticalHit(6);
			}

			if (g_hostileAtmosphere && (p_mech->m_player->m_index == g_localPlayerId || !g_isNetworkGame)) {
				KillMech(p_attacker, p_mech);
			}
			break;
		case 5850:
			if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId) {
				if (p_mech->m_powerState == 2) {
					PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
				}

				SayCriticalHit(7);
			}

			p_mech->m_mobility -= 0x199a;
			if (p_mech->m_mobility < 0) {
				p_mech->m_mobility = 0;
			}
			break;
		case 5800:
			if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId) {
				if (p_mech->m_powerState == 2) {
					PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
				}

				SayCriticalHit(8);
			}

			p_mech->m_mobility -= 0x199a;
			if (p_mech->m_mobility < 0) {
				p_mech->m_mobility = 0;
			}

			p_mech->m_jumpFuel = -2;
			p_mech->m_jumpThrust = 0;
			p_mech->m_jumpJets = 0;
			break;
		case 5750:
			if (p_mech->m_player->m_index == g_localPlayerId || !g_isNetworkGame) {
				KillMech(p_attacker, p_mech);
			}
			break;
		case 5700:
			if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId) {
				if (p_mech->m_powerState == 2) {
					PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
				}

				SayCriticalHit(9);
			}

			DamageCockpitPanels(p_mech, 1);
			break;
		case 5550:
		case 5600:
		case 5650:
			if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId) {
				if (p_mech->m_powerState == 2) {
					PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
				}

				SayCriticalHit(10);
			}

			p_mech->m_mobility -= 0x199a;
			if (p_mech->m_mobility < 0) {
				p_mech->m_mobility = 0;
			}
			break;
		case 5500:
			if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId) {
				if (p_mech->m_powerState == 2) {
					PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
				}

				SayCriticalHit(11);
			}

			p_mech->m_mobility -= 0x199a;
			if (p_mech->m_mobility < 0) {
				p_mech->m_mobility = 0;
			}
			break;
		case 5350:
		case 5400:
		case 5450:
			if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId) {
				if (p_mech->m_powerState == 2) {
					PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
				}

				SayCriticalHit(12);
			}

			DamageCockpitPanels(p_mech, 0);
			break;
		case 5300:
			if (!p_recursing && p_mech->m_player->m_index == g_localPlayerId) {
				if (p_mech->m_powerState == 2) {
					PlaySoundEffect(0xd0, 100, 0x40, 5, 0x50);
				}

				SayCriticalHit(9);
			}

			DamageCockpitPanels(p_mech, 0);
			break;
		}

		if (p_mech->m_mobility && p_mech->m_mobility < 0x6666) {
			p_mech->m_mobility = 0x6666;
		}
	}

	for (i = p_slot; i < section->m_slotCount - 1; i++) {
		section->m_slots[i] = section->m_slots[i + 1];
	}

	section->m_slots[i] = 0;
	section->m_slotCount--;
}

// Deals p_damage (16.16) to section p_section of the mech, on behalf of player p_attacker: to its
// rear armor with bit 0x8000 (sections 2 to 4), and past the armor to the internal structure,
// which may destroy the section or hit its critical slots. Section 3 stands for a random one of
// 2 to 4. The local player takes none while invulnerable, and in a network game each machine
// only damages its own mech.
// The original ends in an explicit return (the jmp to the epilogue). The only diff is a stack-slot
// permutation of the locals.
// FUNCTION: MW2 0x1000991b
void ApplyDamageToMech(MechS32 p_attacker, Mech* p_mech, MechS32 p_damage, MechS32 p_section)
{
	MechS32 side;
	MechU32 levels;
	MechS32 rear;
	MechS32 i;
	MechS32 level;
	MechS32 front;
	MechS32 roll;
	MechSection* section;

	side = 0;
	front = TRUE;
	rear = FALSE;
	level = 0;
	roll = 0;
	if (p_mech->m_player->m_index == g_localPlayerId && g_difficulty->m_invulnerable) {
		return;
	}

	if (!g_mechPoweredUp) {
		return;
	}

	if (g_isNetworkGame && p_mech->m_player->m_index != g_localPlayerId) {
		return;
	}

	if (p_damage <= 0) {
		return;
	}

	if (p_section & 0x8000) {
		rear = TRUE;
		p_section &= ~0x8000;
	}

	if (p_section < 1 || p_section > 8) {
		return;
	}

	if (p_section == 3) {
		p_section = RandomIntBelow(3);
		switch (p_section) {
		case 0:
			p_section = 2;
			break;
		case 1:
			p_section = 3;
			break;
		case 2:
			p_section = 4;
			break;
		}
	}

	section = p_mech->m_sections + p_section - 1;
	if (rear && (p_section == 2 || p_section == 4 || p_section == 3)) {
		side = 1;
		front = FALSE;
		levels = (section->m_flags & 0xf0) >> 4;
	}
	else {
		levels = section->m_flags & 0xf;
	}

	section->m_armor[side] -= p_damage;
	section->m_flags |= 0x8000;
	if (section->m_armor[side] <= 0) {
		if (!(section->m_flags & 0x4000) && p_mech->m_player->m_index == g_localPlayerId && p_mech->m_powerState == 2) {
			PlaySoundEffect(0xec, 100, 0x40, 5, 0x50);
		}

		section->m_flags |= 0x4000;
		section->m_internal += section->m_armor[side];
		section->m_armor[side] = 0;
		if (p_mech->m_player->m_index == g_localPlayerId && (p_section == 1 || p_section == 3) &&
			p_mech->m_powerState != 4 && p_damage > 0x20000) {
			g_hitFadePending = 1;
		}

		if (section->m_internal <= 0) {
			DestroySection(p_attacker, p_mech, p_section);
			return;
		}
		else {
			for (i = 0; i < RandomIntBelow(5); i++) {
				roll = RandomIntBelow(100);
				if (roll < 20 && section->m_slotCount > 0) {
					if (roll == 12) {
						DestroySection(p_attacker, p_mech, p_section);
						return;
					}
					else {
						DestroyCriticalSlot(p_attacker, p_mech, p_section, RandomIntBelow(section->m_slotCount), 0);
					}
				}
			}
		}
	}

	if (levels) {
		if (p_mech->m_player->m_index == g_localPlayerId) {
			level = 15 - ((section->m_internal + section->m_armor[side] / g_localArmorPerLevel) * 3) /
							 (MechS32) (levels << 16);
		}
		else {
			level = 15 - ((section->m_internal + section->m_armor[side] / g_otherArmorPerLevel) * 3) /
							 (MechS32) (levels << 16);
		}
	}

	RaisePartDamageLevel(p_mech->m_player->m_obj, level, p_section);
	return;
}

// Ejects from p_mech, unless it is already shutting down or ejecting: the local player (p_eject)
// ejects with a sound, or hears that the ejection system is disabled, and the mech is destroyed.
// FUNCTION: MW2 0x10009d2a
void EjectPlayer(Mech* p_mech, MechS32 p_eject)
{
	if (p_mech->m_powerState == 4 || p_mech->m_powerState == 5) {
		return;
	}

	if (p_eject && p_mech->m_player->m_index == g_localPlayerId) {
		if (!g_hostileAtmosphere) {
			p_mech->m_powerState = 5;
			PlaySoundEffect(0xc5, 100, 0x40, 5, 0x32);
		}
		else {
			PlayCockpitSound(0x20, -1);
		}
	}

	KillMech(p_mech->m_player->m_index, p_mech);
	return;
}

// Toggles the local player's object between HideObjTree and ShowObjTree.
// FUNCTION: MW2 0x10009dd2
void ToggleLocalMechVisible(void)
{
	if (!g_localMechHidden) {
		HideObjTree(g_players[g_localPlayerId]->m_obj);
	}
	else {
		ShowObjTree(g_players[g_localPlayerId]->m_obj);
	}

	g_localMechHidden = !g_localMechHidden;
}
