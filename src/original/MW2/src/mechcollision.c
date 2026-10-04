#include "mechcollision.h"

#include "approxlen.h"
#include "classtable.h"
#include "collision.h"
#include "config.h"
#include "debris.h"
#include "decomp.h"
#include "eyepoint.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "fixedtrig.h"
#include "mech.h"
#include "mechdamage.h"
#include "muldiv.h"
#include "network.h"
#include "object.h"
#include "players.h"
#include "polydraw.h"
#include "ray.h"
#include "shape.h"
#include "shapelists.h"
#include "shots.h"
#include "simmain.h"
#include "soundfx.h"
#include "types.h"

// Moves p_mech by (p_dx, p_dy, p_dz) from its player's position, into (*p_x, *p_y, *p_z), and
// stops it at the first mech (CollideWithMechs), building (CollideWithBuildings) or terrain shape it hits,
// bouncing its velocity off the surface. Returns the number of ticks it has been colliding (0:
// free); *p_hit is the shape it hit and *p_player the mech's player.
// Stack-slot permutation of the locals. The original sums dot's three products in source order;
// the build starts with the second.
// FUNCTION: MW2 0x100758a0
MechS32 MoveMechWithCollisions(
	Mech* p_mech,
	Shape** p_hit,
	Player** p_player,
	MechS32 p_dx,
	MechS32 p_dy,
	MechS32 p_dz,
	MechS32* p_x,
	MechS32* p_y,
	MechS32* p_z
)
{
	MechS32 bounce;
	MechS32 nx;
	MechS32 ny;
	MechS32 nz;
	MechS32 x0;
	MechS32 over;
	MechS32 y0;
	MechS32 hit;
	MechS32 z0;
	Shape* shape;
	MechS32 x1;
	MechS32 y1;
	MechS32 length;
	MechS32 z1;
	Ray ray;
	MechS32 lift;
	MechS32 dot;
	Ray normal;

	x0 = p_mech->m_player->m_position.m_x;
	y0 = p_mech->m_player->m_position.m_y;
	z0 = p_mech->m_player->m_position.m_z;
	x1 = p_dx + x0;
	y1 = p_dy + y0;
	z1 = p_dz + z0;
	*p_x = x1;
	*p_y = y1;
	*p_z = z1;
	shape = NULL;
	hit = FALSE;
	if (p_mech->m_powerState != 2 && p_mech->m_powerState != 4) {
		p_mech->m_collisionTicks = 0;
		return 0;
	}
	else {
		if (CollideWithMechs(p_mech, p_x, p_y, p_z, p_player)) {
			p_mech->m_player->m_collidedWith = (*p_player)->m_index;
			hit = TRUE;
			p_mech->m_collisionTicks++;
			*p_hit = NULL;
		}
		else if (CollideWithBuildings(p_mech, p_x, p_y, p_z, &shape)) {
			hit = TRUE;
			p_mech->m_collisionTicks++;
			*p_hit = shape;
		}
		else {
			if (!p_dx && !p_dy && !p_dz) {
				if (p_mech->m_collisionTicks) {
					p_mech->m_collisionTicks++;
					return p_mech->m_collisionTicks;
				}
				else {
					return 0;
				}
			}

			BuildRayFromSegment(&ray, x0, y0, z0, x1, y1, z1);
			length = GetRayLength(&ray);
			lift = ray.m_dirY;
			if (lift < 0) {
				lift = -lift;
			}

			lift = FixedMul16(p_mech->m_radius, 0x10000 - lift) + FixedMul16(p_mech->m_height, lift);
			SetRayLength(&ray, length + lift);
			if (TestSceneryCollision(&ray, &shape)) {
				hit = TRUE;
				p_mech->m_collisionTicks++;
				*p_hit = shape;
				if (!g_segmentNormalX && !g_segmentNormalY && !g_segmentNormalZ && shape) {
					BuildRayFromSegment(
						&normal,
						shape->m_centerX,
						shape->m_centerY,
						shape->m_centerZ,
						ray.m_x1,
						ray.m_y1,
						ray.m_z1
					);
					BuildRayFixed(&normal);
					g_segmentNormalX = normal.m_dirX;
					g_segmentNormalY = normal.m_dirY;
					g_segmentNormalZ = normal.m_dirZ;
				}

				dot = FixedMul16(g_segmentNormalX * 2, ray.m_dirX) + FixedMul16(g_segmentNormalY * 2, ray.m_dirY) +
					  FixedMul16(g_segmentNormalZ * 2, ray.m_dirZ);
				nx = ray.m_dirX - FixedMul16(dot, g_segmentNormalX);
				ny = ray.m_dirY - FixedMul16(dot, g_segmentNormalY);
				nz = ray.m_dirZ - FixedMul16(dot, g_segmentNormalZ);
				bounce =
					-ApproximateVectorLength(p_mech->m_newVelocityX, p_mech->m_newVelocityY, p_mech->m_newVelocityZ) >>
					2;
				p_mech->m_velocityX = FixedMul16(nx, bounce);
				p_mech->m_velocityY = FixedMul16(ny, bounce);
				p_mech->m_velocityZ = FixedMul16(nz, bounce);
				SetRayLength(&ray, GetRayLength(&ray) - lift);
				over = length - GetRayLength(&ray);
				if (over > 0) {
					nx = FixedMul16(nx, over) >> 2;
					ny = FixedMul16(ny, over) >> 2;
					nz = FixedMul16(nz, over) >> 2;
					*p_x = ray.m_x1 + nx;
					*p_y = ray.m_y1 + ny;
					*p_z = ray.m_z1 + nz;
				}
				else {
					*p_x = ray.m_x1;
					*p_y = ray.m_y1;
					*p_z = ray.m_z1;
				}
			}
		}
	}

	if (!hit) {
		p_mech->m_collisionTicks = 0;
		*p_hit = NULL;
	}

	return p_mech->m_collisionTicks;
}

