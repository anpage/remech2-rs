#ifndef GPANIM_H
#define GPANIM_H

#include "players.h"
#include "types.h"

// The functions and globals of gpanim.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_motionSounds[4][4];
	extern int32_t g_lastMotionSound[60];

	void FirstGPAnim(void);
	void StartMotion(Player* p_player);
	void StopMotion(Player* p_player);
	int32_t* GetEyepointOffset(Player* p_player, int32_t* p_offset);
	void UpdateMotion(Player* p_player);
	void UpdateMotionSounds(Player* p_player, int32_t (*p_sounds)[4], int32_t* p_offset);
	void ResetMotion(Player* p_player);

#ifdef __cplusplus
}
#endif

#endif // GPANIM_H
