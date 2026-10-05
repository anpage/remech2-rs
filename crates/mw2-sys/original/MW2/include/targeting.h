#ifndef TARGETING_H
#define TARGETING_H

#include "cockpitreadout.h"
#include "decomp.h"
#include "navpoint.h"
#include "pane.h"
#include "recttransition.h"
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
	extern MechChar g_satelliteRangeText[0x40];
	extern MechChar g_satelliteExtraText[0x20];
	extern PANE g_satelliteSavedViewport;
	extern MechChar g_smallMapRangeText[0x20];
	extern MechChar g_smallMapExtraText[0x20];
	extern MechChar g_readoutText[0x20];
	extern PANE g_largeMapSavedViewport;
	extern MechChar g_largeMapRangeText[0x20];
	extern MechChar g_bearingText[0x20];
	extern PANE g_smallMapSavedViewport;
	extern MechChar g_largeMapExtraText[0x20];
	extern MechChar g_readoutLabel[8];
	extern CockpitReadout g_readout;
	extern MechChar g_smallMapExtraLabel[4];
	extern MechChar g_smallMapRangeLabel[4];
	extern MechChar g_smallMapBearingLabel[12];
	extern MechChar g_largeMapExtraLabel[4];
	extern MechChar g_largeMapRangeLabel[8];
	extern MechChar g_largeMapBearingLabel[12];
	extern MechChar g_satelliteExtraLabel[4];
	extern MechChar g_satelliteRangeLabel[8];
	extern MechChar g_bearingLabel[12];
	extern MechChar g_metersUnit[4];
	extern MechChar g_kilometersUnit[8];
	extern MechS32 g_smallMapColors[7][3];
	extern MechS32 g_largeMapColors[7][3];
	extern MechS32 g_satelliteColors[7][3];
	extern MechS32 g_mapColors[13];
	extern MechS32 g_smallMapAnims[5];
	extern MechS32 g_largeMapAnims[5];
	extern MechS32 g_satelliteAnims[5];
	extern PANE g_smallMapTransitionFirst;
	extern PANE g_smallMapTransitionSecond;
	extern PANE g_smallMapTransitionRect;
	extern RectTransitionState g_mapTransitionState;
	extern RectTransitionDef g_smallMapTransitionDef;
	extern RectTransition g_smallMapTransition;
	extern PANE g_largeMapTransitionFirst;
	extern PANE g_largeMapTransitionSecond;
	extern PANE g_largeMapTransitionRect;
	extern RectTransitionDef g_largeMapTransitionDef;
	extern RectTransition g_largeMapTransition;
	extern PANE g_satelliteTransitionFirst;
	extern PANE g_satelliteTransitionSecond;
	extern PANE g_satelliteTransitionRect;
	extern RectTransitionState g_satelliteTransitionState;
	extern RectTransitionDef g_satelliteTransitionDef;
	extern RectTransition g_satelliteTransition;
	extern PANE g_smallMapViewport;
	extern struct CockpitLayout g_smallMapLayout;
	extern PANE g_largeMapViewport;
	extern struct CockpitLayout g_largeMapLayout;
	extern PANE g_satelliteViewport;
	extern struct CockpitLayout g_satelliteLayout;

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
