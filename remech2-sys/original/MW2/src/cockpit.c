#include "cockpit.h"

#include "anim2d.h"
#include "bandpoly.h"
#include "classtable.h"
#include "clock.h"
#include "cockpitpanel.h"
#include "cockpitreadout.h"
#include "collision.h"
#include "config.h"
#include "decomp.h"
#include "environment.h"
#include "eyepoint.h"
#include "face.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "fixedtrig.h"
#include "gamething.h"
#include "geocache.h"
#include "hud.h"
#include "loadres.h"
#include "mappoint.h"
#include "mapview.h"
#include "mechclass.h"
#include "menu.h"
#include "mw2prj.h"
#include "navpoint.h"
#include "object.h"
#include "palette.h"
#include "players.h"
#include "point.h"
#include "polydraw.h"
#include "random.h"
#include "recttransition.h"
#include "render.h"
#include "rendersettings.h"
#include "screenscale.h"
#include "setres.h"
#include "simmain.h"
#include "soundfx.h"
#include "speech.h"
#include "targeting.h"
#include "team.h"
#include "types.h"
#include "vertex.h"
#include "vfx3d.h"
#include "vfxa.h"
#include "widescreen.h"

#include <stdio.h>

// The satellite view's display option, 0 or 1 (DrawSatelliteStatic).
// GLOBAL: MW2 0x100a5a18
MechS32 g_satelliteStaticState = 0;

// The frame callback the satellite view replaces (SwitchCockpitView).
// GLOBAL: MW2 0x100a5a1c
void (*g_savedFrameDrawCallback)(void) = DrawScene;

// The overlay settings the satellite view keeps while a cockpit view shows (SwitchCockpitView):
// g_showCrosshair's, g_showHud's and g_showTargetMarker's, and whether they are held.

// GLOBAL: MW2 0x100a5a20
MechS32 g_savedShowCrosshair = 1;

// GLOBAL: MW2 0x100a5a24
undefined4 g_savedShowHud = 1;

// GLOBAL: MW2 0x100a5a28
MechS32 g_savedShowTargetMarker = 1;

// GLOBAL: MW2 0x100a5a2c
MechS32 g_hudSettingsSaved = 0;

// The height the map view's shading starts at (GetMapHeightShade).
// GLOBAL: MW2 0x100a5a30
MechS32 g_mapShadeBase = 0;

// GLOBAL: MW2 0x100a5a34
MechS32 g_mapShadeTop = 0x1900;

// The height range the map view's shading spans (GetMapHeightShade).
// GLOBAL: MW2 0x100a5a38
MechS32 g_mapShadeRange = 0x1900;

// The gauge functions of the cockpit layouts, by index.
// GLOBAL: MW2 0x100a5a40
CockpitGaugeFn g_cockpitGauges[10] = {
	NULL,
	(CockpitGaugeFn) OutlinePane,
	(CockpitGaugeFn) DrawGaugeEllipse,
	NULL,
	(CockpitGaugeFn) IsInsideGaugeEllipse,
	(CockpitGaugeFn) GetRectNeedleToward,
	(CockpitGaugeFn) GetEllipseNeedleToward,
	(CockpitGaugeFn) GetRectNeedleAt,
	(CockpitGaugeFn) GetEllipseNeedleAt,
	NULL
};

// GLOBAL: MW2 0x100a5a68
PANE g_cockpitGaugePanes[5] = {
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0}
};

// GLOBAL: MW2 0x100a5ad0
PANE g_unk0x100a5ad0[8] = {
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0},
	{&g_mainPixelBuffer, 0, 0, 0, 0}
};

// GLOBAL: MW2 0x100a5b70
Point g_unk0x100a5b70[4] = {0};

// GLOBAL: MW2 0x100a5b90
Point g_unk0x100a5b90[4] = {0};

// GLOBAL: MW2 0x100a5bb0
Point g_unk0x100a5bb0 = {0, 0};

// GLOBAL: MW2 0x100a5bb8
void* g_unk0x100a5bb8[4] = {g_unk0x100a5b90, g_unk0x100a5b70, g_unk0x100a5ad0, &g_unk0x100a5bb0};

// Whether cockpit view 1 and 2's map animation shows this frame (DrawDamagedMapView).
// GLOBAL: MW2 0x100a5bc8
MechS32 g_mapDamageAnimShown = 0;

// The clock times the satellite view's static starts and stops at (DrawSatelliteStatic).

// GLOBAL: MW2 0x100be410
MechS32 g_staticCleanUntil;

// GLOBAL: MW2 0x100be414
MechS32 g_staticNoiseUntil;

// The cockpit view before the current one (SwitchCockpitView).
// GLOBAL: MW2 0x10109c5c
MechS32 g_previousCockpitView;

// The map view's power mode, and the one before: 1 sliding in while the mech starts up
// (PowerUpMapView), 3 sliding out while it shuts down (PowerDownMapView), 4 running (RunMapView),
// 0 reset (ResetMapView). The readouts draw only while it runs.
// GLOBAL: MW2 0x10109c60
MechS32 g_previousMapViewMode;

// GLOBAL: MW2 0x10109c64
MechS32 g_cockpitLayoutIndex;

// GLOBAL: MW2 0x10109c68
MechS32 g_mapViewMode;

// The view g_cockpitLayoutIndex switches to; 3 and 5 enter and leave the satellite view (4).
// GLOBAL: MW2 0x10109c6c
MechS32 g_requestedCockpitView;

