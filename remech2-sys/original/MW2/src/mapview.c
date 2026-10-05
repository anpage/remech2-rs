#include "mapview.h"

#include "animation.h"
#include "decomp.h"
#include "depthsort.h"
#include "eyepoint.h"
#include "fixeddot27.h"
#include "fixeddot29.h"
#include "objectanim.h"
#include "palette.h"
#include "polydraw.h"
#include "recordstacks.h"
#include "render.h"
#include "rendersettings.h"
#include "shapelists.h"
#include "shiftdiv.h"
#include "targeting.h"
#include "types.h"
#include "view.h"
#include "widescreen.h"

// The map (satellite) view's projection: a top-down view of p_worldSpan units across.

// GLOBAL: MW2 0x10109ab0
MechS32 g_savedPalettePending;

// GLOBAL: MW2 0x10109ac0
Eyepoint g_savedEyepoint;

// GLOBAL: MW2 0x10109ba0
MechS32 g_mapViewMinX;

// GLOBAL: MW2 0x10109ba4
MechS32 g_mapViewMaxX;

// GLOBAL: MW2 0x10109ba8
MechS32 g_mapViewMaxY;

// GLOBAL: MW2 0x10109bac
MechS32 g_mapViewMinY;

// GLOBAL: MW2 0x10109bb0
MechS32 g_mapViewNear;

// GLOBAL: MW2 0x10109bb4
MechS32 g_mapViewFar;

// GLOBAL: MW2 0x10109bb8
MechS32 g_mapViewScale;

// GLOBAL: MW2 0x10109bc0
RenderSettings g_savedRenderSettings;

// Saves the eyepoint and the rendering settings, and sets up a view from p_pose (position, then
// rotation) of p_worldSpan units across pane p_slot, as far as p_far.
// FUNCTION: MW2 0x10041fa0
void BeginMapView(MechS32* p_pose, MechS32 p_slot, MechS32 p_worldSpan, MechS32 p_far)
{
	p_worldSpan = MechWidenMapSpan(p_slot, p_worldSpan);
	g_mapViewScale = p_worldSpan / (g_panes[p_slot].m_x1 - g_panes[p_slot].m_x0 + 1);
	g_mapViewMaxX = -(g_mapViewMinX = -(p_worldSpan / 2));
	g_mapViewMinY = -(g_mapViewMaxY = (g_panes[p_slot].m_y1 - g_panes[p_slot].m_y0 + 1) * g_mapViewScale / 2);
	g_mapViewNear = 0;
	g_mapViewFar = p_far;
	g_savedPalettePending = g_palettePending;
	g_savedEyepoint = *g_eyepoint;
	g_eyepoint->m_heading = p_pose[3];
	g_eyepoint->m_pitch = p_pose[4];
	g_eyepoint->m_roll = p_pose[5];
	g_eyepoint->m_x = p_pose[0];
	g_eyepoint->m_y = p_pose[1];
	g_eyepoint->m_z = p_pose[2];
	SelectPane(p_slot);
	g_eyepoint->m_farPlane = g_mapViewFar;
	g_savedRenderSettings = g_renderSettings;
	g_renderSettings.m_shapeFilter = CullMapViewShape;
	g_renderSettings.m_projectVertex = ProjectMapViewVertex;
	UpdateProjection(g_eyepoint);
	g_eyepoint->m_projectScaleX16 = 0x2000;
	g_eyepoint->m_projectShiftX = 3;
	g_eyepoint->m_projectScaleY16 = g_eyepoint->m_pixelAspect >> 3;
	g_eyepoint->m_projectShiftY = 3;
	UpdateViewMatrix(g_eyepoint);
	SelectEyepoint(g_eyepoint);
	g_projectionDirty = 0;
}

// Draws the map view's scene: the terrain when bit 0 of p_flags is set, then the shapes.
// FUNCTION: MW2 0x1004215f
void DrawMapViewScene(MechU32 p_flags)
{
	if (p_flags & 1) {
		DrawSkyAndGround(g_eyepoint);
	}

	DrawShapeList(g_sceneShapes);
	FUN_10069591();
}

// Restores the eyepoint and the rendering settings BeginMapView saved.
// FUNCTION: MW2 0x10042195
void EndMapView(void)
{
	g_renderSettings = g_savedRenderSettings;
	*g_eyepoint = g_savedEyepoint;
	g_palettePending = g_savedPalettePending;
	ResetPane();
	UpdateProjection(g_eyepoint);
	UpdateViewMatrix(g_eyepoint);
	SelectEyepoint(g_eyepoint);
	g_projectionDirty = 0;
}

