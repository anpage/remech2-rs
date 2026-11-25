#include "weapons.h"

#include "careerrecord.h"
#include "clock.h"
#include "collision.h"
#include "config.h"
#include "effectinfo.h"
#include "environment.h"
#include "eyepoint.h"
#include "fadepal.h"
#include "fixedmul.h"
#include "fixedsqrt.h"
#include "fixedtrig.h"
#include "inradius.h"
#include "maneuvers.h"
#include "mech.h"
#include "mechclass.h"
#include "mw2prj.h"
#include "network.h"
#include "object.h"
#include "players.h"
#include "polydraw.h"
#include "ramp.h"
#include "random.h"
#include "ray.h"
#include "resource.h"
#include "shape.h"
#include "shots.h"
#include "simmain.h"
#include "soundfx.h"
#include "speech.h"
#include "timedoverlays.h"
#include "transform.h"
#include "types.h"
#include "weapondata.h"
#include "weapondef.h"
#include "weaponslot.h"

#include <stdio.h>
#include <stdlib.h>

// A mech's weapons: selecting them and their groups, firing volleys, the ammunition and the
// target lock of guided weapons.

Shape* g_aimedShape = NULL;

// The fire button fires only the selected weapon; cleared, it fires them all.
int32_t g_singleWeaponFire = 1;

// The weapons each remote player's weapons message fired, and those the local player fired since
// the last one (network.c).

int32_t g_remoteWeaponsFired[10];

int32_t g_localWeaponsFired[10];

void ReleaseWeaponTriggers(Mech* p_mech)
{
	WeaponSlot* slot;
	int32_t i;

	for (i = 0; i < 10; i++) {
		slot = &p_mech->m_weapons[i];
		if (slot->m_state == c_weaponFiring || slot->m_state == c_weaponHeld) {
			slot->m_state = c_weaponReady;
		}
	}

	for (i = 0; i < 10; i++) {
		g_localWeaponsFired[i] = 0;
		g_remoteWeaponsFired[i] = 0;
	}
}