// Set while the map views follow the free eyepoint instead of the player (the free-eye cheat).
// GLOBAL: MW2 0x10109c70
MechS32 g_mapFollowsFreeEye;

// Set to draw the satellite view without its static once (DrawSatelliteStatic).
// GLOBAL: MW2 0x10109c74
MechS32 g_satelliteClean;

// Places the cockpit view p_cockpit on the screen: its viewport (kept in the render-target table,
// and for the normal cockpit also in g_cockpitPanelPanes), its points and extra rectangles, and its
// gauge functions.
// Stack-slot permutation: index, rect, transition and viewport.
// FUNCTION: MW2 0x1003dab0
void LoadCockpitLayout(MechS32 p_cockpit, CockpitLayout* p_layout)
{
	MechS32 index;
	PANE* rect;
	RectTransition* transition;
	PANE* viewport;

	if (p_layout == NULL) {
		return;
	}

	viewport = p_layout->m_viewport;
	g_cockpitLayoutIndex = p_cockpit;
	viewport->m_window = &g_mainPixelBuffer;
	ScaleRectToScreen(&g_mainPixelBuffer, viewport, viewport);
	if (p_cockpit == 1) {
		g_cockpitPanelPanes[0] = *viewport;
	}

	g_panes[p_layout->m_paneSlot] = *viewport;
	ScalePointToFrame(viewport, &p_layout->m_extraTextOrigin, &p_layout->m_extraTextOrigin);
	ScalePointToFrame(viewport, &p_layout->m_rangeTextOrigin, &p_layout->m_rangeTextOrigin);
	ScalePointToFrame(viewport, &p_layout->m_headingTextOrigin, &p_layout->m_headingTextOrigin);
	transition = p_layout->m_transition;
	if (transition) {
		rect = transition->m_def->m_first;
		rect->m_window = &g_mainPixelBuffer;
		if (p_cockpit == 4) {
			ScaleRectToLowRes(rect, g_eyepoint->m_pixelAspect);
			CenterRectOnScreen(&g_mainPixelBuffer, rect, rect);
		}
		else {
			ScaleRectToFrame(viewport, rect, rect);
		}

		rect = transition->m_def->m_second;
		rect->m_window = &g_mainPixelBuffer;
		if (p_cockpit == 4) {
			ScaleRectToLowRes(rect, g_eyepoint->m_pixelAspect);
			CenterRectOnScreen(&g_mainPixelBuffer, rect, rect);
		}
		else {
			ScaleRectToFrame(viewport, rect, rect);
		}

		rect = transition->m_def->m_out;
		rect->m_window = &g_mainPixelBuffer;
	}

	index = MECH_PTR_TO_S32(p_layout->m_gauges[0]);
	p_layout->m_gauges[0] = g_cockpitGauges[index];
	index = MECH_PTR_TO_S32(p_layout->m_gauges[1]);
	p_layout->m_gauges[1] = g_cockpitGauges[index];
	index = MECH_PTR_TO_S32(p_layout->m_gauges[2]);
	p_layout->m_gauges[2] = g_cockpitGauges[index];
	index = MECH_PTR_TO_S32(p_layout->m_gauges[3]);
	p_layout->m_gauges[3] = g_cockpitGauges[index];
	ZoomMapView(0);
	MechWidenCockpitLayout(p_cockpit, p_layout);
}

// Places every cockpit view's layout on the screen and starts in cockpit view 0, then the text
// readout and the map view's shading range.
// FUNCTION: MW2 0x1003dce8
void InitCockpitViews(void)
{
	MechS32 i;

	for (i = 0; i < 6; i++) {
		LoadCockpitLayout(i, g_cockpitLayouts[i]);
	}

	g_cockpitLayoutIndex = 0;
	g_requestedCockpitView = 1;
	g_previousCockpitView = 0;
	ScalePointToFrame(&g_currentPane, &g_cockpitReadout->m_position, &g_cockpitReadout->m_position);
	g_mapShadeBase -= 1000;
	g_mapShadeRange = g_mapShadeTop - g_mapShadeBase;
}

// The map view while the local mech runs: switches views as asked, drawing through the frame
// callback (DrawCockpitView), or the satellite view's own.
// FUNCTION: MW2 0x1003dd82
void RunMapView(void)
{
	g_previousMapViewMode = g_mapViewMode;
	g_mapViewMode = 4;
	if (g_cockpitLayoutIndex <= 2) {
		DrawCockpitView();
	}
	else if (g_cockpitLayoutIndex == 4 && g_requestedCockpitView <= 2) {
		SwitchCockpitView();
	}
}

