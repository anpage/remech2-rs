#include "hud.h"

#include "bargauges.h"
#include "cockpitpanel.h"
#include "config.h"
#include "environment.h"
#include "eyepoint.h"
#include "fixedmul.h"
#include "geocache.h"
#include "loadres.h"
#include "muldiv.h"
#include "muldiv14.h"
#include "mw2prj.h"
#include "object.h"
#include "players.h"
#include "polydraw.h"
#include "ray.h"
#include "render.h"
#include "screenscale.h"
#include "setres.h"
#include "shape.h"
#include "targeting.h"
#include "team.h"
#include "types.h"
#include "vfxa.h"
#include "view.h"
#include "weapondata.h"
#include "weapons.h"

// The size of the altimeter's mark shape (InitHudGauges), which spaces its columns.
int32_t g_altimeterMarkWidth = 0xf0f;

int32_t g_altimeterMarkHeight = 0xd79;

// The altimeter's place, in 16.16 fractions of its gauge until InitHudGauges scales it.
Point g_altimeterOrigin = {0xb333, 0x8000};

// The compass's place, likewise.
Point g_compassOrigin = {0x8000, 0x6666};

Point g_hudGaugePositions[6] = {{0x73, 0x10}, {8, 0x4a}, {4, 0x28}, {4, 0x4a}, {0, 0}, {0, 0}};

// The HUD's display options: the HUD itself, the crosshair, the target marker, the compass and
// the altimeter.
uint32_t g_showHud = 1;

int32_t g_showCrosshair = 1;

int32_t g_showTargetMarker = 1;

int32_t g_showCompass = 1;

int32_t g_showAltimeter = 1;

// The layout of the altimeter and the compass, from their shapes' extents (InitHudGauges).

int32_t g_compassArrowWidth;

int32_t g_compassArrowHeight;

int32_t g_compassSideArrowWidth;

int32_t g_compassSideArrowHeight;

int32_t g_altimeterGroundX;

int32_t g_altimeterTargetX;

// The altimeter's scale: pixels per 16.16 unit of height.
int32_t g_altimeterScale;

// The compass's scale: pixels per degree, 16.16.
int32_t g_compassScale;

int32_t g_compassTapeAbove;

int32_t g_altimeterLevelX;

int32_t g_compassTapeBelow;

// Draws the HUD's overlays the display options enable: the compass and its target markers, the
// crosshair, the target marker and the altimeter, at the gauge positions.
void DrawHudAt(
	Mech* p_mech,
	int32_t p_heading,
	int32_t p_twist,
	int32_t p_bearing,
	int32_t p_twistBearing,
	int32_t p_pitch,
	int32_t p_distance
)
{
	if (!g_showHud) {
		return;
	}

	if (g_showCompass) {
		DrawCompass(g_hudGaugePositions[0].m_x, g_hudGaugePositions[0].m_y, p_heading, p_twist);
		DrawCompassMarkers(
			p_mech,
			g_hudGaugePositions[0].m_x,
			g_hudGaugePositions[0].m_y,
			p_bearing,
			p_twistBearing,
			p_pitch
		);
	}

	if (g_showCrosshair) {
		DrawCrosshair(p_mech, p_twistBearing, p_pitch, p_distance);
	}

	if (g_showTargetMarker) {
		DrawTargetMarker(p_mech);
	}

	if (g_showAltimeter) {
		DrawAltimeter(p_mech, g_hudGaugePositions[1].m_x, g_hudGaugePositions[1].m_y);
	}
}

// DrawHudAt with the overlays at their default places, and the crosshair only with
// p_drawCrosshair (the view is the cockpit's).
void DrawHud(
	Mech* p_mech,
	int32_t p_heading,
	int32_t p_twist,
	int32_t p_bearing,
	int32_t p_twistBearing,
	int32_t p_pitch,
	int32_t p_distance,
	int32_t p_drawCrosshair
)
{
	if (!g_showHud) {
		return;
	}

	if (g_showCompass) {
		DrawCompass(0x73, 0x10, p_heading, p_twist);
		DrawCompassMarkers(p_mech, 0x73, 0x10, p_bearing, p_twistBearing, p_pitch);
	}

	if (p_drawCrosshair && g_showCrosshair) {
		DrawCrosshair(p_mech, p_twistBearing, p_pitch, p_distance);
	}

	if (g_showTargetMarker) {
		DrawTargetMarker(p_mech);
	}

	if (g_showAltimeter) {
		DrawAltimeter(p_mech, 8, 0x4a);
	}
}

