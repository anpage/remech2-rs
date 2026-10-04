#ifndef LINEOFSIGHT_H
#define LINEOFSIGHT_H

#include "types.h"

struct Player;
struct WeaponDef;

// The functions and globals of lineofsight.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 CanSeeTarget(struct Player* p_player, MechS32 p_ahead);
	MechS32 IsGroundLevelToward(struct Player* p_player, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 GetTargetRangeBand(struct Player* p_player, struct WeaponDef* p_weapon);

#ifdef __cplusplus
}
#endif

#endif // LINEOFSIGHT_H
