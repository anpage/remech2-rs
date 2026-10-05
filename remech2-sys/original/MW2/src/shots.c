#include "shots.h"

#include "ai.h"
#include "animation.h"
#include "approxlen.h"
#include "careerrecord.h"
#include "clock.h"
#include "collision.h"
#include "config.h"
#include "debris.h"
#include "decomp.h"
#include "effect.h"
#include "effectinfo.h"
#include "eyepoint.h"
#include "fadepal.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "fixedmul29.h"
#include "fixedsqrt.h"
#include "fixedtrig.h"
#include "gamething.h"
#include "geocache.h"
#include "integrate.h"
#include "mech.h"
#include "mechdamage.h"
#include "network.h"
#include "object.h"
#include "objective.h"
#include "palette.h"
#include "players.h"
#include "polydraw.h"
#include "random.h"
#include "ray.h"
#include "render.h"
#include "shape.h"
#include "shapelists.h"
#include "simmain.h"
#include "soundfx.h"
#include "team.h"
#include "types.h"
#include "view.h"
#include "weapondata.h"
#include "weapons.h"

#include <string.h>

DECOMP_SIZE_ASSERT(Shot, 0x50)
DECOMP_SIZE_ASSERT(Effect, 0x28)
DECOMP_SIZE_ASSERT(EffectInfo, 0x1c)
DECOMP_SIZE_ASSERT(CareerRecord, 0xd6)

// GLOBAL: MW2 0x100ad440
MechS32 g_effectCameraActive = 0;

// GLOBAL: MW2 0x100ad444
MechS32 g_effectCameraEffect = -1;

// GLOBAL: MW2 0x100ad448
MechS32 g_lastLocalMissile = -1;

// GLOBAL: MW2 0x100ad44c
MechS32 g_trackedShot = -1;

// GLOBAL: MW2 0x100ad450
Player* g_launchEffectPlayer = NULL;

// GLOBAL: MW2 0x100ad454
MechS32 g_effectCameraEnabled = 1;

// The player whose shot hit something last.
// GLOBAL: MW2 0x100ad458
MechS32 g_lastHitShooter = -1;

// GLOBAL: MW2 0x100ad45c
MechS32 g_nukeTimeLeft = 0;

// GLOBAL: MW2 0x100bee50
MechS32 g_nukeMaxRadius;

// The view that follows a tracked shot: its position, heading and three more angles.
// GLOBAL: MW2 0x100bee58
MechS32 g_trackedShotView[7];

// GLOBAL: MW2 0x100bee74
MechS32 g_nukeRadius;

// GLOBAL: MW2 0x100bee78
Vector3 g_nukePosition;

// The eyepoint's settings while an effect has the camera.
// GLOBAL: MW2 0x100c75f0
MechS32 g_savedLightX;

// GLOBAL: MW2 0x100c75f4
MechS32 g_savedLightY;

// GLOBAL: MW2 0x100c75f8
MechS32 g_savedLightZ;

// GLOBAL: MW2 0x100c75fc
MechS32 g_savedDirectionalLight;

// GLOBAL: MW2 0x100c7600
MechS32 g_savedAmbientLight;

// GLOBAL: MW2 0x100c7604
MechS32 g_savedDistanceFade;

// GLOBAL: MW2 0x100e9250
CareerRecord g_careerRecord;

// GLOBAL: MW2 0x1017bac0
Shot g_shots[0xaf];

// GLOBAL: MW2 0x1017f170
Effect g_effects[0x100];

// Stack-slot permutation: i, shot and effect.
// FUNCTION: MW2 0x1006a230
void FirstShots(void)
{
	MechS32 i;
	Shot* shot;
	Effect* effect;

	for (i = 0; i < 0xaf; i++) {
		shot = &g_shots[i];
		shot->m_type = -1;
		shot->m_object = NULL;
		ResetShotSlot(i);
	}

	for (i = 0; i < 0x100; i++) {
		effect = &g_effects[i];
		effect->m_object = NULL;
		effect->m_animation = -1;
		effect->m_type = -1;
		effect->m_owner = -1;
		ResetEffectSlot(i);
	}

	g_trackedShotView[0] = g_trackedShotView[1] = g_trackedShotView[2] = 0;
	g_trackedShotView[3] = g_trackedShotView[5] = g_trackedShotView[4] = 0;
	g_trackedShotView[6] = 0;
	memset(&g_careerRecord, 0, sizeof(g_careerRecord));
}

// FUNCTION: MW2 0x1006a349
void ResetEffectSlot(MechS32 p_index)
{
	Effect* effect;

	effect = &g_effects[p_index];
	effect->m_position[0] = effect->m_position[1] = effect->m_position[2] = 0;
	effect->m_timeLeft = 0;
	effect->m_active = 0;
	effect->m_hasCamera = 0;
	effect->m_owner = -1;
}

// FUNCTION: MW2 0x1006a3b1
void ResetShotSlot(MechS32 p_index)
{
	Shot* shot;

	shot = &g_shots[p_index];
	shot->m_velocity[0] = shot->m_velocity[1] = shot->m_velocity[2] = 0;
	shot->m_steering[0] = shot->m_steering[1] = shot->m_steering[2] = 0;
	shot->m_age = shot->m_lifetime = 0;
	shot->m_swayPhase = 0;
	shot->m_impact = -1;
	shot->m_target = 0;
	shot->m_targetKind = 0;
	shot->m_flags = 0;
	shot->m_unk0x3c = 0;
	shot->m_damage = shot->m_heat = 0;
	shot->m_tracked = 0;
}

