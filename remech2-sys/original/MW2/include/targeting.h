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

	extern int32_t g_navCount;
	extern int32_t g_inspectResult;
	extern int32_t g_reticleTargeting;
	extern struct CockpitReadout* g_cockpitReadout;
	extern struct CockpitLayout* g_cockpitLayouts[6];
	extern NavPoint g_navTable[128];
	extern char g_satelliteRangeText[0x40];
	extern char g_satelliteExtraText[0x20];
	extern PANE g_satelliteSavedViewport;
	extern char g_smallMapRangeText[0x20];
	extern char g_smallMapExtraText[0x20];
	extern char g_readoutText[0x20];
	extern PANE g_largeMapSavedViewport;
	extern char g_largeMapRangeText[0x20];
	extern char g_bearingText[0x20];
	extern PANE g_smallMapSavedViewport;
	extern char g_largeMapExtraText[0x20];
	extern char g_readoutLabel[8];
	extern CockpitReadout g_readout;
	extern char g_smallMapExtraLabel[4];
	extern char g_smallMapRangeLabel[4];
	extern char g_smallMapBearingLabel[12];
	extern char g_largeMapExtraLabel[4];
	extern char g_largeMapRangeLabel[8];
	extern char g_largeMapBearingLabel[12];
	extern char g_satelliteExtraLabel[4];
	extern char g_satelliteRangeLabel[8];
	extern char g_bearingLabel[12];
	extern char g_metersUnit[4];
	extern char g_kilometersUnit[8];
	extern int32_t g_smallMapColors[7][3];
	extern int32_t g_largeMapColors[7][3];
	extern int32_t g_satelliteColors[7][3];
	extern int32_t g_mapColors[13];
	extern int32_t g_smallMapAnims[5];
	extern int32_t g_largeMapAnims[5];
	extern int32_t g_satelliteAnims[5];
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

	int32_t AddNavPoint(uint32_t p_owner, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t RemoveNavPoint(uint32_t p_owner, uint32_t p_nav);
	void CycleTarget(struct Player* p_player, int32_t p_step, uint32_t p_flags);
	void ResetTargeting(void);
	int32_t TargetNavPoint(uint32_t p_player, int32_t p_nav, uint32_t p_flags);
	int32_t TargetGamePiece(int32_t p_player, int32_t p_index, uint32_t p_flags);
	int32_t TargetGameThing(int32_t p_player, int32_t p_index, uint32_t p_flags);
	int32_t UpdateTarget(struct Player* p_player);
	int32_t GetLocalTargetGamePiece(void);
	int32_t GetLocalTargetGameThing(void);
	struct Shape* GetLocalTargetShape(void);
	struct SceneObject* GetLocalTargetObject(void);
	void TargetAtReticle(void);
	void GetBearingAndRange(
		int32_t p_dx,
		int32_t p_dy,
		int32_t p_dz,
		int32_t* p_heading,
		int32_t* p_length,
		uint32_t* p_distance,
		int32_t* p_pitch
	);
	void CycleNavTarget(struct Player* p_player, int32_t p_step, int32_t p_ownOnly);
	void CycleGameThingTarget(int32_t p_step);
	void CycleGamePieceTarget(int32_t p_step);
	void CycleFriendlyTarget(int32_t p_step);
	void CycleEnemyTarget(int32_t p_step);
	void TargetNearestEnemy(void);

#ifdef __cplusplus
}
#endif

#endif // TARGETING_H