void UpdateWeaponFireState(Mech* p_mech)
{
	int32_t dy;
	int32_t first;
	int32_t dx;
	int32_t dz;
	WeaponSlot* slot;
	int32_t time;
	WeaponDef* def;
	int32_t i;
	int32_t pan;
	int32_t fired;

	first = FALSE;
	if (p_mech->m_powerState != 2) {
		return;
	}

	p_mech->m_lastSelectedWeapon = p_mech->m_selectedWeapon;
	if (p_mech->m_player->m_steering->m_toggleGroupFire) {
		if (g_singleWeaponFire) {
			g_singleWeaponFire = 0;
			PlayCockpitSound(7, 1);
		}
		else {
			g_singleWeaponFire = 1;
			PlayCockpitSound(6, 1);
		}
	}

	if (p_mech->m_flags & 0x4000) {
		AddNextWeaponToGroup(p_mech);
	}
	else if (p_mech->m_selectedWeapon != -1) {
		slot = &p_mech->m_weapons[p_mech->m_selectedWeapon];
		def = &g_weaponDefs[slot->m_type];
		if (!g_singleWeaponFire && p_mech->m_player->m_index == g_localPlayerId) {
			p_mech->m_player->m_steering->m_weaponFireGroup = p_mech->m_player->m_steering->m_weaponFire;
			p_mech->m_player->m_steering->m_weaponFire = 0;
		}

		if (p_mech->m_player->m_steering->m_weaponFireGroup) {
			if (!(p_mech->m_flags & 1)) {
				p_mech->m_flags |= 1;
				FireWeaponGroup();
			}
		}
		else if (p_mech->m_player->m_steering->m_weaponFireGroup1) {
			if (!(p_mech->m_flags & 1)) {
				p_mech->m_flags |= 1;
				if (SelectWeaponGroup(p_mech, 0)) {
					FireWeaponGroup();
				}
			}
		}
		else if (p_mech->m_player->m_steering->m_weaponFireGroup2) {
			if (!(p_mech->m_flags & 1)) {
				p_mech->m_flags |= 1;
				if (SelectWeaponGroup(p_mech, 1)) {
					FireWeaponGroup();
				}
			}
		}
		else if (p_mech->m_player->m_steering->m_weaponFireGroup3) {
			if (!(p_mech->m_flags & 1)) {
				p_mech->m_flags |= 1;
				if (SelectWeaponGroup(p_mech, 2)) {
					FireWeaponGroup();
				}
			}
		}
		else if (p_mech->m_player->m_steering->m_weaponFire) {
			if (!(p_mech->m_flags & 1)) {
				p_mech->m_flags |= 1;
				if (slot->m_state == c_weaponReady) {
					slot->m_state = c_weaponFiring;
					slot->m_volley = def->m_volley;
					slot->m_time = 0;
					DodgeShot(slot, p_mech);
					if (def->m_guided && (p_mech->m_flags & 0x80)) {
						slot->m_target = p_mech->m_player->m_targetInfo.m_target & 0xff;
						slot->m_targetKind = p_mech->m_player->m_targetInfo.m_target & 0xf00;
					}
					else {
						slot->m_target = slot->m_targetKind = 0;
					}
				}
				else {
					PlaySoundEffect(0x149, 100, 0x40, 5, 0x32);
				}
			}
		}
		else if (p_mech->m_flags & 1) {
			p_mech->m_flags &= ~1;
			SelectNextWeaponInGroup(p_mech, 0);
		}
	}

	for (i = 0; i < 10; i++) {
		slot = &p_mech->m_weapons[i];
		if (slot->m_type < 0) {
			continue;
		}

		def = &g_weaponDefs[slot->m_type];
		switch (slot->m_state) {
		case c_weaponRecycling:
			if (g_currentClock - slot->m_time >= def->m_recycle) {
				slot->m_state = c_weaponReady;
				slot->m_time = 0;
			}
			break;
		case c_weaponHeld:
			if (g_currentClock - slot->m_time >= def->m_recycle) {
				if (&p_mech->m_weapons[p_mech->m_selectedWeapon] == slot ||
					p_mech->m_player->m_steering->m_weaponFireGroup ||
					p_mech->m_player->m_steering->m_weaponFireGroup1 ||
					p_mech->m_player->m_steering->m_weaponFireGroup2 ||
					p_mech->m_player->m_steering->m_weaponFireGroup3) {
					slot->m_state = c_weaponFiring;
				}
				else {
					slot->m_state = c_weaponReady;
				}

				slot->m_time = 0;
			}
			break;
		case c_weaponFiring:
			if (slot->m_volley > 0) {
				if (slot->m_volley == def->m_volley) {
					first = TRUE;
				}

				time = slot->m_time + g_deltaTime;
				while (def->m_interval <= time && slot->m_volley > 0) {
					p_mech->m_player->m_firingObj = p_mech->m_objects[slot->m_hardpoint];
					fired = SpawnShot(p_mech->m_player, slot);
					if (fired) {
						SpawnLaunchEffect(def->m_launchEffect, p_mech->m_player);
						SpawnLaunchEffect(def->m_launchEffect2, p_mech->m_player);
						slot->m_volley--;
						time -= def->m_interval;
						slot->m_time = time;
						p_mech->m_deltaHeat += def->m_heat;

						if (first) {
							first = FALSE;
							if (p_mech->m_player->m_index == g_localPlayerId) {
								g_localWeaponsFired[slot->m_index] = 1;
							}

							if (def->m_sound > 0) {
								if (p_mech->m_player->m_index == g_localPlayerId && g_inCockpitView) {
									switch (slot->m_hardpoint) {
									case 2:
									case 5:
										pan = 0x4f;
										break;
									case 4:
									case 6:
										pan = 0x2f;
										break;
									default:
										pan = 0x40;
									}

									PlayWeaponLaunchSound(def->m_shotType, def->m_sound, pan);
								}
								else {
									dx = g_eyepoint->m_x - p_mech->m_player->m_position.m_x;
									dy = g_eyepoint->m_y - p_mech->m_player->m_position.m_y;
									dz = g_eyepoint->m_z - p_mech->m_player->m_position.m_z;
									PlaySoundAt(dx, dy, dz, def->m_sound, g_inCockpitView);
								}
							}
						}

						if (slot->m_volley == 0 && def->m_autoRepeat &&
							(p_mech->m_player->m_steering->m_weaponFire ||
							 p_mech->m_player->m_steering->m_weaponFireGroup ||
							 p_mech->m_player->m_steering->m_weaponFireGroup1 ||
							 p_mech->m_player->m_steering->m_weaponFireGroup2 ||
							 p_mech->m_player->m_steering->m_weaponFireGroup3)) {
							slot->m_time = g_currentClock;
							slot->m_state = c_weaponHeld;
							slot->m_volley = def->m_volley;
							break;
						}
					}
					else {
						p_mech->m_flags &= ~1;
						slot->m_volley = 0;
					}
				}

				if (slot->m_state != c_weaponHeld) {
					slot->m_time = time;
				}
			}
			else {
				slot->m_time = g_currentClock;
				slot->m_state = c_weaponRecycling;
				slot->m_volley = 0;
			}
			break;
		}
	}
}