// Ages every shot in flight by g_deltaTime and moves it on.
// Stack-slot permutation: i and shot.
// UpdateAllShots on the Rust side (src/sim/shots.rs) wraps it, skipping frames of no time.
// FUNCTION: MW2 0x1006a486
void UpdateAllShotsC(void)
{
	MechS32 i;
	Shot* shot;

	for (i = 0; i < 0xaf; i++) {
		shot = &g_shots[i];
		if (shot->m_flags && shot->m_object) {
			shot->m_age += g_deltaTime;
			shot->m_lifetime -= g_deltaTime;
			switch (shot->m_unk0x3c) {
			case 0:
				UpdateShot(i);
				break;
			default:
				break;
			}
		}
	}
}

// Moves the shot on by g_deltaTime and tests the segment it covered for a hit: a mech, a game
// thing or the ground. A hit detonates the shot, just short of the point of impact; otherwise
// it detonates when its lifetime runs out, or steers on toward its target.
// Stack-slot permutation of the locals; the y < groundY comparison has its operands the
// other way around.
// FUNCTION: MW2 0x1006a533
void UpdateShot(MechS32 p_index)
{
	MechS32 hitResult;
	MechS32 velZ;
	MechU16 surface;
	Shape* hit;
	Shot* shot;
	MechS32 victim;
	MechS32 backX;
	MechS32 x;
	MechS32 backY;
	MechS32 x0;
	MechS32 damageFlags;
	MechS32 y;
	MechS32 backZ;
	MechS32 y0;
	MechS32 z;
	MechS32 z0;
	MechS32 dt;
	MechS32 hitX;
	MechS32 hitY;
	Ray ray;
	MechS32 hitZ;
	MechS32 velX;
	MechS32 bearing;
	MechS32 velY;
	MechS32 groundY;

	hit = NULL;
	dt = g_deltaTime;
	damageFlags = 0;
	shot = &g_shots[p_index];
	GetObjPosition(shot->m_object, &x0, &y0, &z0);
	velX = shot->m_velocity[0];
	velY = shot->m_velocity[1];
	velZ = shot->m_velocity[2];
	x = x0;
	y = y0;
	z = z0;
	IntegrateMidpoint(&x, &velX, shot->m_steering[0], dt);
	IntegrateMidpoint(&y, &velY, shot->m_steering[1], dt);
	IntegrateMidpoint(&z, &velZ, shot->m_steering[2], dt);
	BuildRayFromSegment(&ray, x0, y0, z0, x, y, z);
	if (ApproximateVectorLength(x - x0, y - y0, z - z0) > 3000) {
		BuildRayFloat(&ray);
	}
	else {
		BuildRayFixed(&ray);
	}

	hitResult = 0;
	surface = 0;
	hit = NULL;
	if (shot->m_flags & c_shotProximityFuse) {
		hitResult = 1;
		if (shot->m_targetKind == c_shotTargetPlayer) {
			hit = GetObjShape(g_players[shot->m_target]->m_obj);
		}
		else if (shot->m_targetKind == c_shotTargetGameThing) {
			hit = GetStaticObjectShape(g_gameThings[shot->m_target].m_staticObject);
		}

		hitX = backX = x0;
		hitY = backY = y0;
		hitZ = backZ = z0;
	}
	else {
		hitResult = TestSegmentCollision(&ray, &hit, shot->m_shooter);
		if (hitResult) {
			hitX = ray.m_x1;
			hitY = ray.m_y1;
			hitZ = ray.m_z1;
			backX = hitX - FixedMul16(100, ray.m_dirX);
			backY = hitY - FixedMul16(100, ray.m_dirY);
			backZ = hitZ - FixedMul16(100, ray.m_dirZ);
		}
	}

	if (hitResult && hit) {
		surface = hit->m_kind;
		g_lastHitShooter = shot->m_shooter;
		if (surface & 0x100) {
			victim = hit->m_owner;
			shot->m_impact |= c_impactMech;
			if (shot->m_type == 7) {
				shot->m_impact |= 0x1000;
			}

			bearing = g_players[victim]->m_heading - FixedAtan2(shot->m_velocity[0], shot->m_velocity[2]);
			bearing %= 0x1680000;
			if (bearing > 0xb40000) {
				bearing -= 0x1680000;
			}
			else if (bearing < -0xb40000) {
				bearing += 0x1680000;
			}

			if (bearing < 0x5a0000 && bearing > -0x5a0000) {
				damageFlags |= 0x8000;
			}

			if (g_lastHitShooter == g_localPlayerId) {
				switch (GetPlayerSide(victim)) {
				case 0:
					g_careerRecord.m_friendlyHits++;
					break;
				case 1:
					g_careerRecord.m_hits++;
					break;
				case 2:
					g_careerRecord.m_neutralHits++;
					break;
				}
			}

			if (g_lastHitShooter >= 0 && g_players[g_localPlayerId]->m_team == g_players[g_lastHitShooter]->m_team) {
				switch (GetPlayerSide(victim)) {
				case 0:
					g_careerRecord.m_teamFriendlyHits++;
					break;
				case 1:
					g_careerRecord.m_teamHits++;
					break;
				case 2:
					g_careerRecord.m_teamNeutralHits++;
					break;
				}
			}

			if (victim == g_localPlayerId) {
				g_careerRecord.m_hitsTaken++;
			}

			if (g_players[g_localPlayerId]->m_team == g_players[victim]->m_team) {
				g_careerRecord.m_teamHitsTaken++;
			}

			if (!g_netRole || victim == g_localPlayerId) {
				g_players[victim]->m_mech->m_deltaHeat += shot->m_heat;
				ApplyDamageToMech(
					shot->m_shooter,
					g_players[victim]->m_mech,
					shot->m_damage << 16,
					hit->m_partId | damageFlags
				);
			}

			if (victim == g_localPlayerId) {
				shot->m_impact |= 0x4000;
				if (hit->m_partId == 1 || hit->m_partId == 3 || hit->m_partId == 2 || hit->m_partId == 4) {
					shot->m_impact |= 0x2000;
				}

				PlayPlayerHitFeedback(velX, velY, velZ);
			}

			RecordAttack(shot->m_shooter, victim);
		}
		else if (surface & 0x200) {
			shot->m_impact |= c_impactThing;
			DamageGameThing(shot->m_shooter, hit, shot->m_damage, hitX, hitY, hitZ);
		}
		else if ((surface & 0xf0) == 0x50) {
			DamageChunk(hit->m_owner, shot->m_damage << 16);
		}
		else if (surface & 0x400) {
			shot->m_impact |= c_impactThing;
		}
		else {
			shot->m_impact |= c_impactGround;
		}
	}
	else if (y < 0) {
		groundY = GetTerrainHeight(x, y, z);
		if (y < groundY) {
			hitResult = 1;
			shot->m_impact |= c_impactGround;
			ClipRayToGround(&ray, groundY);
			hitX = backX = ray.m_x1;
			hitY = backY = ray.m_y1;
			hitZ = backZ = ray.m_z1;
		}
	}

	if (hitResult) {
		DetonateShot(p_index, 1, hitX, hitY, hitZ, backX, backY, backZ);
	}
	else if (shot->m_lifetime <= 0) {
		DetonateShot(p_index, 0, x, y, z, x, y, z);
	}
	else {
		if (shot->m_targetKind && shot->m_age > 0) {
			GuideMissileToTarget(shot, x, y, z);
		}

		SetObjPosition(shot->m_object, x, y, z);
		UpdateObj(shot->m_object);
		shot->m_velocity[0] = velX;
		shot->m_velocity[1] = velY;
		shot->m_velocity[2] = velZ;
		if (shot->m_tracked) {
			g_trackedShot = p_index;
			g_trackedShotView[0] = x;
			g_trackedShotView[1] = y;
			g_trackedShotView[2] = z;
			g_trackedShotView[3] = FixedAtan2(velX, velZ);
			g_trackedShotView[4] = 0;
			g_trackedShotView[5] = 0;
			g_trackedShotView[6] = 1;
		}
	}
}

