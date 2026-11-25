#include "aiweapons.h"

#include "ai.h"
#include "clock.h"
#include "lineofsight.h"
#include "mech.h"
#include "players.h"
#include "playersteering.h"
#include "random.h"
#include "simmain.h"
#include "soundfx.h"
#include "types.h"
#include "weapondata.h"
#include "weapondef.h"
#include "weapons.h"
#include "weaponslot.h"

// Runs p_player's AI weapons at random intervals (up to m_gunnery x 22 ticks): within 10 degrees
// of the heading p_heading (always against the local player, else one time in three) it aims at
// its goal and may fire (DecideAIFire). Then turns and pitches the torso. Returns whether it
// fired.
int32_t RunAIWeapons(Player* p_player, int32_t p_heading)
{
	int32_t fired;
	int32_t roll;
	int32_t delta;

	fired = false;
	if (p_player->m_nextFireTime <= g_currentClock) {
		roll = RandomIntBelow(p_player->m_gunnery);
		p_player->m_nextFireTime = roll * 22 + g_currentClock;
		delta = p_heading - p_player->m_torsoTwist;
		if (delta < 0xa0000 && delta > -0xa0000) {
			if ((p_player->m_ai.m_goal & 0xff) == g_localPlayerId || !RandomIntBelow(3)) {
				SetTarget(p_player, p_player->m_ai.m_goal);
				if (!roll) {
					if (DecideAIFire(p_player)) {
						if (!(p_player->m_skillFlag4) || CanSeeTarget(p_player, 1)) {
							p_player->m_steering->m_weaponFire = 1;
							fired = true;
						}
					}
				}
			}
		}
	}

	if (!fired) {
		roll = 0;
	}

	p_player->m_steering->m_torsoPan = AimTorsoPan(p_player, p_heading, roll) * 0x2d00;
	p_player->m_steering->m_torsoTilt = -(AimTorsoTilt(p_player, roll) * 0xf00);
	return fired;
}

// Decides whether p_player's AI fires its selected weapon: in range of its target, cool enough, the
// weapon ready and loaded, and by chance per recycle time; otherwise it selects the next weapon.
// Firing weapon types 0 and 4 also steers; a guided weapon's volley may lock on.
int32_t DecideAIFire(Player* p_player)
{
	int32_t fire;
	int32_t type;
	Mech* mech;
	int32_t i;
	WeaponSlot* slot;
	WeaponDef* def;

	mech = p_player->m_mech;
	for (i = 0, fire = false; i < mech->m_weaponCount && !fire; i++) {
		fire = true;
		slot = &mech->m_weapons[mech->m_selectedWeapon];
		type = slot->m_type;
		def = &g_weaponDefs[type];
		switch (GetTargetRangeBand(p_player, def)) {
		case 1:
		case 2:
			break;
		default:
			fire = false;
			break;
		}

		if (fire) {
			fire = false;
			if ((def->m_heat + mech->m_heat) >> 16 < 65.0 && IsSelectedWeaponReady(mech) == 1 && slot->m_ammo &&
				!RandomIntBelow(def->m_recycle / 90 + 1)) {
				fire = true;
			}
		}

		if (!fire) {
			SelectNextWeaponInGroup(mech, 1);
		}
	}

	if (fire) {
		if (type == 0) {
			p_player->m_steering->m_torsoTilt += 0x1e000;
		}
		else if (type == 4) {
			p_player->m_steering->m_torsoTilt += 0x3c000;
		}

		if (g_weaponDefs[type].m_guided) {
			if (p_player->m_gunnery <= 4 && !RandomIntBelow(p_player->m_gunnery + 1)) {
				mech->m_flags |= 0x80;
			}

			if (p_player->m_ai.m_goal == (g_localPlayerId | 0x200)) {
				PlaySoundEffect(0x6f, 100, 0x40, 5, 0x32);
			}
		}
	}

	return fire;
}