// Draws the altimeter: the mech's height, the height of m_player's mark (Player::m_groundHeight) and
// the height of the target, clamped to the gauge. p_x and p_y go unused.
void DrawAltimeter(Mech* p_mech, int32_t p_x, int32_t p_y)
{
	CockpitPanel* gauge;
	int32_t height;
	Mech* mech;
	int32_t x;
	PANE* target;
	int32_t y;
	int32_t mark;
	int32_t unused;
	int32_t shape;
	int32_t level;

	gauge = g_cockpitPanels[c_panelAltimeter];
	target = g_cockpitPanels[c_panelAltimeter]->m_target;
	height = p_mech->m_player->m_position.m_y - p_mech->m_height;
	level = (MulDiv64(g_altimeterScale, height - 20100, 100) >> 16) + g_altimeterOrigin.m_y;
	if (g_altimeterOrigin.m_y < level) {
		DrawPaneShape(g_altimeterOrigin.m_x, g_altimeterOrigin.m_y, 0x115, target);
	}

	DrawPaneShape(g_altimeterOrigin.m_x, level, 7, target);
	DrawPaneShape(g_altimeterLevelX, g_altimeterOrigin.m_y, 1, target);
	mark = (MulDiv64(g_altimeterScale, height - p_mech->m_player->m_groundHeight, 100) >> 16) + g_altimeterOrigin.m_y;
	DrawPaneShape(g_altimeterGroundX, mark, 4, target);
	if (!(p_mech->m_player->m_targetInfo.m_target & 0xf00) || (p_mech->m_player->m_targetInfo.m_target & 0x1000)) {
		return;
	}

	x = g_altimeterTargetX;
	if ((p_mech->m_player->m_targetInfo.m_target & 0xf00) == 0x200) {
		mech = g_players[p_mech->m_player->m_targetInfo.m_target & 0xff]->m_mech;
		y = (MulDiv64(g_altimeterScale, height - (mech->m_player->m_position.m_y - mech->m_height), 100) >> 16) +
			g_altimeterOrigin.m_y;
	}
	else if ((p_mech->m_player->m_targetInfo.m_target & 0xf00) == 0x400) {
		GetStaticObjectPosition(
			g_gameThings[p_mech->m_player->m_targetInfo.m_target & 0xff].m_staticObject,
			&unused,
			&y,
			&unused
		);
		y = (MulDiv64(g_altimeterScale, height - y, 100) >> 16) + g_altimeterOrigin.m_y;
	}
	else if ((p_mech->m_player->m_targetInfo.m_target & 0xf00) == 0x100) {
		y = (MulDiv64(
				 g_altimeterScale,
				 height - g_navTable[p_mech->m_player->m_targetInfo.m_target & 0xff].m_position[1],
				 100
			 ) >>
			 16) +
			g_altimeterOrigin.m_y;
	}
	else {
		return;
	}

	if (y < 0) {
		y = 0;
		shape = 0x25;
		x += g_altimeterMarkWidth;
	}
	else if (gauge->m_height < y) {
		y = gauge->m_height;
		shape = 0x1c;
		x += g_altimeterMarkWidth;
	}
	else {
		shape = 0x1f;
	}

	DrawShapeOverPane(x, y, shape, target);
}

