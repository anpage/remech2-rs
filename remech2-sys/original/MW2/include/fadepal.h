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

	void PlayWeaponLaunchSound(undefined4 p_shotType, MechS32 p_sound, undefined4 p_pan);
	void RenderViewToPane(MechU32 p_target, MechS32 p_fovX, MechS32* p_view, struct SceneObject* p_object);
	void FlashZappedPalette(void);
	void FlashZappedPaletteLevel(MechU32 p_level);
	void FadeToEndPalette(MechS32 p_alternate);
	void EmitWreckSmoke(struct Mech* p_mech);
	void BreakUpMech(struct Mech* p_mech);
	void FireJumpJetEffects(struct Mech* p_mech);
	void PlayMechLanding(struct Mech* p_mech, MechS32 p_speed);
	void PlayPlayerHitFeedback(MechS32 p_x, MechS32 p_y, MechS32 p_z);

#ifdef __cplusplus
}
#endif

#endif // FADEPAL_H
