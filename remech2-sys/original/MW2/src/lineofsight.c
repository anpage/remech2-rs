#include "lineofsight.h"

#include "collision.h"
#include "fixedmul.h"
#include "fixedtrig.h"
#include "players.h"
#include "ray.h"
#include "shape.h"
#include "targeting.h"
#include "types.h"
#include "weapondef.h"

// Tests whether the player can see its target: nothing but the target itself lies on the line
// to it. With p_ahead, a target behind the player (between 90 and 270 degrees off the view)
// is never seen.
int32_t CanSeeTarget(Player* p_player, int32_t p_ahead)
{
	Ray ray;
	Shape* hit;
	int32_t bearing;
	uint32_t target;
	uint16_t surface;

	bearing = (p_player->m_heading + p_player->m_torsoTwist - p_player->m_targetInfo.m_heading + 0x1680000) % 0x1680000;
	if (p_ahead && bearing < 0x10e0000 && bearing > 0x5a0000) {
		return FALSE;
	}

	BuildRayFromSegment(
		&ray,
		p_player->m_position.m_x,
		p_player->m_position.m_y,
		p_player->m_position.m_z,
		p_player->m_targetInfo.m_position.m_x,
		p_player->m_targetInfo.m_position.m_y,
		p_player->m_targetInfo.m_position.m_z
	);
	BuildRayFixed(&ray);
	hit = NULL;
	if (TestSegmentCollision(&ray, &hit, p_player->m_index) && hit) {
		surface = hit->m_kind;
		target = p_player->m_targetInfo.m_target;
		if ((surface & 0x100) && (target & 0x200)) {
			return hit->m_owner == (target & 0xff);
		}
		else if ((surface & 0x200) && (target & 0x400)) {
			return hit->m_owner == (target & 0xff);
		}
		else {
			return FALSE;
		}
	}

	return TRUE;
}

// Walks the ground from (p_x, p_y, p_z) toward the player in steps of 1000 units, and tests
// that the terrain stays within 1000 units of height 0 all the way.
int32_t IsGroundLevelToward(Player* p_player, int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t steps;
	int32_t level;
	int32_t pitch;
	uint32_t distance;
	int32_t unk0x10;
	int32_t height;
	int32_t i;
	int32_t yaw;
	int32_t stepX;
	int32_t stepY;
	int32_t stepZ;

	level = 0;
	GetBearingAndRange(
		p_player->m_position.m_x - p_x,
		p_player->m_position.m_y - p_y,
		p_player->m_position.m_z - p_z,
		&yaw,
		&unk0x10,
		&distance,
		&pitch
	);
	steps = (int32_t) distance / 1000;
	stepX = FixedMul16(FixedCos(yaw) * 1000, FixedCos(pitch));
	stepZ = FixedMul16(FixedSin(yaw) * 1000, FixedCos(pitch));
	stepY = FixedSin(pitch) * 1000;
	for (i = 0; i < steps; i++) {
		p_x -= stepX;
		p_y -= stepY;
		p_z -= stepZ;
		height = GetTerrainHeight(p_x, p_y, p_z);
		if (height - 1000 > level || height + 1000 < level) {
			return 0;
		}
	}

	return 1;
}

// Returns which of the weapon's range bands the player's target is in: 3 within the short
// range, 2 within the long one, 1 right at it and 0 beyond it.
int32_t GetTargetRangeBand(Player* p_player, WeaponDef* p_weapon)
{
	if (p_player->m_targetInfo.m_distance < p_weapon->m_shortRange) {
		return 3;
	}

	if (p_player->m_targetInfo.m_distance < p_weapon->m_longRange) {
		return 2;
	}

	if (p_player->m_targetInfo.m_distance > p_weapon->m_longRange) {
		return 0;
	}

	return 1;
}