// Moves to the cockpit view g_requestedCockpitView asks for: 0 to 2 directly, 3 into the satellite view
// (4) with its own frame callback, and 5 back out to the view it was entered from. Keeps the
// overlay settings the satellite view changes while a cockpit view shows, and leaves it when the
// player dies. Returns whether the view changed.
// Stack-slot permutation; g_cockpitLayoutIndex != g_requestedCockpitView compares in the other operand
// order.
// FUNCTION: MW2 0x1003ddd7
MechS32 SwitchCockpitView(void)
{
	MechS32 changed;
	CockpitLayout* layout;
	MechS32 sound;

	changed = FALSE;
	if (g_cockpitLayoutIndex <= 2 && g_previousCockpitView == 4 && g_hudSettingsSaved) {
		g_showCrosshair = g_savedShowCrosshair;
		g_showTargetMarker = g_savedShowTargetMarker;
		g_hudSettingsSaved = 0;
	}

	if (g_cockpitLayoutIndex == 4 && (g_localMechLost || (g_players[g_localPlayerId]->m_flags & 6))) {
		g_requestedCockpitView = 5;
	}

	if (g_cockpitLayoutIndex != g_requestedCockpitView) {
		changed = TRUE;
		switch (g_requestedCockpitView) {
		case 0:
			g_previousCockpitView = g_cockpitLayoutIndex;
			g_cockpitLayoutIndex = 0;
			break;
		case 1:
			g_previousCockpitView = g_cockpitLayoutIndex;
			g_cockpitLayoutIndex = 1;
			break;
		case 2:
			g_previousCockpitView = g_cockpitLayoutIndex;
			g_cockpitLayoutIndex = 2;
			break;
		case 4:
			g_previousCockpitView = g_cockpitLayoutIndex;
			g_cockpitLayoutIndex = 4;
			if (!g_hudSettingsSaved) {
				g_savedShowCrosshair = g_showCrosshair;
				g_showCrosshair = 0;
				g_savedShowTargetMarker = g_showTargetMarker;
				g_showTargetMarker = 0;
				g_savedShowHud = g_showHud;
				g_hudSettingsSaved = 1;
			}
			break;
		case 3:
			g_requestedCockpitView = 4;
			g_savedFrameDrawCallback = g_renderSettings.m_frameDrawCallback;
			g_renderSettings.m_frameDrawCallback = DrawCockpitView;
			layout = g_cockpitLayouts[4];
			sound = layout->m_sounds[0];
			if (sound != -1) {
				PlaySoundEffect(sound, 100, 0x40, 5, 0x50);
			}

			PlayCockpitSound(0x12, -1);
			break;
		case 5:
			g_renderSettings.m_frameDrawCallback = g_savedFrameDrawCallback;
			g_requestedCockpitView = g_previousCockpitView;
			layout = g_cockpitLayouts[4];
			sound = layout->m_sounds[1];
			if (sound != -1) {
				PlaySoundEffect(sound, 100, 0x40, 5, 0x50);
			}

			if (g_viewMode == c_viewTrack) {
				PlayCockpitSound(0x11, -1);
			}
			break;
		default:
			break;
		}
	}

	return changed;
}

// Draws the current cockpit view (the frame callback in the satellite view): switches views as
// asked, then draws the map, damaged while the radar panel is.
// FUNCTION: MW2 0x1003e03c
void DrawCockpitView(void)
{
	SwitchCockpitView();
	if (g_cockpitPanels[c_panelRadar]->m_damage) {
		DrawDamagedMapView();
	}
	else {
		DrawMapView();
	}
}

// Draws the current cockpit view's map: sets its pane up from the view, looking down on
// the player's weapon object (or the free eyepoint) from the view's range. The satellite view (4)
// draws the terrain with its own shape filter, face and polygon hooks, over a cleared view. Then
// the field of view, the icons, the view's own gauge and, in the map views, the readouts.
// The pose has a seventh element nothing uses. The only diff is a stack-slot permutation of the
// locals.
// FUNCTION: MW2 0x1003e06c
void DrawMapView(void)
{
	MechS32 farPlane;
	MechS32 heading;
	MechS32 angle;
	struct SceneObject* obj;
	MechS32 zoom;
	MechS32 pose[7];
	PANE* viewport;
	CockpitLayout* layout;
	MechS32 range;
	MechS32 z;
	MechS32 y;
	MechS32 x;
	MechS32 slot;
	Player* player;
	MechU32 flags;

	if (g_cockpitLayoutIndex == 0) {
		return;
	}

	layout = g_cockpitLayouts[g_cockpitLayoutIndex];
	if (!layout) {
		return;
	}

	player = g_players[g_localPlayerId];
	viewport = layout->m_viewport;
	slot = layout->m_paneSlot;
	g_panes[slot] = *viewport;
	angle = player->m_mech->m_torsoTwist.m_value;
	heading = player->m_heading;
	pose[4] = 0x5a0000;
	pose[5] = 0;
	switch (g_cockpitLayoutIndex) {
	case 1:
	case 2:
		pose[3] = heading;
		break;
	case 4:
		pose[3] = 0;
		angle += heading;
		break;
	default:
		pose[3] = 0;
		angle = 0;
		break;
	}

	if (!g_mapFollowsFreeEye) {
		x = y = z = 0;
		obj = player->m_eyeObj;
		if (obj) {
			GetObjPosition(obj, &x, &y, &z);
		}

		pose[0] = x;
		pose[2] = z;
	}
	else {
		pose[0] = g_eyepoint->m_x;
		pose[2] = g_eyepoint->m_z;
	}

	range = layout->m_range;
	pose[1] = range;
	if (g_mapShadeBase < 0) {
		farPlane = range - g_mapShadeBase;
	}
	else {
		farPlane = range;
	}

	BeginMapView(pose, slot, range, farPlane);
	if (g_cockpitLayoutIndex == 4) {
		g_renderSettings.m_shapeFilter = MapShapeFilter;
		g_renderSettings.m_drawFace = SatelliteFaceColor;
		g_renderSettings.m_drawPolygon = SatelliteDrawPolygon;
		zoom = GetViewMode();
		SetViewMode(c_viewSatellite);
		ChoosePlayerDetailLevels();
		SetViewMode(zoom);
		VFX_pane_wipe(viewport, g_groundColor);
		flags = 0;
		DrawMapViewScene(flags);
	}

	if (g_cockpitLayoutIndex != 4 || !(player->m_flags & 0x16)) {
		DrawMapFieldOfView(layout, angle);
	}

	DrawMapContents(layout);
	EndMapView();
	if (layout->m_gauges[0]) {
		((CockpitOutlineFn) layout->m_gauges[0])(viewport, layout->m_colors[12]);
	}

	if (g_mapViewMode == 4 && (g_cockpitLayoutIndex != 4 || g_satelliteStaticState <= 1)) {
		DrawMapViewText(layout);
	}
}