// Pushes the point (*p_x, *p_y, *p_z) of p_mech out of the first other mech it overlaps, and
// gives p_mech a velocity away from it. A mech pushing into one that pushed into it first (whose
// m_collidedWith is p_mech's player) always collides. A mech in state 4 is knocked down
// (KnockMechOver) instead. Returns 1 on a collision, with *p_hit the other mech's player.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10075d7b
MechS32 CollideWithMechs(Mech* p_mech, MechS32* p_x, MechS32* p_y, MechS32* p_z, Player** p_hit)
{
	MechS32 radius;
	MechS32 otherRadius;
	Player* player;
	MechS32 reach;
	MechS32 dist;
	MechS32 result;
	MechS32 i;
	Vector3* pos;
	MechS32 dx;
	MechS32 dy;
	Mech* mech;
	MechS32 dz;
	MechS32 speed;
	MechS32 id;

	id = p_mech->m_player->m_index;
	radius = p_mech->m_radius;
	result = 0;
	for (i = 0; i < g_playerCount; i++) {
		player = g_players[i];
		if (!player || player->m_index == id) {
			continue;
		}

		mech = player->m_mech;
		if (!mech) {
			continue;
		}

		if (mech->m_flags & 0x100) {
			continue;
		}

		if (mech->m_player->m_flags & 0x4000) {
			continue;
		}

		if (player->m_collidedWith == id && i < id) {
			if (mech->m_powerState == 4) {
				KnockMechOver(mech);
			}
			else {
				result = 1;
				*p_hit = player;
				pos = &player->m_position;
				dx = *p_x - pos->m_x;
				dy = *p_y - pos->m_y;
				dz = *p_z - pos->m_z;
				dist = ApproximateVectorLength(dx, dy, dz);
				if (dist == 0) {
					g_segmentNormalX = g_segmentNormalY = 0;
					g_segmentNormalZ = 0x10000;
				}
				else {
					g_segmentNormalX = FixedDiv16(dx, dist);
					g_segmentNormalY = FixedDiv16(dy, dist);
					g_segmentNormalZ = FixedDiv16(dz, dist);
				}

				reach = mech->m_radius + radius;
				*p_x = pos->m_x + FixedMul16(g_segmentNormalX, reach);
				*p_y = pos->m_y + FixedMul16(g_segmentNormalY, reach);
				*p_z = pos->m_z + FixedMul16(g_segmentNormalZ, reach);
				p_mech->m_newVelocityZ = mech->m_newVelocityZ;
				speed = ApproximateVectorLength(
					p_mech->m_newVelocityX - mech->m_newVelocityX,
					p_mech->m_newVelocityY - mech->m_newVelocityY,
					p_mech->m_newVelocityZ
				);
				if (speed > 0x20000) {
					speed >>= 1;
				}
				else {
					speed = 0x10000;
				}

				p_mech->m_velocityX = FixedMul16(g_segmentNormalX, speed);
				p_mech->m_velocityY = FixedMul16(g_segmentNormalY, speed);
				p_mech->m_velocityZ = FixedMul16(g_segmentNormalZ, speed);
			}

			break;
		}
		else {
			otherRadius = mech->m_radius;
			pos = &player->m_position;
			dx = *p_x - pos->m_x;
			dy = *p_y - pos->m_y;
			dz = *p_z - pos->m_z;
			reach = otherRadius + radius;
			dist = ApproximateVectorLength(dx, dy, dz);
			if (dist < reach) {
				if (mech->m_powerState == 4) {
					KnockMechOver(mech);
				}
				else {
					result = 1;
					*p_hit = player;
					if (dist == 0) {
						g_segmentNormalX = g_segmentNormalY = 0;
						g_segmentNormalZ = 0x10000;
					}
					else {
						g_segmentNormalX = FixedDiv16(dx, dist);
						g_segmentNormalY = FixedDiv16(dy, dist);
						g_segmentNormalZ = FixedDiv16(dz, dist);
					}

					*p_x = pos->m_x + FixedMul16(g_segmentNormalX, reach);
					*p_y = pos->m_y + FixedMul16(g_segmentNormalY, reach);
					*p_z = pos->m_z + FixedMul16(g_segmentNormalZ, reach);
					p_mech->m_newVelocityZ = mech->m_newVelocityZ;
					speed = ApproximateVectorLength(
						p_mech->m_newVelocityX - mech->m_newVelocityX,
						p_mech->m_newVelocityY - mech->m_newVelocityY,
						p_mech->m_newVelocityZ
					);
					if (speed > 0x20000) {
						speed >>= 1;
					}
					else {
						speed = 0x10000;
					}

					p_mech->m_velocityX = FixedMul16(g_segmentNormalX, speed);
					p_mech->m_velocityY = FixedMul16(g_segmentNormalY, speed);
					p_mech->m_velocityZ = FixedMul16(g_segmentNormalZ, speed);
				}

				break;
			}
		}
	}

	return result;
}

