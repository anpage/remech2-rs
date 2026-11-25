#ifndef ARTILLERY_H
#define ARTILLERY_H

#include "types.h"

struct Mech;
struct Player;

// The functions of artillery.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void FirstArtillery(struct Player* p_player);
	void UpdateArtillery(struct Mech* p_mech);
	void LateUpdateArtillery(struct Mech* p_mech);
	void ShutdownArtillery(struct Mech* p_mech);
	int32_t CreateArtillery(int32_t p_index, struct Player* p_player);

#ifdef __cplusplus
}
#endif

#endif // ARTILLERY_H
