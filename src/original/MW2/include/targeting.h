#ifndef TARGETING_H
#define TARGETING_H

#include "decomp.h"
#include "navpoint.h"
#include "pane.h"
#include "types.h"
#include "vfxa.h"
#include "window.h"

struct SceneObject;
struct Player;
struct Shape;

// The functions and globals of targeting.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_navCount;
	extern MechS32 g_inspectResult;
	extern MechS32 g_reticleTargeting;
	extern struct CockpitReadout* g_cockpitReadout;
	extern struct CockpitLayout* g_cockpitLayouts[6];
	extern NavPoint g_navTable[128];

	MechS32 AddNavPoint(MechU32 p_owner, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 RemoveNavPoint(MechU32 p_owner, MechU32 p_nav);
	void CycleTarget(struct Player* p_player, MechS32 p_step, MechU32 p_flags);
	void ResetTargeting(void);
	MechS32 TargetNavPoint(MechU32 p_player, MechS32 p_nav, MechU32 p_flags);
	MechS32 TargetGamePiece(MechS32 p_player, MechS32 p_index, MechU32 p_flags);
	MechS32 TargetGameThing(MechS32 p_player, MechS32 p_index, MechU32 p_flags);
	MechS32 UpdateTarget(struct Player* p_player);
	MechS32 GetLocalTargetGamePiece(void);
	MechS32 GetLocalTargetGameThing(void);
	struct Shape* GetLocalTargetShape(void);
	struct SceneObject* GetLocalTargetObject(void);
	void TargetAtReticle(void);
	void GetBearingAndRange(
		MechS32 p_dx,
		MechS32 p_dy,
		MechS32 p_dz,
		MechS32* p_heading,
		MechS32* p_length,
		MechU32* p_distance,
		MechS32* p_pitch
	);
	void CycleNavTarget(struct Player* p_player, MechS32 p_step, MechS32 p_ownOnly);
	void CycleGameThingTarget(MechS32 p_step);
	void CycleGamePieceTarget(MechS32 p_step);
	void CycleFriendlyTarget(MechS32 p_step);
	void CycleEnemyTarget(MechS32 p_step);
	void TargetNearestEnemy(void);

#ifdef __cplusplus
}
#endif

#endif // TARGETING_H