// Pushes the point (*p_x, *p_y, *p_z) of p_mech out of the first building (a shape of type 6)
// it overlaps and bounces its velocity away. A shape of kind 0x50 isn't solid: it only gets a
// push and a sound. Returns 1 on a collision, with *p_hit the shape.
// Stack-slot permutation of the locals. Operand order: dist < reach loads reach first in the
// original.
// FUNCTION: MW2 0x10076295
MechS32 CollideWithBuildings(Mech* p_mech, MechS32* p_x, MechS32* p_y, MechS32* p_z, Shape** p_hit)
{
	MechS32 dz;
	Shape* root;
	MechS32 reach;
	MechS32 dist;
	Shape* shape;
	MechS32 bounce;
	MechS32 dx;
	MechS32 dy;
	SceneObject* obj;
	MechS32 index;
	MechS32 x;
	MechS32 y;
	MechS32 z;

	root = g_sceneShapes;
	if (!root) {
		return 0;
	}

	*p_hit = NULL;
	for (shape = root->m_nextCollider; shape; shape = shape->m_nextCollider) {
		if (shape->m_collisionType != 6 || shape->m_kind & 0x100) {
			continue;
		}

		dx = *p_x - shape->m_centerX;
		dy = *p_y - shape->m_centerY;
		dz = *p_z - shape->m_centerZ;
		reach = p_mech->m_radius + shape->m_radius;
		dist = ApproximateVectorLength(dx, dy, dz);
		if (dist < reach) {
			if ((shape->m_kind & 0xf0) == 0x50) {
				obj = shape->m_object;
				dist = ApproximateVectorLength(p_mech->m_velocityX, p_mech->m_velocityY, p_mech->m_velocityZ);
				if (dist > 0) {
					index = AddDebrisPiece(obj, 1);
					if (index >= 0) {
						PushDebrisPiece(
							index,
							p_mech->m_velocityX * 2,
							p_mech->m_velocityY * 2,
							p_mech->m_velocityZ * 2
						);
						GetObjPosition(obj, &x, &y, &z);
						x -= g_eyepoint->m_x;
						y -= g_eyepoint->m_y;
						z -= g_eyepoint->m_z;
						PlaySoundAt(x, y, z, 0xb5, g_inCockpitView);
					}
				}

				return 0;
			}

			if (dist == 0) {
				g_segmentNormalX = g_segmentNormalY = 0;
				g_segmentNormalZ = 0x10000;
			}
			else {
				g_segmentNormalX = FixedDiv16(dx, dist);
				g_segmentNormalY = FixedDiv16(dy, dist);
				g_segmentNormalZ = FixedDiv16(dz, dist);
			}

			*p_x = shape->m_centerX + FixedMul16(g_segmentNormalX, reach);
			*p_y = shape->m_centerY + FixedMul16(g_segmentNormalY, reach);
			*p_z = shape->m_centerZ + FixedMul16(g_segmentNormalZ, reach);
			bounce =
				-ApproximateVectorLength(p_mech->m_newVelocityX, p_mech->m_newVelocityY, p_mech->m_newVelocityZ) >> 2;
			if (bounce > 8) {
				bounce = -bounce >> 1;
			}
			else {
				bounce = -4;
			}

			p_mech->m_velocityX = FixedMul16(g_segmentNormalX, bounce);
			p_mech->m_velocityY = FixedMul16(g_segmentNormalY, bounce);
			p_mech->m_velocityZ = FixedMul16(g_segmentNormalZ, bounce);
			*p_hit = shape;
			return 1;
		}
	}

	return 0;
}