// Launches one shot of p_slot's weapon from the player's current hardpoint. Returns 0 when the
// weapon can't fire again: no hardpoint, or out of ammunition.
int32_t SpawnShot(Player* p_player, WeaponSlot* p_slot)
{
	int32_t result;
	Shot* shot;
	WeaponDef* def;
	int32_t found;
	int32_t dx;
	int32_t dy;
	int32_t dz;
	int32_t i;
	int32_t speed;

	def = &g_weaponDefs[p_slot->m_type];
	result = 1;
	if (!p_player || !p_player->m_firingObj) {
		return 0;
	}

	found = FALSE;
	if (!p_slot->m_ammo) {
		return 0;
	}

	if (p_player->m_index == g_localPlayerId) {
		g_careerRecord.m_shotsFired++;
	}

	if (g_players[p_player->m_index]->m_team == g_players[g_localPlayerId]->m_team) {
		g_careerRecord.m_teamShotsFired++;
	}

	if (p_slot->m_ammo != -1 && (!g_difficulty->m_unlimitedAmmo || p_player->m_index != g_localPlayerId) &&
		(!g_isNetworkGame || p_player->m_index == g_localPlayerId)) {
		p_slot->m_ammo--;
		if (p_slot->m_bin->m_shots) {
			p_slot->m_bin->m_shots--;
		}
		else {
			p_slot->m_binIndex++;
			if (p_slot->m_binCount == p_slot->m_binIndex) {
				p_slot->m_binIndex--;
			}
			else {
				p_slot->m_bin++;
			}
		}

		if (p_slot->m_ammo == 0) {
			p_slot->m_state = c_weaponEmpty;
			result = 0;
		}
	}

	if (def->m_launchesShot) {
		for (i = 0; !found && i < 0xaf; i++) {
			shot = &g_shots[i];
			if (def->m_shotType == shot->m_type && !shot->m_flags && shot->m_object) {
				found = TRUE;
			}
		}

		if (found) {
			if (p_player->m_index != g_localPlayerId) {
				UpdateAimDistance(p_player);
			}

			shot->m_age = p_slot->m_time;
			shot->m_lifetime = def->m_lifetime - p_slot->m_time;
			shot->m_impact = def->m_impact;
			shot->m_damage = def->m_damage;
			shot->m_heat = def->m_shotHeat;
			shot->m_flags = 1;
			shot->m_target = p_slot->m_target;
			shot->m_targetKind = p_slot->m_targetKind;
			shot->m_shooter = p_player->m_index;
			speed = def->m_speed;
			GetMechAimDirection(p_player, &dx, &dy, &dz);
			NormalizeVectorGuarded(&dx, &dy, &dz);
			shot->m_velocity[0] = dx * speed;
			shot->m_velocity[1] = dy * speed;
			shot->m_velocity[2] = dz * speed;
			shot->m_steering[1] = FixedMul16(-g_gravity, def->m_gravity << 16);
			shot->m_steering[0] = shot->m_steering[2] = 0;
			SpawnLaunchFx(p_player, shot->m_object, dx, dy, dz, shot->m_type == 3 || shot->m_type == 4);
			ShowObjTree(shot->m_object);
			DisableObjTreeCollision(shot->m_object);
			UpdateObj(shot->m_object);

			if (p_player->m_index == g_localPlayerId) {
				if (shot->m_type == 3 || shot->m_type == 4) {
					g_lastLocalMissile = i - 1;
				}
				else {
					g_lastLocalMissile = -1;
				}
			}
		}
	}

	return result;
}