// Culls a shape against the map view's frustum: 1 hidden, 4 in front of the near plane, 5 past
// the far plane, 6 and 7 outside the side planes, 0 visible. Keeps its depth in g_queueDepth.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10042206
MechS32 CullMapViewShape(Shape* p_shape)
{
	MechS32 y;
	MechS32 z;
	MechS32 radius;
	MechS32 dist;
	MechS32 dx;
	MechS32 side;
	MechS32 dy;
	MechS32 height;
	MechS32 dz;
	MechS32 depth;
	MechS32 x;

	if (p_shape->m_flags & 0x1000) {
		return 1;
	}

	x = p_shape->m_centerX;
	y = p_shape->m_centerY;
	z = p_shape->m_centerZ;
	radius = p_shape->m_radius;
	dx = x - g_viewEyeX;
	dy = y - g_viewEyeY;
	dz = z - g_viewEyeZ;
	depth = g_queueDepth = FixedDot29(dx, g_viewRotZ0, dy, g_viewRotZ1, dz, g_viewRotZ2);
	if (depth + radius < g_viewNearPlane) {
		return 4;
	}

	if (depth - radius > g_viewFarPlane) {
		return 5;
	}

	side = FixedDot29(dx, g_viewRotX0, dy, g_viewRotX1, dz, g_viewRotX2);
	if (side > 0) {
		dist = side - g_mapViewMaxX;
	}
	else {
		dist = g_mapViewMinX - side;
	}

	if (dist > radius) {
		return 6;
	}

	height = FixedDot29(dx, g_viewRotY0, dy, g_viewRotY1, dz, g_viewRotY2);
	if (height > 0) {
		dist = height - g_mapViewMaxY;
	}
	else {
		dist = g_mapViewMinY - height;
	}

	if (dist > radius) {
		return 7;
	}

	return 0;
}

// Projects a vertex onto the map view (ProjectCoordinate's scaling) once per frame, with its clip
// outcodes, and adds it to the polygon being built: ProjectVertex's map-view counterpart.
// Stack-slot permutation: outcode and y.
// FUNCTION: MW2 0x100423b3
ProjectedVertex* ProjectMapViewVertex(ProjectedVertex* p_vertex)
{
	MechS32 x;
	MechU8 outcode;
	MechS32 y;

	outcode = 0;
	if (!p_vertex->m_projected) {
		x = p_vertex->m_x;
		y = p_vertex->m_y;
		x = ProjectCoordinate(x, g_mapViewScale, g_viewShiftX, g_viewCenterX);
		y = g_viewBottom - g_viewTop - ProjectCoordinate(y, g_mapViewScale, g_viewShiftY, g_viewCenterY);
		if (x - g_viewLeft < 0) {
			outcode |= 1;
		}

		if (x - g_viewRight > 0) {
			outcode |= 2;
		}

		if (y - g_viewTop < 0) {
			outcode |= 4;
		}

		if (y - g_viewBottom > 0) {
			outcode |= 8;
		}

		p_vertex->m_screenX = x;
		p_vertex->m_screenY = y;
		p_vertex->m_outcode = outcode;
		p_vertex->m_projected = 1;
	}

	g_polygonOrCodes |= outcode;
	g_polygonAndCodes &= outcode;
	if (g_polygonPointCount >= 20) {
		g_queueHasRoom = 0;
	}
	else {
		g_polygonPoints[g_polygonPointCount] = p_vertex;
		g_polygonPointCount++;
	}

	return p_vertex;
}

// Projects p_point onto the map view in place. Returns whether it is in front of the eye and
// on the screen.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x1004251e
MechS32 ProjectMapPoint(MapPoint* p_point)
{
	MechS32 visible;
	MechS32 x;
	MechS32 y;
	MechS32 z;
	MechS32 dx;
	MechS32 dy;
	MechS32 dz;

	visible = FALSE;
	x = p_point->m_xy.m_x;
	y = p_point->m_xy.m_y;
	z = p_point->m_z;
	dx = x - g_viewEyeX;
	dy = y - g_viewEyeY;
	dz = z - g_viewEyeZ;
	x = FixedDot27(dx, g_viewProjX0, dy, g_viewProjX1, dz, g_viewProjX2);
	y = FixedDot27(dx, g_viewProjY0, dy, g_viewProjY1, dz, g_viewProjY2);
	z = FixedDot27(dx, g_viewProjZ0, dy, g_viewProjZ1, dz, g_viewProjZ2);
	p_point->m_xy.m_x = ProjectCoordinate(x, g_mapViewScale, g_viewShiftX, g_viewCenterX);
	p_point->m_xy.m_y = g_viewBottom - g_viewTop - ProjectCoordinate(y, g_mapViewScale, g_viewShiftY, g_viewCenterY);
	p_point->m_z = z;
	if (z > 0) {
		if (p_point->m_xy.m_x >= g_viewLeft && p_point->m_xy.m_x <= g_viewRight && p_point->m_xy.m_y >= g_viewTop &&
			p_point->m_xy.m_y <= g_viewBottom) {
			visible = TRUE;
		}
		else {
			visible = FALSE;
		}
	}

	return visible;
}
