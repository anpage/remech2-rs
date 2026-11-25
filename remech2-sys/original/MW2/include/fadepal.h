#ifndef FADEPAL_H
#define FADEPAL_H

#include "decomp.h"
#include "types.h"

struct SceneObject;

struct Mech;

// The functions of fadepal.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void PlayWeaponLaunchSound(undefined4 p_shotType, int32_t p_sound, undefined4 p_pan);
	void RenderViewToPane(uint32_t p_target, int32_t p_fovX, int32_t* p_view, struct SceneObject* p_object);
	void FlashZappedPalette(void);
	void FlashZappedPaletteLevel(uint32_t p_level);
	void FadeToEndPalette(int32_t p_alternate);
	void EmitWreckSmoke(struct Mech* p_mech);
	void BreakUpMech(struct Mech* p_mech);
	void FireJumpJetEffects(struct Mech* p_mech);
	void PlayMechLanding(struct Mech* p_mech, int32_t p_speed);
	void PlayPlayerHitFeedback(int32_t p_x, int32_t p_y, int32_t p_z);

#ifdef __cplusplus
}
#endif

#endif // FADEPAL_H
