/* The artillery gamepiece (GP_MW2ARTILLERY, player type 3): its reset, update and allocation
   functions. */
#include "artillery.h"

#include "ai.h"
#include "clock.h"
#include "decomp.h"
#include "fadepal.h"
#include "mech.h"
#include "mechclass.h"
#include "mechdamage.h"
#include "object.h"
#include "objective.h"
#include "players.h"
#include "playersteering.h"
#include "poolsizes.h"
#include "ramp.h"
#include "random.h"
#include "resource.h"
#include "staticmem.h"
#include "targeting.h"
#include "types.h"
#include "weapons.h"
#include "weaponslot.h"

// Resets the player's mech of this class: its torso objects and ramps, state and weapons, stands
// its object up where it is and clears the player's steering.
// FUNCTION: MW2 0x10059fc0
void FirstArtillery(Player* p_player)
{
	Mech* mech;

	mech = p_player->m_mech;
	if (!mech) {
		return;
	}

	mech->m_torsoObj = NextThingRecordObject();
	mech->m_pitchObj = NextThingRecordObject();
	StartRamp(&mech->m_torsoTwist, 0, 0, 0.8);
	StartRamp(&mech->m_torsoPitch, 0, 0, 0.8);
	mech->m_selectedWeapon = 0;
	mech->m_heat = 0;
	mech->m_lastSelectedWeapon = 0;
	mech->m_weaponCount = 10;
	mech->m_collisionTicks = 0;
	mech->m_flags = 0x2000;
	mech->m_powerState = 0;
	mech->m_stateTime = 0;
	mech->m_deltaHeat = 0;
	mech->m_unk0xb4 = 0;
	mech->m_autopilot = 0;
	mech->m_velocityY = 0;
	mech->m_unk0xf0 = 0;
	MoveObj(mech->m_player->m_obj, 0, mech->m_height, 0);
	UpdateObj(mech->m_player->m_obj);
	GetObjWorldAngles(
		mech->m_player->m_obj,
		&mech->m_player->m_pitch,
		&mech->m_player->m_heading,
		&mech->m_player->m_roll
	);
	GetObjPosition(
		mech->m_player->m_obj,
		&mech->m_player->m_position.m_x,
		&mech->m_player->m_position.m_y,
		&mech->m_player->m_position.m_z
	);
	mech->m_player->m_torsoPitch = mech->m_player->m_torsoTwist = mech->m_player->m_torsoRoll = 0;
	mech->m_player->m_speedLevel = 0;
	mech->m_player->m_nextMotionState = -1;
	EnableObjTreeCollision(mech->m_player->m_obj);
	mech->m_player->m_steering->m_autopilot = 0;
	mech->m_player->m_steering->m_throttle = 0;
	mech->m_player->m_steering->m_advanceNav = 0;
	InitializeAI(mech->m_player);
	mech->m_topSpeed = 0;
}

// Updates the mech's torso: clears the tick's heat, steps the twist and pitch ramps and turns the
// torso objects (m_pitchObj by the pitch, m_torsoObj by the player's view angles with the twist).
// FUNCTION: MW2 0x1005a203
void UpdateArtillery(Mech* p_mech)
{
	Mech* mech;

	if (!p_mech) {
		return;
	}

	mech = p_mech;
	mech->m_deltaHeat = 0;
	UpdateRamp(&mech->m_torsoTwist);
	UpdateRamp(&mech->m_torsoPitch);
	if (mech->m_pitchObj) {
		SetObjRotation(mech->m_pitchObj, mech->m_torsoPitch.m_value, 0, 0, 0);
	}

	if (mech->m_torsoObj) {
		mech->m_player->m_torsoPitch = 0;
		mech->m_player->m_torsoTwist = mech->m_torsoTwist.m_value;
		mech->m_player->m_torsoRoll = 0;
		SetObjRotation(
			mech->m_torsoObj,
			mech->m_player->m_torsoPitch,
			mech->m_player->m_torsoTwist,
			mech->m_player->m_torsoRoll,
			0
		);
	}

	UpdateObj(mech->m_player->m_obj);
}