// Damages p_mech and p_other by their collision, when the difficulty has collision damage and
// they met faster than 200000.
// The only diff is a stack-slot permutation of speed and damage.
// FUNCTION: MW2 0x100765f8
void DamageMechsInCollision(Mech* p_mech, Mech* p_other)
{
	MechS32 speed;
	MechS32 damage;

	if (!g_difficulty->m_collisionDamage) {
		return;
	}

	speed = ApproximateVectorLength(
		p_mech->m_newVelocityX - p_other->m_newVelocityX,
		p_mech->m_newVelocityY - p_other->m_newVelocityY,
		p_mech->m_newVelocityZ - p_other->m_newVelocityZ
	);
	if (speed > 200000) {
		damage = FixedDiv16(speed - 200000, 1300000) * 3;
		ApplyCollisionDamage(p_mech, p_other, damage);
	}
}

// Damages p_mech by a collision along the last collision normal: from below (the normal points
// down) the legs, scaled by the other mech's mass, from above both side torsos at half, and
// otherwise the section facing the normal. p_other is NULL for a building or the ground.
// Stack-slot permutation: attacker and angle. Operand order: damage < p_damage loads p_damage
// first in the original.
// FUNCTION: MW2 0x1007669e
void ApplyCollisionDamage(Mech* p_mech, Mech* p_other, MechS32 p_damage)
{
	MechS32 attacker;
	MechS32 damage;
	MechS32 angle;

	if (p_other) {
		attacker = p_other->m_player->m_index;
	}
	else {
		attacker = -2;
	}

	if (g_segmentNormalY < -0xddb4) {
		if (!p_other) {
			damage = p_damage;
		}
		else {
			damage = MulDiv64(p_damage << 2, p_other->m_tons, p_mech->m_tons > 0 ? p_mech->m_tons : 1);
			if (damage < p_damage) {
				damage = p_damage;
			}
		}

		ApplyDamageToMech(attacker, p_mech, damage, 1);
	}
	else if (g_segmentNormalY > 0xb505) {
		p_damage >>= 1;
		ApplyDamageToMech(attacker, p_mech, p_damage, 7);
		ApplyDamageToMech(attacker, p_mech, p_damage, 8);
	}
	else {
		angle = FixedAtan2(-g_segmentNormalX, -g_segmentNormalZ) - p_mech->m_player->m_heading -
				p_mech->m_player->m_torsoTwist;
		if (angle < -0xb40000) {
			angle += 0x1680000;
		}
		else if (angle > 0xb40000) {
			angle -= 0x1680000;
		}

		if (angle > 0) {
			if (angle < 0x2d0000 || angle > 0x870000) {
				ApplyDamageToMech(attacker, p_mech, p_damage, 2);
			}
			else {
				ApplyDamageToMech(attacker, p_mech, p_damage, 5);
			}
		}
		else if (angle > -0x2d0000 || angle < -0x870000) {
			ApplyDamageToMech(attacker, p_mech, p_damage, 4);
		}
		else {
			ApplyDamageToMech(attacker, p_mech, p_damage, 6);
		}
	}
}

