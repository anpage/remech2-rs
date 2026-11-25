/* The door gamepiece (GP_MW2DOOR, player type 7): its reset, update and allocation functions. */
#include "door.h"

#include "ai.h"
#include "clock.h"
#include "fadepal.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "fixedsqrt.h"
#include "fixedtrig.h"
#include "inradius.h"
#include "mech.h"
#include "mechclass.h"
#include "missionobjective.h"
#include "navpoint.h"
#include "object.h"
#include "objective.h"
#include "players.h"
#include "playersteering.h"
#include "poolsizes.h"
#include "ramp.h"
#include "starmission.h"
#include "staticmem.h"
#include "targeting.h"
#include "types.h"
#include "weaponslot.h"

// Resets the player's mech of this class: clears its state, stands its object up at its current
// place and starts its position ramps there.
void FirstDoor(Player* p_player)
{
	Mech* mech;

	mech = p_player->m_mech;
	if (!mech) {
		return;
	}

	mech->m_selectedWeapon = 0;
	mech->m_heat = 0;
	mech->m_mobility = 0x10000;
	mech->m_lastSelectedWeapon = 0;
	mech->m_weaponCount = 0;
	mech->m_collisionTicks = 0;
	mech->m_flags = 0;
	mech->m_powerState = 0;
	mech->m_stateTime = 0;
	mech->m_deltaHeat = 0;
	mech->m_autopilot = 0;
	mech->m_velocityY = 0;
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
	StartRamp(&mech->m_speed, mech->m_player->m_position.m_x, mech->m_player->m_position.m_x, 0.3);
	StartRamp(&mech->m_throttle, mech->m_player->m_position.m_y, mech->m_player->m_position.m_y, 0.3);
	StartRamp(&mech->m_turnRate, mech->m_player->m_position.m_z, mech->m_player->m_position.m_z, 0.3);
	mech->m_player->m_torsoPitch = mech->m_player->m_torsoTwist = mech->m_player->m_torsoRoll = 0;
	mech->m_player->m_speedLevel = 0;
	mech->m_player->m_nextMotionState = -1;
	mech->m_player->m_pendingSound = -1;
	EnableObjTreeCollision(mech->m_player->m_obj);
	mech->m_player->m_headingSin = 0;
	mech->m_player->m_headingCos = 0x10000;
	InitializeAI(mech->m_player);
}

// Moves a mech in state 2 along its position ramps and places its object there, facing its
// player's heading.
void UpdateDoor(Mech* p_mech)
{
	int32_t heading;
	Mech* mech;

	if (!p_mech) {
		return;
	}

	mech = p_mech;
	if (mech->m_powerState == 2) {
		UpdateRamp(&mech->m_speed);
		UpdateRamp(&mech->m_throttle);
		UpdateRamp(&mech->m_turnRate);
		mech->m_player->m_position.m_x = mech->m_speed.m_value;
		mech->m_player->m_position.m_y = mech->m_throttle.m_value;
		mech->m_player->m_position.m_z = mech->m_turnRate.m_value;
		SetObjPosition(
			mech->m_player->m_obj,
			mech->m_player->m_position.m_x,
			mech->m_player->m_position.m_y,
			mech->m_player->m_position.m_z
		);
		SetObjRotation(
			mech->m_player->m_obj,
			mech->m_player->m_pitch,
			mech->m_player->m_heading,
			mech->m_player->m_roll,
			0
		);
		UpdateObj(mech->m_player->m_obj);
		heading = mech->m_player->m_heading;
		mech->m_player->m_headingSin = FixedSin(heading) >> 13;
		mech->m_player->m_headingCos = FixedCos(heading) >> 13;
	}
}

// Updates a mech of this class: in state 2 it heads for the nav point of its team's current
// objective, when that is a nav objective; states 0, 1 and 3 go to 2, and in state 4 it is
// destroyed after a delay.
void LateUpdateDoor(Mech* p_mech)
{
	Mech* mech;
	int32_t team;
	MissionObjective* objective;
	int32_t dz;
	int32_t nav;
	int32_t x;
	int32_t navX;
	int32_t step;
	int32_t y;
	int32_t navY;
	int32_t z;
	int32_t navZ;
	int32_t dx;
	int32_t dy;

	mech = p_mech;
	UpdateAI(mech->m_player);
	if (mech->m_powerState == 2) {
		team = mech->m_player->m_team;
		objective = &g_objectiveTable[team].m_objectives[g_currentObjective[team]];
		mech->m_player->m_targetInfo.m_target = 0;
		mech->m_player->m_steering->m_autopilot = 0;
		if (objective->m_type == 0x100 && objective->m_targetCount > 0 && (objective->m_targets[0] & 0xff00) == 0x100) {
			nav = (uint8_t) objective->m_targets[0];
			GetObjPosition(mech->m_player->m_obj, &x, &y, &z);
			navX = g_navTable[nav].m_position[0];
			navY = g_navTable[nav].m_position[1];
			navZ = g_navTable[nav].m_position[2];
			dx = navX - x;
			dy = navY - y;
			dz = navZ - z;
			step = FixedDiv16(mech->m_topSpeed * g_deltaTime, 0x697e98);
			if (IsWithinRadius(dx, dy, dz, step)) {
				NormalizeVectorGuarded(&dx, &dy, &dz);
				dx = FixedMul16(dx, step);
				dy = FixedMul16(dy, step);
				dz = FixedMul16(dz, step);
			}
			else {
				dx = navX;
				dy = navY;
				dz = navZ;
			}

			mech->m_speed.m_target = dx;
			mech->m_throttle.m_target = dy;
			mech->m_turnRate.m_target = dz;
		}
	}
	else {
		mech->m_player->m_speedLevel = 0;
		mech->m_player->m_nextMotionState = -1;
	}

	switch (mech->m_powerState) {
	case 0:
		mech->m_powerState = 2;
		mech->m_stateTime = 0;
		break;
	case 1:
		mech->m_powerState = 2;
		break;
	case 2:
		break;
	case 3:
		mech->m_powerState = 2;
		break;
	case 4:
		if (!mech->m_stateTime) {
			mech->m_stateTime = g_currentClock + 0x712;
		}

		if (mech->m_stateTime > g_currentClock) {
			EmitWreckSmoke(mech);
		}
		break;
	}
}

void ShutdownDoor(Mech* p_mech)
{
	if (!p_mech) {
		return;
	}
}

// Allocates p_player's mech, its weapons and sections, and sets them up.
int32_t CreateDoor(int32_t p_index, Player* p_player)
{
	void* buffer = NULL;
	MechSection* sections = NULL;
	WeaponSlot* weapons = NULL;
	void* extra = NULL;
	Mech* mech = NULL;
	WeaponSlot* slot;
	int32_t i;
	int32_t size;

	p_player->m_mech = NULL;
	size = GetMechAllocSize();
	buffer = StaticPoolAlloc(size, g_staticPoolTags[1]);
	if (!buffer) {
		return false;
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
		slot->m_ammo = -1;
		slot++;
	}

	return true;
}