// Updates the mech for the tick. A running mech (state 2) fires its weapons, drops a target it
// can no longer claim, aims its torso from the steering (turning freely with a twist limit of a
// full turn, otherwise within the limit) and stops on overheating; otherwise the steering is
// cleared. Then the heat, and the power states: 0 starts powering up, 1 runs once the power-up
// time has passed, 3 starts over and 4 is destroyed.
// Stack-slot permutation: mech, twist and delta.
// FUNCTION: MW2 0x1005a2ea
void LateUpdateArtillery(Mech* p_mech)
{
	Mech* mech;
	MechS32 twist;
	MechS32 delta;

	mech = p_mech;
	UpdateAI(mech->m_player);
	if (mech->m_powerState == 2) {
		UpdateWeaponFireState(mech);
		if (mech->m_player->m_aiMode != 2) {
			if (mech->m_player->m_targetInfo.m_target && !(mech->m_player->m_targetInfo.m_target & 0x1000) &&
				!UpdateTarget(mech->m_player)) {
				mech->m_player->m_targetInfo.m_target = 0;
			}
		}
		else {
			mech->m_player->m_targetInfo.m_target = 0;
		}

		UpdateWeaponLock(mech);
		mech->m_autopilot = 0;
		mech->m_player->m_steering->m_autopilot = 0;
		if ((mech->m_flags & 4) && !(mech->m_flags & 8)) {
			mech->m_player->m_steering->m_throttle = 0;
		}

		twist = mech->m_player->m_steering->m_torsoPan;
		if (mech->m_maxTorsoTwist >= 0x1680000) {
			delta = twist - mech->m_torsoTwist.m_value;
			while (delta > 0xb40000) {
				delta -= 0x1680000;
			}

			while (delta < -0xb40000) {
				delta += 0x1680000;
			}

			mech->m_torsoTwist.m_target = twist;
			mech->m_torsoTwist.m_value = twist - delta;
		}
		else if (mech->m_maxTorsoTwist < mech->m_torsoTwist.m_target) {
			mech->m_torsoTwist.m_target = mech->m_maxTorsoTwist;
		}
		else if (-mech->m_maxTorsoTwist > mech->m_torsoTwist.m_target) {
			mech->m_torsoTwist.m_target = -mech->m_maxTorsoTwist;
		}

		mech->m_torsoPitch.m_target = mech->m_player->m_steering->m_torsoTilt;
	}
	else {
		mech->m_torsoTwist.m_target = 0;
		mech->m_torsoPitch.m_target = 0;
		mech->m_player->m_steering->m_throttle = 0;
		mech->m_player->m_steering->m_torsoPan = 0;
		mech->m_player->m_steering->m_torsoTilt = 0;
	}

	CalculateHeat(mech);
	switch (mech->m_powerState) {
	case 0:
		mech->m_powerState = 1;
		mech->m_stateTime = g_currentClock + RandomIntBelow(0x16a) + 0x43e;
		break;
	case 1:
		if (mech->m_stateTime < g_currentClock) {
			mech->m_powerState = 2;
		}
		break;
	case 2:
		break;
	case 3:
		mech->m_powerState = 0;
		break;
	case 4:
		if (!mech->m_stateTime) {
			mech->m_stateTime = g_currentClock + 0x712;
			FUN_1001cdd1();
		}

		if (mech->m_stateTime > g_currentClock) {
			EmitWreckSmoke(mech);
		}
		break;
	}
}

// FUNCTION: MW2 0x1005a61d
void ShutdownArtillery(Mech* p_mech)
{
	if (!p_mech) {
		return;
	}
}

// Allocates p_player's mech, its weapons and sections, and sets them up.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1005a637
MechS32 CreateArtillery(MechS32 p_index, Player* p_player)
{
	void* buffer = NULL;
	MechSection* sections = NULL;
	WeaponSlot* weapons = NULL;
	void* extra = NULL;
	Mech* mech = NULL;
	WeaponSlot* slot;
	MechS32 i;
	MechS32 size;

	p_player->m_mech = NULL;
	size = GetMechAllocSize();
	buffer = StaticPoolAlloc(size, g_staticPoolTags[1]);
	if (!buffer) {
		return FALSE;
	}

	mech = buffer;
	weapons = (WeaponSlot*) (mech + 1);
	sections = (MechSection*) (weapons + 10);
	extra = sections + 8;
	mech->m_weapons = weapons;
	mech->m_sections = sections;
	mech->m_ammoBins = extra;
	mech->m_player = p_player;
	for (i = 0; i < 8; i++) {
		mech->m_objects[i] = NULL;
	}

	p_player->m_mech = mech;
	p_player->m_mechSize = 0x10e;
	slot = mech->m_weapons;
	for (i = 0; i < 10; i++) {
		slot->m_status = -1;
		slot->m_type = -1;
		slot->m_state = c_weaponEmpty;
		slot->m_time = 0;
		slot->m_unk0x14 = 0;
		slot->m_ammo = -1;
		slot++;
	}

	return TRUE;
}