// Sways (p_x, p_y, p_z) sideways and up and down around the shot's line of flight, by the
// phase in m_swayPhase, and advances the phase.
// FUNCTION: MW2 0x1006ad78
void SwayShot(Shot* p_shot, MechS32* p_x, MechS32* p_y, MechS32* p_z)
{
	MechS32 sideZ;
	MechS32 sideY;
	MechS32 sideX;

	sideX = -p_shot->m_velocity[2];
	sideY = 0;
	sideZ = p_shot->m_velocity[0];
	NormalizeVectorGuarded(&sideX, &sideY, &sideZ);
	*p_x += FixedMul29(sideX, FixedSin(p_shot->m_swayPhase)) >> 9;
	*p_z += FixedMul29(sideZ, FixedSin(p_shot->m_swayPhase)) >> 9;
	*p_y += FixedMul29(sideZ, FixedCos(p_shot->m_swayPhase)) >> 10;
	p_shot->m_swayPhase += g_deltaTime * 0x3fa57;
	p_shot->m_swayPhase %= 0x1680000;
}

// Steers a guided missile at (p_x, p_y, p_z) toward its target, and arms its proximity fuse
// within 100 units of it.
// Stack-slot permutation: every local but distance.
// GuideMissileToTarget on the Rust side (src/sim/shots.rs) wraps it, arming the fuse at the rate
// of a 45 FPS sim.
// FUNCTION: MW2 0x1006ae5a
void GuideMissileToTargetC(Shot* p_shot, MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	MechS32 targetY;
	MechS32 targetZ;
	Player* player;
	MechS32 dy;
	MechS32 dx;
	MechS32 dz;
	GameThing* thing;
	MechS32 distance;
	MechS32 vx;
	MechS32 vy;
	MechS32 vz;
	MechS32 targetX;

	if (p_shot->m_target < 0) {
		return;
	}

	switch (p_shot->m_targetKind) {
	case c_shotTargetPlayer:
		player = g_players[p_shot->m_target];
		if ((player->m_flags & 2) || (player->m_flags & 4)) {
			p_shot->m_steering[0] = p_shot->m_steering[1] = p_shot->m_steering[2] = 0;
			p_shot->m_targetKind = 0;
			return;
		}

		targetX = player->m_position.m_x;
		targetY = player->m_position.m_y;
		targetZ = player->m_position.m_z;
		break;
	case c_shotTargetGameThing:
		thing = &g_gameThings[p_shot->m_target];
		if (thing->m_flags & 4) {
			p_shot->m_steering[0] = p_shot->m_steering[1] = p_shot->m_steering[2] = 0;
			p_shot->m_targetKind = 0;
			return;
		}

		GetStaticObjectPosition(thing->m_staticObject, &targetX, &targetY, &targetZ);
		break;
	default:
		return;
	}

	dx = targetX - p_x;
	dy = targetY - p_y;
	dz = targetZ - p_z;
	distance = ApproximateVectorLength(dx, dy, dz);
	if (distance <= 100) {
		p_shot->m_flags |= c_shotProximityFuse;
	}

	dx = FixedDiv16(dx, distance);
	dy = FixedDiv16(dy, distance);
	dz = FixedDiv16(dz, distance);
	vx = p_shot->m_velocity[0];
	vy = p_shot->m_velocity[1];
	vz = p_shot->m_velocity[2];
	NormalizeVectorGuarded(&vx, &vy, &vz);
	p_shot->m_steering[0] = dx - vx;
	p_shot->m_steering[1] = dy - vy;
	p_shot->m_steering[2] = dz - vz;
	targetY = FixedAtan2(vx, vz);
	targetX = -FixedAsin(vy << 13);
	SetObjRotation(p_shot->m_object, targetX, targetY, 0, 0);
}