// Selects the next weapon of the selected one's group; p_wrap moves on to the next group when
// there is none.
void SelectNextWeaponInGroup(Mech* p_mech, int32_t p_wrap)
{
	int32_t index;
	int32_t group;
	WeaponSlot* slot;
	int32_t found;
	int32_t i;
	int32_t selected;

	found = FALSE;
	if (p_mech->m_weaponCount <= 0) {
		return;
	}

	selected = p_mech->m_selectedWeapon;
	group = p_mech->m_weapons[p_mech->m_selectedWeapon].m_group;
	index = selected;
	for (i = 1; i < 10; i++) {
		index++;
		index %= 10;
		slot = &p_mech->m_weapons[index];
		if (slot->m_type < 0) {
			continue;
		}

		if (slot->m_status != -1 && slot->m_group == group && slot->m_state != c_weaponEmpty && index != selected) {
			found = TRUE;
			p_mech->m_selectedWeapon = index;
			break;
		}
	}

	if (!found && p_wrap) {
		SelectNextWeaponGroup(p_mech);
	}
}

// Selects the next weapon that still has ammunition.
void SelectNextWeapon(Mech* p_mech)
{
	int32_t done;
	int32_t tries;

	done = FALSE;
	tries = 0;
	if (p_mech->m_weaponCount <= 0) {
		return;
	}

	while (!done) {
		p_mech->m_selectedWeapon++;
		p_mech->m_selectedWeapon %= 10;
		if (p_mech->m_weapons[p_mech->m_selectedWeapon].m_type < 0) {
			continue;
		}

		if (g_manualWeaponCycle && p_mech->m_player->m_index == g_localPlayerId) {
			return;
		}

		if (p_mech->m_weapons[p_mech->m_selectedWeapon].m_status == -1) {
			p_mech->m_selectedWeapon = 0;
		}

		if (p_mech->m_weapons[p_mech->m_selectedWeapon].m_state != c_weaponEmpty || ++tries == 10) {
			done = TRUE;
		}
	}
}

// Selects the first weapon of the next group that has one.
void SelectNextWeaponGroup(Mech* p_mech)
{
	int32_t group;
	int32_t i;
	int32_t found;
	int32_t selected;
	int32_t current;

	i = 0;
	found = FALSE;
	selected = p_mech->m_selectedWeapon;
	if (p_mech->m_weaponCount <= 0) {
		return;
	}

	current = p_mech->m_weapons[p_mech->m_selectedWeapon].m_group;
	group = current + 1;
	group %= 10;
	while (group != current && !found) {
		for (i = 0; i < 10 && !found; i++) {
			if (p_mech->m_weapons[i].m_type < 0) {
				continue;
			}

			if (p_mech->m_weapons[i].m_status != -1 && p_mech->m_weapons[i].m_group == group &&
				p_mech->m_weapons[i].m_state != c_weaponEmpty) {
				found = TRUE;
				p_mech->m_selectedWeapon = selected = i;
				break;
			}
		}

		group++;
		group %= 10;
	}
}

// Selects the first weapon of p_group. Returns 1 if the group has one.
int32_t SelectWeaponGroup(Mech* p_mech, int32_t p_group)
{
	int32_t i;
	int32_t found;
	int32_t selected;
	int32_t current;

	i = 0;
	found = FALSE;
	if (p_group < 0 || p_group >= 3) {
		return 0;
	}

	if (p_mech->m_weaponCount <= 0) {
		return 0;
	}

	current = p_mech->m_weapons[p_mech->m_selectedWeapon].m_group;
	if (current == p_group) {
		return 1;
	}

	for (i = 0; i < 10 && !found; i++) {
		if (p_mech->m_weapons[i].m_type < 0) {
			continue;
		}

		if (p_mech->m_weapons[i].m_status != -1 && p_mech->m_weapons[i].m_group == p_group &&
			p_mech->m_weapons[i].m_state != c_weaponEmpty) {
			found = TRUE;
			p_mech->m_selectedWeapon = selected = i;
			break;
		}
	}

	return found;
}

// Returns 1 if the selected weapon is ready, 0 if not, -1 without one.
int32_t IsSelectedWeaponReady(Mech* p_mech)
{
	int32_t result;

	result = -1;
	if (p_mech->m_selectedWeapon != -1) {
		if (p_mech->m_weapons[p_mech->m_selectedWeapon].m_state == c_weaponReady) {
			result = 1;
		}
		else {
			result = 0;
		}
	}

	return result;
}