// Draws the map view's contents: the nav points, the center icon and the other players and
// things, and the target.
// The only diff is a stack-slot permutation of the locals. The original has an unused local (unused).
// FUNCTION: MW2 0x1003e32c
void DrawMapContents(CockpitLayout* p_layout)
{
	MechS32 id;
	PANE* viewport;
	MechS32 unused;
	MechS32 y;
	MechS32 x;
	Player* player;
	void* shape;

	viewport = p_layout->m_viewport;
	player = g_players[g_localPlayerId];
	DrawMapNavPoints(p_layout);
	if (g_cockpitLayoutIndex != 4) {
		x = (viewport->m_x1 - viewport->m_x0 + 1) >> 1;
		y = (viewport->m_y1 - viewport->m_y0 + 1) >> 1;
		id = p_layout->m_icons[0][0] + g_artResolution;
		shape = LoadCachedResource(g_mw2PrjHandle, id, g_resourceTypeTags[c_resTagShp], 0);
		if (shape) {
			VFX_shape_draw(viewport, shape, 0, x, y);
			UnlockCachedResource(id, g_resourceTypeTags[c_resTagShp]);
			DrawMapUnits(p_layout);
		}
	}

	DrawMapTarget(p_layout);
}

// Draws icon p_icon at the world position p_pos, if the map view shows it.
// FUNCTION: MW2 0x1003e40a
void DrawMapIcon(CockpitLayout* p_layout, MapPoint p_pos, MechS32 p_icon)
{
	PANE* viewport;
	void* shape;
	MechS32 visible;

	viewport = p_layout->m_viewport;
	visible = ProjectMapPoint(&p_pos);
	if (visible && p_layout->m_gauges[1]) {
		visible = ((CockpitInsideFn) p_layout->m_gauges[1])(viewport, p_pos.m_xy.m_x, p_pos.m_xy.m_y);
	}

	if (visible) {
		shape = LoadCachedResource(g_mw2PrjHandle, p_icon + g_artResolution, g_resourceTypeTags[c_resTagShp], 0);
		if (shape) {
			VFX_shape_draw(viewport, shape, 0, p_pos.m_xy.m_x, p_pos.m_xy.m_y);
			UnlockCachedResource(p_icon + g_artResolution, g_resourceTypeTags[c_resTagShp]);
		}
	}
}

// Draws the other live players and the live game things, by side.
// Stack-slot permutation; i < g_gameThingCount compares in the other operand order.
// FUNCTION: MW2 0x1003e4cd
void DrawMapUnits(CockpitLayout* p_layout)
{
	Player* player;
	MechS32 icon;
	GameThing* thing;
	MechS32 i;
	MapPoint pos;

	for (i = 0; i < g_playerCount; i++) {
		if (i != g_localPlayerId) {
			player = g_players[i];
			if ((player->m_flags & 0x1400) && !(player->m_flags & 0x800) && !(player->m_flags & 0x4000) &&
				!(player->m_flags & 0x16)) {
				pos.m_xy.m_x = player->m_position.m_x;
				pos.m_xy.m_y = player->m_position.m_y;
				pos.m_z = player->m_position.m_z;
				icon = p_layout->m_icons[1][GetPlayerSide(i)];
				DrawMapIcon(p_layout, pos, icon);
			}
		}
	}

	for (i = 0; i < g_gameThingCount; i++) {
		thing = &g_gameThings[i];
		if ((thing->m_flags & 0x1400) && !(thing->m_flags & 0x1e)) {
			GetStaticObjectPosition(thing->m_staticObject, &pos.m_xy.m_x, &pos.m_xy.m_y, &pos.m_z);
			icon = p_layout->m_icons[1][GetThingSide(i)];
			DrawMapIcon(p_layout, pos, icon);
		}
	}
}

// Returns whether team p_team has reached the nav point. The team test is an | where an & was
// meant: any reached nav counts.
// FUNCTION: MW2 0x1003e645
MechS32 IsNavReached(NavPoint* p_nav, MechS32 p_team)
{
	return (p_nav->m_flags & 0x20) && (p_nav->m_teamsReached | (1 << p_team));
}

