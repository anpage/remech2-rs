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

	int32_t CanSeeTarget(struct Player* p_player, int32_t p_ahead);
	int32_t IsGroundLevelToward(struct Player* p_player, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t GetTargetRangeBand(struct Player* p_player, struct WeaponDef* p_weapon);

#ifdef __cplusplus
}
#endif

#endif // LINEOFSIGHT_H