// Damages p_mech for hitting p_shape (NULL: the ground) faster than 200000, and the shape too
// when it can be damaged. In a network game, a shape of kind 0xb0 destroys both legs and itself.
// Stack-slot permutation: damage and speed.
// FUNCTION: MW2 0x100768a8
void DamageMechHittingShape(Mech* p_mech, Shape* p_shape)
{
	MechS32 damage;
	MechS32 speed;

	if (g_isNetworkGame && p_shape && (p_shape->m_kind & 0xf0) == 0xb0) {
		ApplyDamageToMech(-2, p_mech, 0x320000, 7);
		ApplyDamageToMech(-2, p_mech, 0x320000, 8);
		SpawnEffect(
			p_mech->m_player->m_index,
			0xd,
			p_shape->m_centerX,
			p_shape->m_centerY,
			p_shape->m_centerZ,
			p_shape->m_centerX,
			p_shape->m_centerY,
			p_shape->m_centerZ
		);
		KillGameThing(p_shape->m_owner);
	}
	else {
		speed = ApproximateVectorLength(p_mech->m_newVelocityX, p_mech->m_newVelocityY, p_mech->m_newVelocityZ);
		if (speed > 200000) {
			damage = FixedDiv16(speed - 200000, 1300000) * 3;
			if (g_difficulty->m_collisionDamage) {
				ApplyCollisionDamage(p_mech, NULL, damage);
			}

			if (p_shape && p_shape->m_kind & 0x200) {
				DamageGameThing(
					p_mech->m_player->m_index,
					p_shape,
					damage >> 16,
					p_shape->m_centerX,
					p_shape->m_centerY,
					p_shape->m_centerZ
				);
			}
		}
	}
}

// Knocks p_mech over: plays its fall (unless another machine of a network game controls it),
// marks it fallen and plays the crash sound.
// FUNCTION: MW2 0x10076a23
void KnockMechOver(Mech* p_mech)
{
	MechS32 x;
	MechS32 y;
	MechS32 z;

	if (!g_netRole || p_mech->m_player->m_index == g_localPlayerId) {
		BlowOffObjTree(p_mech->m_player->m_obj, ReleaseObjShape, 999);
		p_mech->m_flags |= 0x100;
	}
	else {
		return;
	}

	x = p_mech->m_player->m_position.m_x - g_eyepoint->m_x;
	y = p_mech->m_player->m_position.m_y - g_eyepoint->m_y;
	z = p_mech->m_player->m_position.m_z - g_eyepoint->m_z;
	PlaySoundAt(x, y, z, 0xb5, g_inCockpitView);
}