// Lays out the altimeter and the compass from their shapes' extents. Each shape is released by
// its extent plus its id, not by the id it was loaded with.
void InitHudGauges(void)
{
	void* shape;
	int32_t height;
	int32_t width;
	PANE* target;

	target = g_cockpitPanels[c_panelAltimeter]->m_target;
	ScalePointToFrame(target, &g_altimeterOrigin, &g_altimeterOrigin);
	shape = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		int32_t extent;

		extent = VFX_shape_resolution(shape, 0);
		g_altimeterMarkWidth = extent >> 16;
		g_altimeterMarkHeight = extent & 0xffff;
		UnlockCachedResource(extent + 1, g_resourceTypeTags[c_resTagShp]);
	}

	g_altimeterLevelX = g_altimeterOrigin.m_x;
	g_altimeterGroundX = g_altimeterMarkWidth + g_altimeterLevelX;
	g_altimeterTargetX = g_altimeterMarkWidth + g_altimeterGroundX;
	shape = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 7, g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		int32_t extent;

		extent = VFX_shape_resolution(shape, 0);
		width = extent >> 16;
		height = extent & 0xffff;
		g_altimeterOrigin.m_x -= width;
		UnlockCachedResource(extent + 7, g_resourceTypeTags[c_resTagShp]);
		g_altimeterScale = (height << 16) / 0xe8;
	}

	target = g_cockpitPanels[c_panelCompass]->m_target;
	ScalePointToFrame(target, &g_compassOrigin, &g_compassOrigin);
	shape = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 0x19, g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		int32_t extent;

		extent = VFX_shape_resolution(shape, 0);
		width = extent >> 16;
		height = extent & 0xffff;
		UnlockCachedResource(extent + 0x19, g_resourceTypeTags[c_resTagShp]);
		g_compassScale = (width << 16) / 0x168;
	}

	shape = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 0x13, g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		int32_t extent;
		int32_t origin;

		extent = VFX_shape_resolution(shape, 0);
		origin = VFX_shape_origin(shape, 0);
		extent &= 0xffff;
		origin &= 0xffff;
		g_compassTapeBelow = extent - origin;
		g_compassTapeAbove = origin;
		UnlockCachedResource(extent + 0x13, g_resourceTypeTags[c_resTagShp]);
	}

	shape = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 0x25, g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		int32_t extent;

		extent = VFX_shape_resolution(shape, 0);
		g_compassArrowWidth = extent >> 16;
		g_compassArrowHeight = extent & 0xffff;
		UnlockCachedResource(extent + 0x25, g_resourceTypeTags[c_resTagShp]);
	}

	shape = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 0x1f, g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		int32_t extent;

		extent = VFX_shape_resolution(shape, 0);
		g_compassSideArrowWidth = extent >> 16;
		g_compassSideArrowHeight = extent & 0xffff;
		UnlockCachedResource(extent + 0x1f, g_resourceTypeTags[c_resTagShp]);
	}
}

// Returns the HUD gauges' positions (the compass's, then the altimeter's).
Point* GetHudGaugePositions(void)
{
	return g_hudGaugePositions;
}

// Draws the crosshair at the end of the aim ray: for a selected weapon that follows its locked
// target, the locked shape when the target is in range and within 3 units and 3 degrees of the
// aim (returns TRUE then); otherwise the lock or guided-state shapes, or 0x67 without a ready
// weapon.
int32_t DrawCrosshair(Mech* p_mech, int32_t p_bearing, int32_t p_pitch, int32_t p_distance)
{
	int32_t x;
	int32_t pitch;
	int32_t shape;
	int32_t shapeOut;
	int32_t shapeIn;
	WeaponSlot* slot;
	WeaponDef* def;
	int32_t range;
	int32_t result;
	int32_t y;

	result = FALSE;
	shape = 0x76;
	slot = &p_mech->m_weapons[p_mech->m_selectedWeapon];
	def = &g_weaponDefs[slot->m_type];
	if (slot->m_state == 1) {
		if (def->m_shotType == 3) {
			shapeIn = 0x6a;
			shapeOut = 0x6d;
		}
		else {
			shapeOut = 0x76;
			shapeIn = 0x73;
		}

		if (def->m_guided == 0) {
			range = 0x30000;
			pitch = 3;
			if (p_mech->m_player->m_targetInfo.m_target && !(p_mech->m_player->m_targetInfo.m_target & 0x1100) &&
				def->m_shortRange < p_distance && def->m_longRange > p_distance && range > p_pitch &&
				-range < p_pitch && pitch > p_bearing && -pitch < p_bearing) {
				result = TRUE;
				shape = shapeIn;
			}
			else {
				shape = shapeOut;
			}
		}
		else if (p_mech->m_flags & 0x80) {
			shape = 0x61;
		}
		else if (p_mech->m_flags & 0x8000) {
			shape = 0x70;
		}
		else {
			shape = 0x6d;
		}
	}
	else {
		shape = 0x67;
	}

	if (!ProjectAimPoint(p_mech, &x, &y)) {
		return result;
	}

	DrawHudShape(x, y, shape);
	return result;
}

