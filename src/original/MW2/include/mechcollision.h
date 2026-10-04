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

	MechS32 MoveMechWithCollisions(
		struct Mech* p_mech,
		struct Shape** p_hit,
		struct Player** p_player,
		MechS32 p_dx,
		MechS32 p_dy,
		MechS32 p_dz,
		MechS32* p_x,
		MechS32* p_y,
		MechS32* p_z
	);
	MechS32 CollideWithMechs(struct Mech* p_mech, MechS32* p_x, MechS32* p_y, MechS32* p_z, struct Player** p_hit);
	MechS32 CollideWithBuildings(struct Mech* p_mech, MechS32* p_x, MechS32* p_y, MechS32* p_z, struct Shape** p_hit);
	void DamageMechsInCollision(struct Mech* p_mech, struct Mech* p_other);
	void ApplyCollisionDamage(struct Mech* p_mech, struct Mech* p_other, MechS32 p_damage);
	void DamageMechHittingShape(struct Mech* p_mech, struct Shape* p_shape);
	void KnockMechOver(struct Mech* p_mech);

#ifdef __cplusplus
}
#endif

#endif // MECHCOLLISION_H