// Draws the local player's target: in the view, or clamped to its edge when outside it.
// Stack-slot permutation; m_icons[row][side] loads the table before the row (index order).
// FUNCTION: MW2 0x1003e689
void DrawMapTarget(CockpitLayout* p_layout)
{
	MechS32 index;
	Player* player;
	MechS32 visible;
	MechS32 icon;
	MechS32 side;
	MechS32 kind;
	PANE* viewport;
	MapPoint pos;
	MechS32 row;
	MechU32 target;
	void* shape;

	viewport = p_layout->m_viewport;
	icon = -1;
	player = g_players[g_localPlayerId];
	pos.m_xy.m_x = player->m_targetInfo.m_position.m_x;
	pos.m_xy.m_y = player->m_targetInfo.m_position.m_y;
	pos.m_z = player->m_targetInfo.m_position.m_z;
	visible = ProjectMapPoint(&pos);
	if (visible && p_layout->m_gauges[1]) {
		visible = ((CockpitInsideFn) p_layout->m_gauges[1])(viewport, pos.m_xy.m_x, pos.m_xy.m_y);
	}

	target = player->m_targetInfo.m_target;
	if (!target || (target & 0x1000)) {
		return;
	}

	kind = target & 0xf00;
	index = target & 0xff;
	switch (kind) {
	case 0x200:
		side = GetPlayerSide(index);
		if ((g_players[index]->m_flags & 0x1400) && !(g_players[index]->m_flags & 0x800)) {
			row = 2;
		}
		else {
			row = 3;
		}
		break;
	case 0x400:
		side = GetThingSide(index);
		if (g_gameThings[index].m_flags & 0x1400) {
			row = 2;
		}
		else {
			row = 3;
		}
		break;
	case 0x100:
		if (visible) {
			if (IsNavReached(&g_navTable[index], g_localStar)) {
				side = 1;
			}
			else {
				side = 0;
			}
		}
		else {
			side = 0;
		}

		row = 5;
		break;
	default:
		return;
		break;
	}

	if (!visible) {
		row = 6;
	}

	icon = p_layout->m_icons[row][side];
	if (!icon) {
		return;
	}

	shape = LoadCachedResource(g_mw2PrjHandle, g_artResolution + icon, g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		if (visible) {
			VFX_shape_draw(viewport, shape, 0, pos.m_xy.m_x, pos.m_xy.m_y);
		}
		else if (p_layout->m_gauges[2]) {
			((CockpitNeedleTowardFn) p_layout->m_gauges[2])(viewport, &pos.m_xy, &pos.m_xy);
			VFX_shape_draw(viewport, shape, 0, pos.m_xy.m_x, pos.m_xy.m_y);
		}

		UnlockCachedResource(g_artResolution + icon, g_resourceTypeTags[c_resTagShp]);
	}
}

// Draws the local team's nav points, marking the ones reached.
// Stack-slot permutation; i < g_navCount compares in the other operand order.
// FUNCTION: MW2 0x1003e974
void DrawMapNavPoints(CockpitLayout* p_layout)
{
	MapPoint pos;
	MechS32 i;
	PANE* viewport;
	MechS32 icon;
	MechS32 visible;
	NavPoint* nav;
	void* shape;

	viewport = p_layout->m_viewport;
	for (i = 0; i < g_navCount; i++) {
		nav = &g_navTable[i];
		if (nav->m_team == g_localStar && nav->m_used && !(nav->m_flags & 1)) {
			if (nav->m_obj) {
				GetObjPosition(nav->m_obj, &pos.m_xy.m_x, &pos.m_xy.m_y, &pos.m_z);
			}
			else {
				pos.m_xy.m_x = nav->m_position[0];
				pos.m_xy.m_y = nav->m_position[1];
				pos.m_z = nav->m_position[2];
			}

			visible = ProjectMapPoint(&pos);
			if (visible && p_layout->m_gauges[1]) {
				visible = ((CockpitInsideFn) p_layout->m_gauges[1])(viewport, pos.m_xy.m_x, pos.m_xy.m_y);
			}

			if (visible) {
				if (IsNavReached(nav, g_localStar)) {
					icon = p_layout->m_icons[4][1];
				}
				else {
					icon = p_layout->m_icons[4][0];
				}

				if (icon != -1) {
					shape =
						LoadCachedResource(g_mw2PrjHandle, g_artResolution + icon, g_resourceTypeTags[c_resTagShp], 0);
					if (shape) {
						VFX_shape_draw(viewport, shape, 0, pos.m_xy.m_x, pos.m_xy.m_y);
						UnlockCachedResource(g_artResolution + icon, g_resourceTypeTags[c_resTagShp]);
					}
				}
			}
		}
	}
}

// Draws the two lines of the view's field of view from the center, around heading p_heading.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1003eb22
void DrawMapFieldOfView(CockpitLayout* p_layout, MechS32 p_heading)
{
	MechS32 halfFov;
	PANE* viewport;
	Point end;
	MechS32 y;
	MechS32 x;
	MechS32* colors;

	viewport = p_layout->m_viewport;
	colors = p_layout->m_colors;
	p_heading = (p_heading % 0x1680000 + 0x1680000) % 0x1680000;
	x = (viewport->m_x1 - viewport->m_x0 + 1) >> 1;
	y = (viewport->m_y1 - viewport->m_y0 + 1) >> 1;
	halfFov = FixedAtan2(0x10000, g_eyepoint->m_fovX);
	p_heading = 0x5a0000 - p_heading;
	if (p_layout->m_gauges[3]) {
		((CockpitNeedleAtFn) p_layout->m_gauges[3])(viewport, p_heading - halfFov, &end);
		VFX_line_draw(viewport, x, y, end.m_x, end.m_y, 0, colors[11]);
		((CockpitNeedleAtFn) p_layout->m_gauges[3])(viewport, halfFov + p_heading, &end);
		VFX_line_draw(viewport, x, y, end.m_x, end.m_y, 0, colors[11]);
	}
}