// Marks the player's target on the screen: a player's or a game thing's mech with its side's
// markers (DrawPlayerBrackets, DrawObjectBrackets), a nav with shape 0xe5, or 0xeb at the edge when it is
// off the screen.
void DrawTargetMarker(Mech* p_mech)
{
	int32_t index;
	Player* player;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t onScreen;
	int32_t offScreen;
	int32_t target;
	Point point;

	player = p_mech->m_player;
	target = player->m_targetInfo.m_target;
	if (!target || target & 0x1000) {
		return;
	}

	index = target & 0xff;
	target &= 0xf00;
	switch (target) {
	case 0x200:
		DrawPlayerBrackets(g_players[index], GetPlayerSide(index));
		return;
	case 0x400:
		DrawObjectBrackets(GetLocalTargetObject(), GetThingSide(index));
		return;
	case 0x100:
		onScreen = 0xe5;
		offScreen = 0xeb;
		break;
	default:
		return;
	}

	x = player->m_targetInfo.m_position.m_x;
	y = player->m_targetInfo.m_position.m_y;
	z = player->m_targetInfo.m_position.m_z;
	if (ProjectWorldPoint(&x, &y, &z)) {
		DrawHudShape(x, y, onScreen);
	}
	else {
		point.m_x = x;
		point.m_y = y;
		GetRectNeedleToward(&g_currentPane, &point, &point);
		DrawHudShape(point.m_x, point.m_y, offScreen);
	}
}

// Draws the compass's target markers: the target's bearing p_bearing from the heading (from the
// torso, p_twistBearing, unless the target is a nav point), and the arrows above and below while
// its pitch p_pitch is beyond 3 degrees. p_x and p_y go unused.
void DrawCompassMarkers(
	Mech* p_mech,
	int32_t p_x,
	int32_t p_y,
	int32_t p_bearing,
	int32_t p_twistBearing,
	int32_t p_pitch
)
{
	CockpitPanel* gauge;
	PANE* target;
	int32_t x;
	int32_t y;
	int32_t x2;
	int32_t y2;

	gauge = g_cockpitPanels[c_panelCompass];
	target = g_cockpitPanels[c_panelCompass]->m_target;
	if (!(p_mech->m_player->m_targetInfo.m_target & 0x100)) {
		p_bearing = p_twistBearing;
	}

	if (p_mech->m_autopilot != 2) {
		if (!(p_mech->m_player->m_targetInfo.m_target & 0xf00) || (p_mech->m_player->m_targetInfo.m_target & 0x1000)) {
			return;
		}

		if (p_pitch > -0x30000) {
			DrawShapeOverPane(
				g_compassOrigin.m_x,
				g_compassOrigin.m_y - g_compassArrowHeight - g_compassTapeAbove,
				0x25,
				target
			);
		}

		if (p_pitch < 0x30000) {
			DrawShapeOverPane(
				g_compassOrigin.m_x,
				g_compassArrowHeight + g_compassOrigin.m_y + g_compassTapeBelow,
				0x1c,
				target
			);
		}
	}

	if (p_bearing == 0) {
		DrawPaneShape(g_compassOrigin.m_x, g_compassOrigin.m_y, 0x10, target);
	}
	else {
		DrawPaneShape(g_compassOrigin.m_x + FixedMul16(g_compassScale, p_bearing), g_compassOrigin.m_y, 0xd, target);
	}

	if (p_bearing == 0) {
		DrawPaneShape(g_compassOrigin.m_x, g_compassOrigin.m_y, 0x16, target);
	}

	if (p_bearing > -3) {
		x = gauge->m_width + g_compassSideArrowWidth - 1;
		y = g_compassOrigin.m_y + g_compassSideArrowHeight / 2;
		DrawShapeOverPane(x, y, 0x22, target);
		if (p_bearing > 0x5a) {
			DrawShapeOverPane(x + 1, y, 0x22, target);
		}
	}

	if (p_bearing < 3) {
		x2 = -g_compassSideArrowWidth;
		y2 = g_compassOrigin.m_y + g_compassSideArrowHeight / 2;
		DrawShapeOverPane(x2, y2, 0x1f, target);
		if (p_bearing < -0x5a) {
			DrawShapeOverPane(x2 - 1, y2, 0x1f, target);
		}
	}
}