// Dumps the selected weapon's ammunition.
int32_t JettisonAmmo(Mech* p_mech)
{
	char text[40];
	WeaponSlot* slot;

	if (p_mech->m_selectedWeapon == -1) {
		return 0;
	}

	slot = &p_mech->m_weapons[p_mech->m_selectedWeapon];
	if (g_weaponDefs[slot->m_type].m_volley > 0 && slot->m_ammo > 0) {
		PlaySoundEffect(0xb3, 100, 0x40, 5, 0x50);
		sprintf(text, "Ammo for current weapon jettisonned.");
		ShowInGameMessage(text, 1, 0x16a, 0x32);
		PlaySoundEffect(0xb3, 100, 0x40, 5, 0x32);
		slot->m_ammo = 0;
		slot->m_state = c_weaponEmpty;
		return 1;
	}

	return 0;
}

// Loads the launch sounds of the weapons and the sounds of the effects.
void LoadWeaponSounds(void)
{
	int32_t i;
	WeaponDef* def;

	for (i = 0; i < 30; i++) {
		def = &g_weaponDefs[i];
		if (def->m_sound > 0) {
			PreloadResource(def->m_sound, g_resourceTypeTags[c_resTagSnds]);
		}
	}

	for (i = 0; i < 0x20; i++) {
		if (g_effectInfo[i].m_sound > 0) {
			PreloadResource(g_effectInfo[i].m_sound, g_resourceTypeTags[c_resTagSnds]);
		}
	}
}

void SetWeaponGroup(Mech* p_mech, int32_t p_index, int32_t p_group)
{
	WeaponSlot* slot;

	if (p_group >= 0 && p_group < 3) {
		slot = &p_mech->m_weapons[p_index];
		slot->m_group = p_group;
	}
}

// Moves the local player's selected weapon to p_group.
void SetSelectedWeaponGroup(int32_t p_group)
{
	Mech* mech;
	WeaponSlot* slot;

	mech = g_players[g_localPlayerId]->m_mech;
	slot = &mech->m_weapons[mech->m_selectedWeapon];
	slot->m_group = p_group;
}

void CycleLocalWeaponGroup(void)
{
	Mech* mech;

	mech = g_players[g_localPlayerId]->m_mech;
	SelectNextWeaponGroup(mech);
}

// Fires the weapons g_remoteWeaponsFired lists from p_mech.
void FireRemoteWeapons(Mech* p_mech)
{
	WeaponSlot* slot;
	WeaponDef* def;
	int32_t i;

	for (i = 0; i < 10; i++) {
		if (g_remoteWeaponsFired[i]) {
			g_remoteWeaponsFired[i] = 0;
			slot = &p_mech->m_weapons[i];
			def = &g_weaponDefs[slot->m_type];
			if (slot->m_state != c_weaponEmpty) {
				slot->m_state = c_weaponFiring;
				slot->m_volley = def->m_volley;
				slot->m_time = 0;
				if (def->m_guided && (p_mech->m_flags & 0x80)) {
					slot->m_target = p_mech->m_player->m_targetInfo.m_target & 0xff;
					slot->m_targetKind = p_mech->m_player->m_targetInfo.m_target & 0xf00;
				}
				else {
					slot->m_target = slot->m_targetKind = 0;
				}
			}
		}
	}
}

// Fires every ready weapon of the local player's selected group.
void FireWeaponGroup(void)
{
	Mech* mech;
	int32_t selected;
	int32_t first;
	WeaponDef* def;
	WeaponSlot* slot;

	mech = g_players[g_localPlayerId]->m_mech;
	selected = mech->m_selectedWeapon;
	SelectNextWeaponInGroup(mech, 0);
	first = mech->m_selectedWeapon;
	do {
		slot = &mech->m_weapons[mech->m_selectedWeapon];
		def = &g_weaponDefs[slot->m_type];
		if (slot->m_state == c_weaponReady) {
			slot->m_state = c_weaponFiring;
			slot->m_volley = def->m_volley;
			slot->m_time = 0;
			DodgeShot(slot, mech);
			if (def->m_guided && (mech->m_flags & 0x80)) {
				slot->m_target = mech->m_player->m_targetInfo.m_target & 0xff;
				slot->m_targetKind = mech->m_player->m_targetInfo.m_target & 0xf00;
			}
			else {
				slot->m_target = slot->m_targetKind = 0;
			}
		}

		SelectNextWeaponInGroup(mech, 0);
	} while (mech->m_selectedWeapon != first);

	mech->m_selectedWeapon = selected;
}