// Draws the view's range readout and, in the satellite view, the heading readout, formatting them
// again only when they change.
// Stack-slot permutation: every local.
// FUNCTION: MW2 0x1003ec35
void DrawMapViewText(CockpitLayout* p_layout)
{
	MechDouble value;
	MechChar* units;
	MechS32 range;
	void* font;
	PANE* viewport;
	MechS32 heading;
	Player* player;
	MechDouble degrees;
	PANE saved;

	viewport = p_layout->m_viewport;
	// In widescreen the satellite view's viewport covers the frame and its readouts stay in the HUD box
	saved = *viewport;
	if (g_cockpitLayoutIndex == 4) {
		MechHudPane(viewport);
	}

	player = g_players[g_localPlayerId];
	font = LoadCachedResource(g_mw2PrjHandle, p_layout->m_font + g_artResolution, g_resourceTypeTags[c_resTagFont], 0);
	if (font) {
		if (p_layout->m_range != p_layout->m_formattedRange) {
			range = p_layout->m_range / 2;
			if (range >= 100000) {
				range = FixedDiv16(range, 100000);
				units = p_layout->m_longUnit;
			}
			else {
				range = FixedDiv16(range, 100);
				units = p_layout->m_shortUnit;
			}

			value = range / 65536.0;
			sprintf(p_layout->m_rangeText, "%s%3.1lf%s", p_layout->m_rangeLabel, value, units);
			p_layout->m_formattedRange = p_layout->m_range;
		}

		VFX_string_draw(
			viewport,
			p_layout->m_rangeTextOrigin.m_x,
			p_layout->m_rangeTextOrigin.m_y,
			font,
			p_layout->m_rangeText,
			g_textColors
		);
		if (g_cockpitLayoutIndex == 4) {
			heading = player->m_heading;
			heading = (heading % 0x1680000 + 0x1680000) % 0x1680000;
			if (p_layout->m_formattedHeading != heading) {
				degrees = heading / 65536.0;
				sprintf(p_layout->m_headingText, "%s%3.1lf", p_layout->m_headingLabel, degrees);
				p_layout->m_formattedHeading = heading;
			}

			VFX_string_draw(
				viewport,
				p_layout->m_headingTextOrigin.m_x,
				p_layout->m_headingTextOrigin.m_y,
				font,
				p_layout->m_headingText,
				g_textColors
			);
		}

		UnlockCachedResource(p_layout->m_font + g_artResolution, g_resourceTypeTags[c_resTagFont]);
	}

	*viewport = saved;
}

// Cycles the three cockpit views.
// FUNCTION: MW2 0x1003ee26
void CycleCockpitView(void)
{
	if (g_cockpitLayoutIndex <= 2 && g_requestedCockpitView <= 2) {
		g_requestedCockpitView = g_cockpitLayoutIndex + 1;
		g_requestedCockpitView %= 3;
	}
}

// FUNCTION: MW2 0x1003ee69
MechS32 IsSatelliteView(void)
{
	return g_cockpitLayoutIndex == 4;
}

// FUNCTION: MW2 0x1003ee92
void LeaveSatelliteView(void)
{
	if (g_cockpitLayoutIndex == 4) {
		ToggleSatelliteView();
	}
}

// Leaves the satellite view, or else enters it from a cockpit view.
// FUNCTION: MW2 0x1003eeaf
void ToggleSatelliteView(void)
{
	if (g_cockpitLayoutIndex == 4 && g_requestedCockpitView == 4) {
		g_requestedCockpitView = 5;
	}
	else if (g_cockpitLayoutIndex <= 2 && g_requestedCockpitView <= 2) {
		g_requestedCockpitView = 3;
	}
}

// Zooms the current view's map: p_zoom 0 resets the range, 1 halves it (wrapping to the longest
// below the shortest) and 2 doubles it (wrapping to the shortest).
// FUNCTION: MW2 0x1003ef07
void ZoomMapView(MechS32 p_zoom)
{
	CockpitLayout* layout;

	layout = g_cockpitLayouts[g_cockpitLayoutIndex];
	if (!layout) {
		return;
	}

	switch (p_zoom) {
	case 0:
		layout->m_range = layout->m_startRange;
		break;
	case 1:
		layout->m_range = FixedDiv16(layout->m_range, 0x20000);
		if (layout->m_minRange > layout->m_range) {
			layout->m_range = layout->m_maxRange;
		}
		break;
	case 2:
		layout->m_range = FixedMul16(layout->m_range, 0x20000);
		if (layout->m_maxRange < layout->m_range) {
			layout->m_range = layout->m_minRange;
		}
		break;
	}

	layout->m_zoom = FixedDiv16(layout->m_startRange, layout->m_range);
}

// The map view's shape filter (RenderSettings::m_shapeFilter): skips dead players' shapes and
// shapes of types 0x30 and 0x70, then culls through CullMapViewShape.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1003f00d
MechS32 MapShapeFilter(Shape* p_shape)
{
	MechS32 type;
	MechS32 flags;
	MechS32 skip;
	MechS32 kind;

	skip = FALSE;
	kind = p_shape->m_kind & 0xf00;
	if (kind == 0x100) {
		flags = g_players[p_shape->m_owner]->m_flags;
		if (flags & 0x16) {
			skip = TRUE;
		}
		else {
			skip = FALSE;
		}
	}
	else {
		type = p_shape->m_kind & 0xf0;
		switch (type) {
		case 0x30:
		case 0x70:
			skip = TRUE;
			break;
		default:
			break;
		}
	}

	if (!skip) {
		skip = CullMapViewShape(p_shape);
	}

	return skip;
}