// Removes the shot from the world and frees its slot; p_explode spawns the effect its impact
// calls for at (p_x, p_y, p_z), with the camera looking from (p_camX, p_camY, p_camZ).
// FUNCTION: MW2 0x1006b0b4
void DetonateShot(
	MechS32 p_index,
	MechS32 p_explode,
	MechS32 p_x,
	MechS32 p_y,
	MechS32 p_z,
	MechS32 p_camX,
	MechS32 p_camY,
	MechS32 p_camZ
)
{
	Shot* shot;
	MechS32 impact;

	shot = &g_shots[p_index];
	if (shot->m_tracked) {
		g_trackedShot = -1;
	}

	HideObjTree(shot->m_object);
	DisableObjTreeCollision(shot->m_object);
	impact = shot->m_impact;
	ResetShotSlot(p_index);
	if (p_explode) {
		SpawnEffect(shot->m_shooter, impact, p_x, p_y, p_z, p_camX, p_camY, p_camZ);
	}
}

// FUNCTION: MW2 0x1006b152
void SpawnEffect(
	MechS32 p_owner,
	MechS32 p_type,
	MechS32 p_x,
	MechS32 p_y,
	MechS32 p_z,
	MechS32 p_camX,
	MechS32 p_camY,
	MechS32 p_camZ
)
{
	SpawnEffectEx(p_owner, p_type, p_x, p_y, p_z, p_camX, p_camY, p_camZ, 0, 0, 0);
}

// FUNCTION: MW2 0x1006b18b
void SpawnRotatedEffect(
	MechS32 p_type,
	MechS32 p_rotX,
	MechS32 p_rotY,
	MechS32 p_rotZ,
	MechS32 p_x,
	MechS32 p_y,
	MechS32 p_z
)
{
	SpawnEffectEx(-2, p_type, p_x, p_y, p_z, p_x, p_y, p_z, p_rotX, p_rotY, p_rotZ);
}

