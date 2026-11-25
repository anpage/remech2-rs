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

	extern int32_t g_savedPalettePending;
	extern Eyepoint g_savedEyepoint;
	extern int32_t g_mapViewMinX;
	extern int32_t g_mapViewMaxX;
	extern int32_t g_mapViewMaxY;
	extern int32_t g_mapViewMinY;
	extern int32_t g_mapViewNear;
	extern int32_t g_mapViewFar;
	extern int32_t g_mapViewScale;
	extern RenderSettings g_savedRenderSettings;

	void BeginMapView(int32_t* p_pose, int32_t p_slot, int32_t p_worldSpan, int32_t p_far);
	void DrawMapViewScene(uint32_t p_flags);
	void EndMapView(void);
	int32_t CullMapViewShape(Shape* p_shape);
	ProjectedVertex* ProjectMapViewVertex(ProjectedVertex* p_vertex);
	int32_t ProjectMapPoint(MapPoint* p_point);

#ifdef __cplusplus
}
#endif

#endif // MAPVIEW_H
