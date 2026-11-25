#ifndef AIWEAPONS_H
#define AIWEAPONS_H

#include "types.h"

struct Mech;
struct Player;

// The functions and globals of aiweapons.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t RunAIWeapons(struct Player* p_player, int32_t p_heading);
	int32_t DecideAIFire(struct Player* p_player);

#ifdef __cplusplus
}
#endif

#endif // AIWEAPONS_H