// Moves the next weapon into the selected one's group.
void AddNextWeaponToGroup(Mech* p_mech)
{
	int32_t group;
	int32_t next;

	p_mech->m_flags ^= 0x4000;
	next = p_mech->m_selectedWeapon;
	group = p_mech->m_weapons[next].m_group;
	next++;
	if (p_mech->m_weapons[next].m_status == -1) {
		next = 0;
	}

	SetWeaponGroup(p_mech, next, group);
}

// Updates the target lock of the selected weapon: with the target between its two ranges and
// within 16 degrees of the aim, it locks on after 0x16a ticks.
void UpdateWeaponLock(Mech* p_mech)
{
	int32_t dz;
	int32_t twist;
	int32_t pitch;
	int32_t yaw;
	int32_t bearing;
	int32_t inRange;
	int32_t dx;
	WeaponDef* def;
	int32_t dy;
	int32_t heading;

	p_mech->m_flags &= ~0x80;
	def = &g_weaponDefs[p_mech->m_weapons[p_mech->m_selectedWeapon].m_type];
	if (p_mech->m_selectedWeapon == -1 || !p_mech->m_player->m_targetInfo.m_target ||
		(p_mech->m_player->m_targetInfo.m_target & 0x1000) || (p_mech->m_player->m_targetInfo.m_target & 0x100) ||
		!def->m_guided) {
		p_mech->m_flags &= 0x7fff;
		p_mech->m_flags &= 0xffbf;
	}
	else {
		dx = p_mech->m_player->m_position.m_x - p_mech->m_player->m_targetInfo.m_position.m_x;
		dy = p_mech->m_player->m_position.m_y - p_mech->m_player->m_targetInfo.m_position.m_y;
		dz = p_mech->m_player->m_position.m_z - p_mech->m_player->m_targetInfo.m_position.m_z;
		if (IsWithinRadius(dx, dy, dz, def->m_shortRange) || !IsWithinRadius(dx, dy, dz, def->m_longRange)) {
			inRange = FALSE;
			yaw = 0x100001;
			pitch = 0x100001;
			p_mech->m_flags &= 0x7fff;
		}
		else {
			inRange = TRUE;
			heading = (p_mech->m_player->m_heading % 0x1680000 + 0x1680000) % 0x1680000;
			twist = p_mech->m_player->m_torsoTwist % 0x1680000;
			bearing = p_mech->m_player->m_targetInfo.m_heading - heading;
			if (bearing > 0xb40000) {
				bearing -= 0x1680000;
			}
			else if (bearing < -0xb40000) {
				bearing += 0x1680000;
			}

			yaw = bearing - twist;
			if (yaw > 0xb40000) {
				yaw -= 0x1680000;
			}
			else if (yaw < -0xb40000) {
				yaw += 0x1680000;
			}

			pitch = (p_mech->m_player->m_targetInfo.m_pitch + p_mech->m_torsoPitch.m_value) % 0x1680000;
		}

		if (inRange && abs(yaw) < 0x100000 && abs(pitch) < 0x100000) {
			p_mech->m_flags |= 0x8000;
			if (!(p_mech->m_flags & 0x40)) {
				p_mech->m_flags |= 0x40;
				p_mech->m_stateTime = 0x16a;
			}
			else if (p_mech->m_stateTime <= 0) {
				p_mech->m_stateTime = 0;
				p_mech->m_flags |= 0x80;
			}
			else {
				p_mech->m_stateTime -= g_deltaTime;
			}
		}
		else {
			p_mech->m_flags &= 0x7fff;
			if (p_mech->m_flags & 0x40) {
				p_mech->m_stateTime += g_deltaTime;
				if (p_mech->m_stateTime >= 0x16a) {
					p_mech->m_flags &= ~0x40;
				}
			}
		}
	}
}

// Casts the player's aim ray and returns the shape it hits, if any. A hit on a mech or a
// building sets the distance the weapons converge at.
Shape* UpdateAimDistance(Player* p_player)
{
	Ray ray;
	int32_t length;
	Shape* hit;
	uint16_t flags;
	int32_t collided;

	hit = NULL;
	UpdateRamp(&p_player->m_aimDistance);
	BuildAimRay(p_player, &ray);
	collided = TestSegmentCollision(&ray, &hit, p_player->m_index);
	if (collided && hit) {
		flags = hit->m_kind;
		if ((flags & 0x100) || (flags & 0x200)) {
			length = GetRayLength(&ray);
			p_player->m_aimDistance.m_value = length > 2000 ? length : 2000;
			if (p_player->m_index == g_localPlayerId) {
				g_aimedShape = hit;
			}
		}
	}

	p_player->m_aimRange.m_target = p_player->m_aimDistance.m_value;
	UpdateRamp(&p_player->m_aimRange);

	return hit;
}