// The satellite view's face hook (RenderSettings::m_drawFace): the color a face of p_face's shape
// draws in, from the view's colors by the shape's kind and side, or the face's own (p_flags).
// Faces of a textured kind (0x3000) draw in the view's color 10.
// The only diff is a stack-slot permutation of the locals (and the jump tables' addresses).
// FUNCTION: MW2 0x1003f0e7
MechU32 SatelliteFaceColor(Face* p_face, Vertex* p_vertices, MechU32 p_flags, MechS32 p_distance)
{
	MechU32 color;
	MechU32 type;
	MechU32 result;
	MechU32 index;
	CockpitLayout* layout;
	Shape* shape;
	MechS32 kind;
	MechS32* colors;

	result = 0;
	layout = g_cockpitLayouts[g_cockpitLayoutIndex];
	if (!layout) {
		return result;
	}

	colors = layout->m_colors;
	result = 0;
	if (g_renderSettings.m_greyscale) {
		result |= 0xf0;
	}

	shape = p_face->m_shape;
	kind = shape->m_kind & 0xf00;
	switch (kind) {
	case 0x100:
		index = p_face->m_shape->m_owner;
		result |= colors[GetPlayerSide(index)];
		break;
	case 0x200:
		index = p_face->m_shape->m_owner;
		result |= colors[GetThingSide(index) + 3];
		break;
	case 0x400:
		result |= colors[6];
		break;
	case 0x800:
		if ((p_flags & 0x7000) == 0x3000) {
			color = colors[10];
		}
		else {
			color = (p_flags & 0xf00) >> 4;
		}

		result = color | 0x4000;
		break;
	default:
		type = shape->m_kind & 0xf0;
		switch (type) {
		case 0x40:
			result |= colors[7];
			break;
		case 0x50:
			result |= colors[8];
			break;
		case 0x80:
			result |= colors[9];
			break;
		case 0x10:
		case 0x20:
		case 0x60:
			result = p_flags;
			break;
		default:
			if ((p_flags & 0x7000) == 0x3000) {
				color = colors[10];
			}
			else {
				color = (p_flags & 0xf00) >> 4;
			}

			result = color | 0x4000;
			break;
		}
	}

	return result;
}

// The satellite view's polygon hook (RenderSettings::m_drawPolygon): terrain faces (0x4000) are
// shaded by height, textured ones (0x3000) drawn as bands, and the rest as usual; plain faces
// (0) twice, the second time outlined (0x2000).
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1003f393
void SatelliteDrawPolygon(MechS32 p_count, MechU32* p_points, MechU32 p_flags)
{
	MechU32 shade;
	MechU32 kind;
	MechS32 i;
	MechU32* point;
	CockpitLayout* layout;
	MechU32 color;

	layout = g_cockpitLayouts[g_cockpitLayoutIndex];
	if (!layout) {
		return;
	}

	kind = p_flags & 0x7000;
	point = p_points;
	switch (kind) {
	case 0x4000:
		p_flags &= 0xf0;
		for (i = 0; i < p_count; i++) {
			shade = GetMapHeightShade(layout, point[5]);
			point[2] = (p_flags | shade) << 16;
			point += 6;
		}

		if (g_renderSettings.m_gouraud) {
			VFX_dithered_Gouraud_polygon(&g_currentPane, 0x7fff, p_count, p_points);
		}
		else {
			VFX_flat_polygon(&g_currentPane, p_count, p_points);
		}
		break;
	case 0x3000:
		color = (p_flags & 0xff0) >> 4;
		DrawBand(color, p_count, (MechS32*) p_points, -1, 1);
		break;
	case 0:
		DrawScenePolygon(p_count, p_points, 0);
		DrawScenePolygon(p_count, p_points, p_flags | 0x2000);
		break;
	default:
		DrawScenePolygon(p_count, p_points, p_flags);
		break;
	}
}

// Returns the shade (0-15) of a height p_height in the map view.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1003f513
MechS32 GetMapHeightShade(CockpitLayout* p_layout, MechS32 p_height)
{
	MechS32 shade;
	MechS32 height;
	MechS32 fraction;

	height = p_layout->m_range - (p_height >> 2);
	height -= g_mapShadeBase;
	fraction = FixedDiv16(height, g_mapShadeRange);
	shade = FixedMul16(fraction, 0x10);
	if (shade < 0) {
		shade = 0;
	}
	else if (shade > 15) {
		shade = 15;
	}

	return shade;
}

// Draws the layout's view into the rectangle p_transition has reached, restoring the viewport
// afterwards. Returns FALSE once the transition is over; then, with p_final, it draws into the
// transition's final rectangle and has it stretched to the screen.
// FUNCTION: MW2 0x1003f594
MechS32 DrawMapViewTransition(MechS32 p_reverse, CockpitLayout* p_layout, RectTransition* p_transition, MechS32 p_final)
{
	MechS32 result;
	PANE* rect;
	PANE* viewport;

	viewport = p_layout->m_viewport;
	result = TRUE;
	rect = UpdateRectTransition(p_reverse, p_transition);
	if (!rect) {
		result = FALSE;
		if (p_final) {
			if (!p_reverse) {
				rect = p_transition->m_def->m_second;
			}
			else {
				rect = p_transition->m_def->m_first;
			}
		}
	}

	if (rect) {
		*p_layout->m_savedViewport = *viewport;
		*viewport = *rect;
		DrawMapView();
		*viewport = *p_layout->m_savedViewport;
		if (p_final) {
			g_currentPane = *rect;
			g_stretchPending = 1;
		}
	}

	return result;
}

