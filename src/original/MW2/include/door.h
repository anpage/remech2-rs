#ifndef DOOR_H
#define DOOR_H

#include "decomp.h"
#include "types.h"

struct Mech;
struct Player;

// The functions of door.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void FirstDoor(struct Player* p_player);
	void UpdateDoor(struct Mech* p_mech);
	void LateUpdateDoor(struct Mech* p_mech);
	void ShutdownDoor(struct Mech* p_mech);
	MechS32 CreateDoor(MechS32 p_index, struct Player* p_player);

#ifdef __cplusplus
}
#endif

#endif // DOOR_H