int32_t GetAimRange(Player* p_player)
{
	return p_player->m_aimRange.m_value;
}

void GetMechAimDirection(Player* p_player, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	Ray ray;
	int32_t z;
	int32_t y;
	int32_t x;

	BuildAimRay(p_player, &ray);
	SetRayLength(&ray, GetAimRange(p_player));
	GetFiringPosition(p_player, &x, &y, &z);
	*p_x = ray.m_x1 - x;
	*p_y = ray.m_y1 - y;
	*p_z = ray.m_z1 - z;
}

// Builds the player's aim ray, 150000 long.
void BuildAimRay(Player* p_player, Ray* p_ray)
{
	int32_t z;
	int32_t y;
	int32_t x;
	int32_t dz;
	int32_t dy;
	int32_t dx;

	GetEyeAimDirection(p_player, &dx, &dy, &dz);
	GetObjPosition(p_player->m_eyeObj, &x, &y, &z);
	if (g_eyeHeightOffset) {
		y += *g_eyeHeightOffset;
	}

	BuildRayFromDirection(p_ray, x, y, z, dx, dy, dz, 150000);
}

// The direction the player aims in.
void GetEyeAimDirection(Player* p_player, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	Matrix matrix;
	uint32_t z;
	uint32_t y;
	Mech* mech;
	uint32_t x;

	*p_x = *p_y = 0;
	*p_z = 0x10000;
	GetObjWorldAngles(p_player->m_eyeObj, &x, &y, &z);
	mech = p_player->m_mech;
	x = mech->m_torsoPitch.m_value;
	BuildMatrix(&matrix, x, y, z, 0, 0, 0);
	RotatePoint(&matrix, p_x, p_y, p_z);
}

void GetFiringPosition(Player* p_player, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	GetObjPosition(p_player->m_firingObj, p_x, p_y, p_z);
}

// Places p_obj at the player's current hardpoint.
void PlaceAtFiringObj(Player* p_player, SceneObject* p_obj)
{
	int32_t z;
	int32_t y;
	int32_t x;

	GetFiringPosition(p_player, &x, &y, &z);
	SetObjRotationMatrix(p_obj, GetObjWorldMatrix(p_player->m_firingObj));
	SetObjPosition(p_obj, x, y, z);
}

// Places a launched shot's object at the player's hardpoint, pointing along the launch
// direction, and sets it moving. p_spread moves it up to 100 units off the line.
void SpawnLaunchFx(Player* p_player, SceneObject* p_obj, int32_t p_dx, int32_t p_dy, int32_t p_dz, int32_t p_spread)
{
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t pitch;
	int32_t yaw;
	int32_t speed;
	int32_t sideZ;
	int32_t offset;
	int32_t upX;
	int32_t upY;
	int32_t upZ;
	int32_t sideX;
	int32_t sideY;

	yaw = FixedAtan2(p_dx, p_dz);
	pitch = FixedAsin(p_dy << 13);
	SetObjRotation(p_obj, -pitch, yaw, 0, 0);
	GetFiringPosition(p_player, &x, &y, &z);
	if (p_spread) {
		offset = RandomIntBelow(200) - 100;
		sideX = -p_dz;
		sideY = 0;
		sideZ = p_dx;
		NormalizeVectorGuarded(&sideX, &sideY, &sideZ);
		x += FixedMul16(offset, sideX);
		z += FixedMul16(offset, sideZ);
		upX = FixedMul16(p_dy, sideZ);
		upY = FixedMul16(p_dz, sideX) + FixedMul16(sideZ, p_dx);
		upZ = -FixedMul16(sideX, p_dy);
		x += FixedMul16(offset, upX);
		y += FixedMul16(offset, upY);
		z += FixedMul16(offset, upZ);
	}

	SetObjPosition(p_obj, x, y, z);
	speed = p_obj->m_shape->m_radius * 2;
	p_dx = FixedMul16(p_dx, speed);
	p_dy = FixedMul16(p_dy, speed);
	p_dz = FixedMul16(p_dz, speed);
	MoveObj(p_obj, p_dx, p_dy, p_dz);
}
