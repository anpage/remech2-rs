#ifndef HUD_H
#define HUD_H

#include "mech.h"
#include "point.h"
#include "targeting.h"
#include "types.h"

// The functions and globals of hud.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_altimeterMarkWidth;
	extern int32_t g_altimeterMarkHeight;
	extern Point g_altimeterOrigin;
	extern Point g_compassOrigin;
	extern Point g_hudGaugePositions[6];
	extern uint32_t g_showHud;
	extern int32_t g_showCrosshair;
	extern int32_t g_showTargetMarker;
	extern int32_t g_showCompass;
	extern int32_t g_showAltimeter;
	extern int32_t g_compassArrowWidth;
	extern int32_t g_compassArrowHeight;
	extern int32_t g_compassSideArrowWidth;
	extern int32_t g_compassSideArrowHeight;
	extern int32_t g_altimeterGroundX;
	extern int32_t g_altimeterTargetX;
	extern int32_t g_altimeterScale;
	extern int32_t g_compassScale;
	extern int32_t g_compassTapeAbove;
	extern int32_t g_altimeterLevelX;
	extern int32_t g_compassTapeBelow;
	void DrawHudAt(
		Mech* p_mech,
		int32_t p_heading,
		int32_t p_twist,
		int32_t p_bearing,
		int32_t p_twistBearing,
		int32_t p_pitch,
		int32_t p_distance
	);
	void DrawHud(
		Mech* p_mech,
		int32_t p_heading,
		int32_t p_twist,
		int32_t p_bearing,
		int32_t p_twistBearing,
		int32_t p_pitch,
		int32_t p_distance,
		int32_t p_drawCrosshair
	);
	void DrawAltimeter(Mech* p_mech, int32_t p_x, int32_t p_y);
	void InitHudGauges(void);
	Point* GetHudGaugePositions(void);
	int32_t DrawCrosshair(Mech* p_mech, int32_t p_bearing, int32_t p_pitch, int32_t p_distance);
	void DrawTargetMarker(Mech* p_mech);
	void DrawCompassMarkers(
		Mech* p_mech,
		int32_t p_x,
		int32_t p_y,
		int32_t p_bearing,
		int32_t p_twistBearing,
		int32_t p_pitch
	);
	void DrawCompass(int32_t p_x, int32_t p_y, int32_t p_heading, int32_t p_twist);
	int32_t ProjectAimPoint(Mech* p_mech, int32_t* p_x, int32_t* p_y);
	void DrawPlayerBrackets(struct Player* p_player, int32_t p_side);
	void DrawObjectBrackets(struct SceneObject* p_object, int32_t p_side);
	void DrawHudShape(int32_t p_x, int32_t p_y, int32_t p_id);
	void DrawPaneShape(int32_t p_x, int32_t p_y, int32_t p_id, PANE* p_target);
	void DrawShapeOverPane(int32_t p_x, int32_t p_y, int32_t p_id, PANE* p_target);

#ifdef __cplusplus
}
#endif

#endif // HUD_H
