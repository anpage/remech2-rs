#ifndef MECHCOLLISION_H
#define MECHCOLLISION_H

#include "types.h"

struct Mech;
struct Player;
struct Shape;

// The functions and globals of mechcollision.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t MoveMechWithCollisions(
		struct Mech* p_mech,
		struct Shape** p_hit,
		struct Player** p_player,
		int32_t p_dx,
		int32_t p_dy,
		int32_t p_dz,
		int32_t* p_x,
		int32_t* p_y,
		int32_t* p_z
	);
	int32_t CollideWithMechs(struct Mech* p_mech, int32_t* p_x, int32_t* p_y, int32_t* p_z, struct Player** p_hit);
	int32_t CollideWithBuildings(struct Mech* p_mech, int32_t* p_x, int32_t* p_y, int32_t* p_z, struct Shape** p_hit);
	void DamageMechsInCollision(struct Mech* p_mech, struct Mech* p_other);
	void ApplyCollisionDamage(struct Mech* p_mech, struct Mech* p_other, int32_t p_damage);
	void DamageMechHittingShape(struct Mech* p_mech, struct Shape* p_shape);
	void KnockMechOver(struct Mech* p_mech);

#ifdef __cplusplus
}
#endif

#endif // MECHCOLLISION_H
