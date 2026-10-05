#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "eyepoint.h"
#include "mappoint.h"
#include "projectedvertex.h"
#include "rendersettings.h"
#include "shape.h"
#include "types.h"

// The functions and globals of mapview.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_savedPalettePending;
	extern Eyepoint g_savedEyepoint;
	extern MechS32 g_mapViewMinX;
	extern MechS32 g_mapViewMaxX;
	extern MechS32 g_mapViewMaxY;
	extern MechS32 g_mapViewMinY;
	extern MechS32 g_mapViewNear;
	extern MechS32 g_mapViewFar;
	extern MechS32 g_mapViewScale;
	extern RenderSettings g_savedRenderSettings;

	void BeginMapView(MechS32* p_pose, MechS32 p_slot, MechS32 p_worldSpan, MechS32 p_far);
	void DrawMapViewScene(MechU32 p_flags);
	void EndMapView(void);
	MechS32 CullMapViewShape(Shape* p_shape);
	ProjectedVertex* ProjectMapViewVertex(ProjectedVertex* p_vertex);
	MechS32 ProjectMapPoint(MapPoint* p_point);

#ifdef __cplusplus
}
#endif

#endif // MAPVIEW_H