// FUNCTION: MW2 0x1006b1c8
void SpawnLaunchEffect(MechS32 p_type, Player* p_player)
{
	g_launchEffectPlayer = p_player;
	SpawnEffectEx(-2, p_type, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

// Spawns an effect. The low byte of p_type is the effect; the high byte says what a shot hit
// (c_impactMech etc.), which picks a variant of it, and 0x4000 keeps its sound at full volume.
// The effect takes a free slot of its type (none if three of them already burn within 500
// units), and the effect camera may follow it from (p_camX, p_camY, p_camZ).
// Stack-slot permutation of the locals; one distance comparison has its operands the other
// way around.
// FUNCTION: MW2 0x1006b1fb
void SpawnEffectEx(
	MechS32 p_owner,
	MechS32 p_type,
	MechS32 p_x,
	MechS32 p_y,
	MechS32 p_z,
	MechS32 p_camX,
	MechS32 p_camY,
	MechS32 p_camZ,
	MechS32 p_rotX,
	MechS32 p_rotY,
	MechS32 p_rotZ
)
{
	MechS32 pieces;
	MechS32 distance;
	MechS32 dy;
	MechS32 flags;
	MechS32 dx;
	MechS32 chainType;
	MechS32 chain;
	MechS32 cameraX;
	MechS32 dz;
	MechS32 quiet;
	MechS32 current;
	EffectInfo* info;
	MechS32 slot;
	MechS32 i;
	MechS32 cameraZ;
	Effect* effect;
	MechS32 take;
	MechS32 closer;
	MechS32 count;
	MechS32 volume;

	effect = NULL;
	info = NULL;
	quiet = 0;
	chain = 0;
	chainType = -1;
	if (p_type < 0) {
		return;
	}

	flags = p_type & 0xff00;
	p_type &= 0xff;
	if (p_type >= 0x20) {
		return;
	}

	if (flags & 0x4000) {
		quiet = 1;
	}

	switch (p_type) {
	case 5:
		if (flags & c_impactMech) {
			p_x = p_camX;
			p_z = p_camZ;
			p_type = 8;
		}
		else if (flags & c_impactGround) {
			p_type = 9;
		}
		else if (flags & 0x2000) {
			p_type = 0x12;
		}
		break;
	case 3:
		if (flags & c_impactGround) {
			p_type = 0xc;
		}
		else if (flags & 0x2000) {
			p_type = 0x11;
		}
		else if (flags & c_impactThing) {
			p_type = 0x13;
		}
		chain = 1;
		chainType = 0x10b;
		break;
	case 4:
		if (flags & c_impactGround) {
			p_type = 0xc;
		}
		else if (flags & 0x2000) {
			p_type = 0x11;
		}
		else if (flags & c_impactThing) {
			p_type = 0x14;
		}
		chain = 1;
		chainType = 0x10b;
		break;
	case 0:
		if (flags & 0x2000) {
			p_type = 0xe;
		}
		else if (flags & c_impactGround) {
			p_type = 9;
		}
		break;
	case 1:
		if (flags & 0x2000) {
			p_type = 0xf;
		}
		else if (flags & c_impactGround) {
			p_type = 9;
		}
		break;
	case 2:
		if (flags & 0x2000) {
			p_type = 0x10;
		}
		else if (flags & c_impactGround) {
			p_type = 9;
		}
		break;
	case 6:
		if (flags & c_impactGround) {
			p_type = 0x15;
		}
		break;
	case 0xb:
		if (flags & 0x1000) {
			pieces = 0x10;
		}
		else if (flags & c_impactMech) {
			pieces = 4;
		}
		else if (flags & c_impactThing) {
			pieces = 4;
		}
		else {
			pieces = 8;
		}

		ScatterDebris(p_x, p_y, p_z, pieces);
		return;
	default:
		break;
	}

	info = &g_effectInfo[p_type];
	dx = g_eyepoint->m_x - p_x;
	dy = g_eyepoint->m_y - p_y;
	dz = g_eyepoint->m_z - p_z;
	if (info->m_needsObject || info->m_camera) {
		count = 0;
		slot = -1;
		for (i = 0; i < 0x100; i++) {
			effect = &g_effects[i];
			if (effect->m_type == p_type) {
				if (effect->m_active) {
					if (effect->m_type < 0x17 && ApproximateVectorLength(
													 effect->m_position[0] - p_x,
													 effect->m_position[1] - p_y,
													 effect->m_position[2] - p_z
												 ) < 500) {
						count++;
						if (g_lodQuality != 1 || count > 2) {
							return;
						}
					}
				}
				else if (slot < 0 && (!info->m_needsObject || effect->m_object)) {
					slot = i;
				}
			}
		}

		if (slot < 0) {
			return;
		}

		effect = &g_effects[slot];
		effect->m_active = 1;
		effect->m_position[0] = p_x;
		effect->m_position[1] = p_y;
		effect->m_position[2] = p_z;
		effect->m_timeLeft = info->m_duration + g_deltaTime;
		effect->m_owner = p_owner;
		if (effect->m_object) {
			if (p_type >= 0x17 && g_launchEffectPlayer) {
				PlaceAtFiringObj(g_launchEffectPlayer, effect->m_object);
				g_launchEffectPlayer = NULL;
			}
			else {
				SetObjRotation(effect->m_object, p_rotX, p_rotY, p_rotZ, 0);
				SetObjPosition(effect->m_object, p_x, p_y, p_z);
			}

			ShowObjTree(effect->m_object);
			DisableObjTreeCollision(effect->m_object);
			UpdateObj(effect->m_object);
			if (effect->m_animation != -1) {
				SetAnimFrame(effect->m_animation, 0);
				SetAnimMode(effect->m_animation, 2);
			}
		}

		if (info->m_camera && g_effectCameraEnabled) {
			closer = 0;
			take = 1;
			if (g_effectCameraActive) {
				distance = dx * dx + dz * dz;
				cameraX = g_eyepoint->m_lightX - g_eyepoint->m_x;
				cameraZ = g_eyepoint->m_lightZ - g_eyepoint->m_z;
				current = cameraX * cameraX + cameraZ * cameraZ;
				if (current > distance) {
					closer = 1;
				}
				else {
					take = 0;
				}
			}

			if (take) {
				if (!closer) {
					g_effectCameraActive = 1;
					g_lightFollowsObject = 0;
					g_savedLightX = g_eyepoint->m_lightX;
					g_savedLightY = g_eyepoint->m_lightY;
					g_savedLightZ = g_eyepoint->m_lightZ;
					g_savedDirectionalLight = g_eyepoint->m_directionalLight;
					g_savedAmbientLight = g_eyepoint->m_ambientLight;
					g_eyepoint->m_ambientLight -= 10;
					if (g_eyepoint->m_ambientLight > 0xff || g_eyepoint->m_ambientLight < 0) {
						g_eyepoint->m_ambientLight = 0x40;
					}

					g_savedDistanceFade = g_renderSettings.m_distanceFade;
					g_eyepoint->m_directionalLight = 0;
					g_renderSettings.m_distanceFade = 0;
				}

				g_effectCameraEffect = slot;
				effect->m_hasCamera = 1;
				g_eyepoint->m_lightX = p_camX;
				g_eyepoint->m_lightY = p_camY;
				g_eyepoint->m_lightZ = p_camZ;
				if (info->m_flash > -1) {
					StartPaletteFlash(info->m_flash, info->m_duration, 1);
				}
			}
		}
	}

	if (chain) {
		SpawnEffect(p_owner, chainType, p_x, p_y, p_z, p_camX, p_camY, p_camZ);
	}

	if (info->m_sound > 0 && (info->m_soundChance == -1 || RandomIntBelow(100) < info->m_soundChance)) {
		if (!GetViewMode() && !quiet) {
			volume = 1;
		}
		else {
			volume = 0;
		}

		PlaySoundAt(dx, dy, dz, info->m_sound, volume);
	}
}

// Counts down the effects, lets the damaging ones hurt what is near them, and frees them when
// their time runs out, giving the camera back if one had it.
// Stack-slot permutation of the locals; the i == g_effectCameraEffect comparison has its
// operands the other way around.
// FUNCTION: MW2 0x1006b99a
void UpdateEffects(void)
{
	MechS32 release;
	MechS32 i;
	MechS32 restore;
	Effect* effect;
	MechS32 x;
	MechS32 y;
	MechS32 z;
	MechS32 radius;

	UpdateNuke();
	if (g_difficulty->m_heatTracking) {
		HeatMechsNearFires();
	}

	for (i = 0; i < 0x100; i++) {
		effect = &g_effects[i];
		if (effect->m_active) {
			if (g_effectInfo[effect->m_type].m_damages) {
				x = effect->m_position[0];
				y = effect->m_position[1];
				z = effect->m_position[2];
				radius = effect->m_object->m_shape->m_radius;
				if (g_difficulty->m_splashDamage) {
					DamageMechsInRadius(effect->m_owner, x, y, z, radius, 0x100);
				}

				DamageThingsInRadius(effect->m_owner, x, y, z, radius, 0x100);
				DamageChunksInRadius(x, y, z, radius, 0x100);
			}

			effect->m_timeLeft -= g_deltaTime;
			if (effect->m_timeLeft <= 0) {
				release = 1;
				restore = 0;
				if (effect->m_hasCamera && g_effectCameraActive && i == g_effectCameraEffect) {
					if (!GetPaletteFadeSteps()) {
						restore = 1;
					}
					else {
						release = 0;
					}
				}

				if (release) {
					if (restore) {
						g_lightFollowsObject = 1;
						g_eyepoint->m_lightX = g_savedLightX;
						g_eyepoint->m_lightY = g_savedLightY;
						g_eyepoint->m_lightZ = g_savedLightZ;
						g_eyepoint->m_directionalLight = g_savedDirectionalLight;
						g_eyepoint->m_ambientLight = g_savedAmbientLight;
						g_renderSettings.m_distanceFade = g_savedDistanceFade;
						effect->m_hasCamera = 0;
						g_effectCameraActive = 0;
						g_effectCameraEffect = -1;
					}

					if (effect->m_object) {
						HideObjTree(effect->m_object);
					}

					if (effect->m_animation > 0) {
						SetAnimMode(effect->m_animation, 0);
						SetAnimFrame(effect->m_animation, 0);
					}

					ResetEffectSlot(i);
				}
			}
		}
	}
}

// Damages every mech within p_radius (plus its own radius) of (p_x, p_y, p_z) by p_rate per
// tick in each of its sections, twice as much when its power is off.
// Stack-slot permutation of the locals; the i != g_localPlayerId comparison and the
// p_rate * g_deltaTime product have their operands the other way around.
// FUNCTION: MW2 0x1006bc13
void DamageMechsInRadius(MechS32 p_owner, MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_radius, MechS32 p_rate)
{
	Player* player;
	MechS32 reach;
	Mech* mech;
	MechS32 i;
	MechS32 dx;
	MechS32 dy;
	MechS32 dz;
	MechS32 damage;

	i = g_playerCount;
	while (i--) {
		if (g_netRole && i != g_localPlayerId) {
			continue;
		}

		player = g_players[i];
		mech = player->m_mech;
		if (mech->m_flags & 0x100) {
			continue;
		}

		dx = player->m_position.m_x - p_x;
		dy = player->m_position.m_y - p_y;
		dz = player->m_position.m_z - p_z;
		reach = mech->m_radius + p_radius;
		if (ApproximateVectorLength(dx, dy, dz) < reach) {
			damage = p_rate * g_deltaTime;
			if (mech->m_powerState == 4) {
				damage <<= 1;
			}

			ApplyDamageToMech(p_owner, mech, damage >> 2, 1);
			ApplyDamageToMech(p_owner, mech, damage, 2);
			ApplyDamageToMech(p_owner, mech, damage, 3);
			ApplyDamageToMech(p_owner, mech, damage, 4);
			ApplyDamageToMech(p_owner, mech, damage, 5);
			ApplyDamageToMech(p_owner, mech, damage, 6);
			ApplyDamageToMech(p_owner, mech, damage, 7);
			ApplyDamageToMech(p_owner, mech, damage, 8);
		}
	}
}

// Damages every game thing whose shape comes within p_radius of (p_x, p_y, p_z).
// Stack-slot permutation of the locals; the distance < reach comparison has its operands the
// other way around.
// FUNCTION: MW2 0x1006bdb4
void DamageThingsInRadius(MechS32 p_owner, MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_radius, MechS32 p_rate)
{
	MechS32 distance;
	GameThing* thing;
	MechS32 i;
	MechS32 dx;
	MechS32 dy;
	MechS32 dz;
	Shape* shape;
	MechS32 reach;
	MechS32 radius;
	MechS32 x;
	MechS32 y;
	MechS32 z;

	i = g_gameThingCount;
	while (i--) {
		thing = &g_gameThings[i];
		if (thing->m_flags & 4) {
			continue;
		}

		shape = GetStaticObjectShape(thing->m_staticObject);
		if (!shape) {
			continue;
		}

		radius = GetShapeBounds(shape, &x, &y, &z);
		dx = x - p_x;
		dy = y - p_y;
		dz = z - p_z;
		reach = radius + p_radius;
		distance = ApproximateVectorLength(dx, dy, dz);
		if (distance < reach) {
			DamageGameThing(p_owner, shape, FixedMul16(p_rate, g_deltaTime), x, y, z);
		}
	}
}

// FUNCTION: MW2 0x1006beb5
MechS32* GetTrackedShotView(void)
{
	if (g_trackedShot != -1 && g_shots[g_trackedShot].m_tracked) {
		return g_trackedShotView;
	}
	else {
		g_trackedShot = -1;
		return NULL;
	}
}

// Starts tracking the local player's last shot, if it is still in flight.
// FUNCTION: MW2 0x1006bf05
MechS32 TrackLastShot(void)
{
	if (g_lastLocalMissile > 0 && g_shots[g_lastLocalMissile].m_flags &&
		g_shots[g_lastLocalMissile].m_shooter == g_localPlayerId) {
		g_trackedShot = g_lastLocalMissile;
		g_shots[g_lastLocalMissile].m_tracked = 1;
		return 1;
	}

	g_trackedShot = -1;
	return 0;
}

// Counts the destruction of game thing p_index for the last shooter's side and team, and
// removes it.
// The shooter and local player comparisons have their operands the other way around.
// FUNCTION: MW2 0x1006bf8c
void KillGameThing(MechU32 p_index)
{
	GameThing* thing;

	thing = &g_gameThings[p_index];
	if (g_aimedShape && (g_aimedShape->m_kind & 0x200) && g_aimedShape->m_owner == p_index) {
		g_aimedShape = NULL;
	}

	if (thing->m_flags & 4) {
		return;
	}

	if (g_lastHitShooter >= 0) {
		if (g_lastHitShooter == g_localPlayerId) {
			switch (GetThingSide(p_index)) {
			case 0:
				g_careerRecord.m_directFriendlyThingKills++;
				break;
			case 2:
				g_careerRecord.m_directNeutralThingKills++;
				break;
			case 1:
				g_careerRecord.m_directThingKills++;
				break;
			}
		}

		if (g_players[g_localPlayerId]->m_team == g_players[g_lastHitShooter]->m_team) {
			switch (GetThingSide(p_index)) {
			case 0:
				g_careerRecord.m_teamFriendlyThingKills++;
				break;
			case 2:
				g_careerRecord.m_teamNeutralThingKills++;
				break;
			case 1:
				g_careerRecord.m_teamThingKills++;
				break;
			}
		}
	}

	if (thing->m_flags & 0x40) {
		FUN_1001cdd1();
	}

	DestroyThingObject(thing);
}

// Takes p_damage off the hit points of the game thing p_shape belongs to; when they run out,
// blows it up and counts it destroyed.
// Stack-slot permutation: index and thing.
// FUNCTION: MW2 0x1006c11c
void DamageGameThing(MechS32 p_owner, Shape* p_shape, MechS32 p_damage, MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	MechU32 index;
	GameThing* thing;

	if (!p_shape) {
		return;
	}

	index = p_shape->m_owner;
	thing = &g_gameThings[index];
	if (thing->m_flags & 4) {
		return;
	}

	if (!thing->m_hitPoints) {
		return;
	}

	thing->m_hitPoints -= p_damage;
	if (thing->m_hitPoints <= 0) {
		thing->m_hitPoints = 0;
		if ((p_shape->m_kind & 0xf0) == 0xb0) {
			SpawnEffect(p_owner, 0xd, p_x, p_y, p_z, p_x, p_y, p_z);
		}
		else {
			SpawnEffect(p_owner, 3, p_x, p_y, p_z, p_x, p_y, p_z);
			SpawnEffect(p_owner, 0x20b, p_x, p_y, p_z, p_x, p_y, p_z);
		}

		KillGameThing(index);
	}
}

// Shows up to p_count free effects of type 0xb (all when 0) at (p_x, p_y, p_z) as debris.
// Stack-slot permutation of the locals; the shown < p_count comparison has its operands the
// other way around.
// FUNCTION: MW2 0x1006c237
void ScatterDebris(MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_count)
{
	MechS32 shown;
	MechS32 piece;
	Effect* effect;
	MechS32 i;
	SceneObject* object;

	shown = 0;
	if (!p_count) {
		p_count = 0x100;
	}

	for (i = 0; i < 0x100 && shown < p_count; i++) {
		effect = &g_effects[i];
		if (!effect->m_active && effect->m_type == 0xb && effect->m_object) {
			object = effect->m_object;
			piece = AddDebrisPiece(object, 1);
			if (piece != -1) {
				effect->m_active = 1;
				effect->m_timeLeft = g_effectInfo[0xb].m_duration;
				SetObjPosition(object, p_x, p_y, p_z);
				ThrowDebrisPiece(piece);
				ShowObjTree(object);
				UpdateObj(object);
				shown++;
			}
		}
	}
}

// FUNCTION: MW2 0x1006c345
void SaveCareerRecord(void)
{
	WriteCareerRecordFile("MW2CAR.CFG", &g_careerRecord);
}

// Heats up the mechs near burning game things (shapes of type 0x10), the more the closer.
// Stack-slot permutation of the locals; the i != g_localPlayerId and distance < reach
// comparisons have their operands the other way around.
// FUNCTION: MW2 0x1006c362
void HeatMechsNearFires(void)
{
	Shape* root;
	MechS32 heat;
	Player* player;
	MechS32 reach;
	MechS32 radius;
	MechS32 distance;
	MechS32 x;
	MechS32 y;
	MechS32 z;
	Mech* mech;
	MechS32 i;
	MechS32 dx;
	MechS32 dy;
	Shape* shape;
	MechS32 dz;

	root = g_sceneShapes;
	if (!root) {
		return;
	}

	for (shape = root->m_next; shape; shape = shape->m_next) {
		if ((shape->m_kind & 0xf0) != 0x10) {
			continue;
		}

		radius = GetShapeBounds(shape, &x, &y, &z) * 2;
		i = g_playerCount;
		while (i--) {
			if (g_netRole && i != g_localPlayerId) {
				continue;
			}

			player = g_players[i];
			if ((player->m_flags & 2) || (player->m_flags & 4)) {
				continue;
			}

			mech = player->m_mech;
			dx = player->m_position.m_x - x;
			dy = player->m_position.m_y - y;
			dz = player->m_position.m_z - z;
			reach = mech->m_radius + radius;
			distance = ApproximateVectorLength(dx, dy, dz);
			if (distance < reach) {
				heat = radius;
				heat = heat - heat * distance / reach >> 3;
				mech->m_deltaHeat += heat * g_deltaTime;
			}
		}
	}
}

// Sets off a nuke at the player's position: the palette flash, the effect, and the blast that
// grows over the next ticks (UpdateNuke).
// Stack-slot permutation: duration, effect and shape.
// FUNCTION: MW2 0x1006c4e2
void StartNuke(Player* p_player)
{
	MechS32 i;
	MechS32 duration;
	Effect* effect;
	Shape* shape;

	duration = g_effectInfo[0x16].m_duration * 2;
	g_nukeMaxRadius = 400000;
	i = 0x100;
	while (i--) {
		effect = &g_effects[i];
		if (effect->m_type == 0x16 && effect->m_object && (shape = effect->m_object->m_shape)) {
			g_nukeMaxRadius = shape->m_radius * 100;
			break;
		}
	}

	StartPaletteFade(0x12, duration, 2);
	g_nukePosition = p_player->m_position;
	g_nukeTimeLeft = duration;
	g_nukeRadius = 0;
	SpawnEffect(
		-2,
		0x16,
		g_nukePosition.m_x,
		g_nukePosition.m_y,
		g_nukePosition.m_z,
		g_nukePosition.m_x,
		g_nukePosition.m_y,
		g_nukePosition.m_z
	);
}

// Grows the nuke's blast and damages everything in it.
// The g_nukeRadius > g_nukeMaxRadius comparison has its operands the other way around;
// defining the two globals the other way around didn't flip it.
// FUNCTION: MW2 0x1006c5e7
void UpdateNuke(void)
{
	if (g_nukeTimeLeft <= 0) {
		return;
	}

	g_nukeRadius += g_deltaTime * 200;
	if (g_nukeRadius > g_nukeMaxRadius) {
		g_nukeRadius = g_nukeMaxRadius;
	}

	DamageMechsInRadius(-2, g_nukePosition.m_x, g_nukePosition.m_y, g_nukePosition.m_z, g_nukeRadius, 0x1000);
	DamageThingsInRadius(-2, g_nukePosition.m_x, g_nukePosition.m_y, g_nukePosition.m_z, g_nukeRadius, 0x1000);
	DamageChunksInRadius(g_nukePosition.m_x, g_nukePosition.m_y, g_nukePosition.m_z, g_nukeRadius, 0x1000);
	g_nukeTimeLeft -= g_deltaTime;
}