// Draws the compass tape at heading p_heading (degrees), twice to wrap around, and the torso twist
// p_twist as a bar from the center. p_x and p_y go unused.
void DrawCompass(int32_t p_x, int32_t p_y, int32_t p_heading, int32_t p_twist)
{
	CockpitPanel* gauge;
	int32_t x2;
	int32_t x;
	int32_t heading;
	PANE* target;
	int32_t offset;

	gauge = g_cockpitPanels[c_panelCompass];
	target = g_cockpitPanels[c_panelCompass]->m_target;
	heading = (p_heading + 360) % 360;
	x = g_compassOrigin.m_x + FixedMul16(g_compassScale, heading);
	if (gauge->m_width > x) {
		x2 = x + FixedMul16(g_compassScale, 360);
	}
	else {
		x2 = x - FixedMul16(g_compassScale, 360);
	}

	DrawPaneShape(x, g_compassOrigin.m_y, 0x19, target);
	DrawPaneShape(x2, g_compassOrigin.m_y, 0x19, target);
	if (p_twist) {
		offset = p_twist;
		if (offset < 0) {
			DrawHorizontalBar(
				target,
				g_compassOrigin.m_x + offset,
				g_compassOrigin.m_y - g_compassTapeAbove,
				-offset,
				g_compassTapeAbove,
				0xf
			);
		}
		else {
			DrawHorizontalBar(
				target,
				g_compassOrigin.m_x,
				g_compassOrigin.m_y - g_compassTapeAbove,
				offset,
				g_compassTapeAbove,
				0xf
			);
		}
	}

	DrawPaneShape(g_compassOrigin.m_x, g_compassOrigin.m_y, 0x13, target);
}

// Projects the end of the mech's player's aim ray to the screen: returns ProjectWorldPoint's result,
// and the point in p_x and p_y.
int32_t ProjectAimPoint(Mech* p_mech, int32_t* p_x, int32_t* p_y)
{
	int32_t result;
	Ray ray;
	int32_t z;
	int32_t y;
	int32_t x;

	BuildAimRay(p_mech->m_player, &ray);
	SetRayLength(&ray, GetAimRange(p_mech->m_player));
	x = ray.m_x1;
	y = ray.m_y1;
	z = ray.m_z1;
	result = ProjectWorldPoint(&x, &y, &z);
	*p_x = x;
	*p_y = y;
	return result;
}

// Marks player p_player on the screen: brackets at the corners of its mech, sized by its radius
// and depth, in its side's shapes; off the screen, an arrow at the edge.
void DrawPlayerBrackets(Player* p_player, int32_t p_side)
{
	int32_t topLeft;
	int32_t topRight;
	int32_t bottomLeft;
	int32_t bottomRight;
	int32_t sx;
	int32_t sy;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t size;
	Point point;

	x = p_player->m_position.m_x;
	y = p_player->m_position.m_y;
	z = p_player->m_position.m_z;
	if (!ProjectWorldPoint(&x, &y, &z)) {
		point.m_x = x;
		point.m_y = y;
		GetRectNeedleToward(&g_currentPane, &point, &point);
		switch (p_side) {
		case 0:
			topLeft = 0xeb;
			break;
		case 2:
			topLeft = 0xee;
			break;
		default:
			topLeft = 0xe8;
			break;
		}

		DrawHudShape(point.m_x, point.m_y, topLeft);
	}
	else {
		size = p_player->m_mech->m_radius;
		size = ProjectRadius(g_eyepoint->m_projectScaleX, size, z);
		switch (p_side) {
		case 0:
			topLeft = 0xb8;
			topRight = 0xc1;
			bottomLeft = 0xca;
			bottomRight = 0xd3;
			break;
		case 2:
			topLeft = 0xbe;
			topRight = 0xc7;
			bottomLeft = 0xd0;
			bottomRight = 0xd9;
			break;
		case 1:
			topLeft = 0xbb;
			topRight = 0xc4;
			bottomLeft = 0xcd;
			bottomRight = 0xd6;
			break;
		}

		sx = x - size;
		sy = y - size;
		DrawHudShape(sx, sy, topLeft);
		sx = x + size;
		sy = y - size;
		DrawHudShape(sx, sy, topRight);
		sx = x - size;
		sy = y + size;
		DrawHudShape(sx, sy, bottomLeft);
		sx = x + size;
		sy = y + size;
		DrawHudShape(sx, sy, bottomRight);
	}
}

