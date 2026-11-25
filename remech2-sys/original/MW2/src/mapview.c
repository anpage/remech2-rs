#include "mapview.h"

#include "animation.h"
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

int32_t g_savedPalettePending;

Eyepoint g_savedEyepoint;

int32_t g_mapViewMinX;

int32_t g_mapViewMaxX;

int32_t g_mapViewMaxY;

int32_t g_mapViewMinY;

int32_t g_mapViewNear;

int32_t g_mapViewFar;

int32_t g_mapViewScale;

RenderSettings g_savedRenderSettings;

// Saves the eyepoint and the rendering settings, and sets up a view from p_pose (position, then
// rotation) of p_worldSpan units across pane p_slot, as far as p_far.
void BeginMapView(int32_t* p_pose, int32_t p_slot, int32_t p_worldSpan, int32_t p_far)
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
void DrawMapViewScene(uint32_t p_flags)
{
	if (p_flags & 1) {
		DrawSkyAndGround(g_eyepoint);
	}

	DrawShapeList(g_sceneShapes);
}

// Restores the eyepoint and the rendering settings BeginMapView saved.
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
int32_t CullMapViewShape(Shape* p_shape)
{
	int32_t y;
	int32_t z;
	int32_t radius;
	int32_t dist;
	int32_t dx;
	int32_t side;
	int32_t dy;
	int32_t height;
	int32_t dz;
	int32_t depth;
	int32_t x;

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
ProjectedVertex* ProjectMapViewVertex(ProjectedVertex* p_vertex)
{
	int32_t x;
	uint8_t outcode;
	int32_t y;

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
int32_t ProjectMapPoint(MapPoint* p_point)
{
	int32_t visible;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t dx;
	int32_t dy;
	int32_t dz;

	visible = false;
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
			visible = true;
		}
		else {
			visible = false;
		}
	}

	return visible;
}