// The map view while the local mech starts up: slides the view's rectangle in (its transition),
// then draws it.
// FUNCTION: MW2 0x1003f66d
void PowerUpMapView(void)
{
	RectTransition* transition;
	CockpitLayout* layout;

	if (g_cockpitLayoutIndex <= 2) {
		SwitchCockpitView();
		if (g_cockpitLayoutIndex > 2) {
			return;
		}

		if (g_cockpitLayoutIndex != 0) {
			g_previousMapViewMode = g_mapViewMode;
			g_mapViewMode = 1;
		}

		layout = g_cockpitLayouts[g_cockpitLayoutIndex];
		if (layout) {
			transition = layout->m_transition;
			if (transition) {
				if (g_previousMapViewMode != g_mapViewMode) {
					StartRectTransition(transition);
				}

				if (!DrawMapViewTransition(0, layout, transition, 0)) {
					DrawCockpitView();
				}
			}
		}
	}
	else if (g_cockpitLayoutIndex == 4 && g_requestedCockpitView <= 2) {
		SwitchCockpitView();
	}
}

// Draws the satellite view through its static: while the mission menu is open, or after a
// reset (g_satelliteClean), cleanly; otherwise clean until a random time, then the view's
// transition breaks it up until another random time.
// The two clock comparisons compare in the other operand order.
// FUNCTION: MW2 0x1003f74e
void DrawSatelliteStatic(void)
{
	RectTransition* transition;
	CockpitLayout* layout;

	layout = g_cockpitLayouts[g_cockpitLayoutIndex];
	if (!layout || !(transition = layout->m_transition)) {
		return;
	}

	if (GetMenuSlotState(4) || g_satelliteClean) {
		g_satelliteClean = 0;
		DrawMapView();
		return;
	}

	switch (g_satelliteStaticState) {
	case 0:
		g_staticCleanUntil = g_currentClock + RandomIntBelow(transition->m_def->m_duration);
		g_satelliteStaticState = 1;
	case 1:
		if (g_staticCleanUntil > g_currentClock) {
			DrawMapView();
			break;
		}

		g_satelliteStaticState = 2;
		g_staticNoiseUntil = g_currentClock + RandomIntBelow(transition->m_def->m_duration);
		StartRectTransition(transition);
		g_showHud = 0;
	case 2:
		transition->m_state->m_elapsed = RandomIntBelow(transition->m_def->m_duration - g_deltaTime - 1);
		DrawMapViewTransition(1, layout, transition, 1);
		if (g_staticNoiseUntil < g_currentClock) {
			g_satelliteStaticState = 0;
			break;
		}
		break;
	}
}

// Draws the current cockpit view while the radar panel is damaged: the satellite view through its
// static, or the map with the view's damage animation over it, flickering at damage 1 and steady
// at 2.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1003f8d1
void DrawDamagedMapView(void)
{
	MechS32 threshold;
	PANE* viewport;
	CockpitLayout* layout;
	MechS32 anim;

	layout = g_cockpitLayouts[g_cockpitLayoutIndex];
	if (!layout) {
		return;
	}

	if (g_cockpitLayoutIndex == 4) {
		DrawSatelliteStatic();
		return;
	}

	anim = layout->m_anims[2];
	if (anim != -1) {
		if (g_cockpitPanels[c_panelRadar]->m_damage == 1) {
			if (g_mapDamageAnimShown) {
				threshold = 7;
			}
			else {
				threshold = 3;
			}

			if (RandomIntBelow(10) < threshold) {
				g_mapDamageAnimShown = 1;
			}
			else {
				g_mapDamageAnimShown = 0;
			}
		}
		else if (g_cockpitPanels[c_panelRadar]->m_damage == 2) {
			g_mapDamageAnimShown = 1;
		}

		DrawMapView();
		if (g_mapDamageAnimShown) {
			viewport = layout->m_viewport;
			DrawAnim2d(viewport, anim, 0, 0);
			if (layout->m_gauges[0]) {
				((CockpitOutlineFn) layout->m_gauges[0])(viewport, layout->m_colors[12]);
			}
		}
	}
	else {
		DrawMapView();
	}
}

// The map view while the local mech shuts down or is off: slides the view's rectangle out.
// FUNCTION: MW2 0x1003fa05
void PowerDownMapView(void)
{
	RectTransition* transition;
	CockpitLayout* layout;

	if (g_cockpitLayoutIndex <= 2) {
		SwitchCockpitView();
		if (g_cockpitLayoutIndex > 2) {
			return;
		}

		if (g_cockpitLayoutIndex != 0) {
			g_previousMapViewMode = g_mapViewMode;
			g_mapViewMode = 3;
		}

		layout = g_cockpitLayouts[g_cockpitLayoutIndex];
		if (layout) {
			transition = layout->m_transition;
			if (transition) {
				if (g_previousMapViewMode != g_mapViewMode) {
					StartRectTransition(transition);
				}

				DrawMapViewTransition(1, layout, transition, 0);
			}
		}
	}
	else if (g_cockpitLayoutIndex == 4 && g_requestedCockpitView <= 2) {
		SwitchCockpitView();
	}
}

// Resets the map view's power modes (ShutdownCockpitPanels).
// FUNCTION: MW2 0x1003fad9
void ResetMapView(void)
{
	g_previousMapViewMode = 0;
	g_mapViewMode = 0;
}