// Marks the object p_object on the screen like DrawPlayerBrackets a player: brackets around its shape,
// at most half the screen apart, or an arrow at the edge.
void DrawObjectBrackets(struct SceneObject* p_object, int32_t p_side)
{
	int32_t maxSize;
	int32_t topLeft;
	int32_t topRight;
	int32_t bottomLeft;
	int32_t bottomRight;
	int32_t sx;
	int32_t sy;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t size;
	Point point;

	if (!p_object || !p_object->m_shape) {
		return;
	}

	size = GetShapeBounds(p_object->m_shape, &x, &y, &z);
	if (!ProjectWorldPoint(&x, &y, &z)) {
		point.m_x = x;
		point.m_y = y;
		GetRectNeedleToward(&g_currentPane, &point, &point);
		switch (p_side) {
		case 0:
			topLeft = 0xeb;
			break;
		case 2:
			topLeft = 0xee;
			break;
		default:
			topLeft = 0xe8;
			break;
		}

		DrawHudShape(point.m_x, point.m_y, topLeft);
	}
	else {
		size = ProjectRadius(g_eyepoint->m_projectScaleX, size >> 1, z);
		maxSize = g_eyepoint->m_halfWidth >> 1;
		if (size > maxSize) {
			size = maxSize;
		}
		switch (p_side) {
		case 0:
			topLeft = 0xb8;
			topRight = 0xc1;
			bottomLeft = 0xca;
			bottomRight = 0xd3;
			break;
		case 2:
			topLeft = 0xbe;
			topRight = 0xc7;
			bottomLeft = 0xd0;
			bottomRight = 0xd9;
			break;
		case 1:
			topLeft = 0xbb;
			topRight = 0xc4;
			bottomLeft = 0xcd;
			bottomRight = 0xd6;
			break;
		}

		sx = x - size;
		sy = y - size;
		DrawHudShape(sx, sy, topLeft);
		sx = x + size;
		sy = y - size;
		DrawHudShape(sx, sy, topRight);
		sx = x - size;
		sy = y + size;
		DrawHudShape(sx, sy, bottomLeft);
		sx = x + size;
		sy = y + size;
		DrawHudShape(sx, sy, bottomRight);
	}
}

// Draws frame 0 of the "SHP" resource p_id (relative to g_artResolution) at p_x, p_y of the
// current pane.
void DrawHudShape(int32_t p_x, int32_t p_y, int32_t p_id)
{
	void* shape;

	shape = LoadCachedResource(g_mw2PrjHandle, p_id + g_artResolution, g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		VFX_shape_draw(&g_currentPane, shape, 0, p_x, p_y);
		UnlockCachedResource(p_id + g_artResolution, g_resourceTypeTags[c_resTagShp]);
	}
}

// Draws frame 0 of the "SHP" resource p_id (relative to g_artResolution) at p_x, p_y.
void DrawPaneShape(int32_t p_x, int32_t p_y, int32_t p_id, PANE* p_target)
{
	void* shape;

	shape = LoadCachedResource(g_mw2PrjHandle, p_id + g_artResolution, g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		VFX_shape_draw(p_target, shape, 0, p_x, p_y);
		UnlockCachedResource(p_id + g_artResolution, g_resourceTypeTags[c_resTagShp]);
	}
}

// Draws DrawHudShape's shape at p_x, p_y of pane p_target.
void DrawShapeOverPane(int32_t p_x, int32_t p_y, int32_t p_id, PANE* p_target)
{
	DrawHudShape(p_target->m_x0 + p_x, p_target->m_y0 + p_y, p_id);
}
