#ifndef HUD_H
#define HUD_H

#include "decomp.h"
#include "mech.h"
#include "point.h"
#include "targeting.h"
#include "types.h"

// The functions and globals of hud.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_altimeterMarkWidth;
	extern MechS32 g_altimeterMarkHeight;
	extern Point g_altimeterOrigin;
	extern Point g_compassOrigin;
	extern Point g_hudGaugePositions[6];
	extern undefined4 g_showHud;
	extern MechS32 g_showCrosshair;
	extern MechS32 g_showTargetMarker;
	extern MechS32 g_showCompass;
	extern MechS32 g_showAltimeter;
	extern MechS32 g_compassArrowWidth;
	extern MechS32 g_compassArrowHeight;
	extern MechS32 g_compassSideArrowWidth;
	extern MechS32 g_compassSideArrowHeight;
	extern MechS32 g_altimeterGroundX;
	extern MechS32 g_altimeterTargetX;
	extern MechS32 g_altimeterScale;
	extern MechS32 g_compassScale;
	extern MechS32 g_compassTapeAbove;
	extern MechS32 g_altimeterLevelX;
	extern MechS32 g_compassTapeBelow;
	void DrawHudAt(
		Mech* p_mech,
		MechS32 p_heading,
		MechS32 p_twist,
		MechS32 p_bearing,
		MechS32 p_twistBearing,
		MechS32 p_pitch,
		MechS32 p_distance
	);
	void DrawHud(
		Mech* p_mech,
		MechS32 p_heading,
		MechS32 p_twist,
		MechS32 p_bearing,
		MechS32 p_twistBearing,
		MechS32 p_pitch,
		MechS32 p_distance,
		MechS32 p_drawCrosshair
	);
	void DrawAltimeter(Mech* p_mech, MechS32 p_x, MechS32 p_y);
	void InitHudGauges(void);
	Point* GetHudGaugePositions(void);
	MechS32 DrawCrosshair(Mech* p_mech, MechS32 p_bearing, MechS32 p_pitch, MechS32 p_distance);
	void DrawTargetMarker(Mech* p_mech);
	void DrawCompassMarkers(
		Mech* p_mech,
		MechS32 p_x,
		MechS32 p_y,
		MechS32 p_bearing,
		MechS32 p_twistBearing,
		MechS32 p_pitch
	);
	void DrawCompass(MechS32 p_x, MechS32 p_y, MechS32 p_heading, MechS32 p_twist);
	MechS32 ProjectAimPoint(Mech* p_mech, MechS32* p_x, MechS32* p_y);
	void DrawPlayerBrackets(struct Player* p_player, MechS32 p_side);
	void DrawObjectBrackets(struct SceneObject* p_object, MechS32 p_side);
	void DrawHudShape(MechS32 p_x, MechS32 p_y, MechS32 p_id);
	void DrawPaneShape(MechS32 p_x, MechS32 p_y, MechS32 p_id, PANE* p_target);
	void DrawShapeOverPane(MechS32 p_x, MechS32 p_y, MechS32 p_id, PANE* p_target);

#ifdef __cplusplus
}
#endif

#endif // HUD_H
