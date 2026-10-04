#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "mappoint.h"
#include "projectedvertex.h"
#include "shape.h"
#include "types.h"

// The functions and globals of mapview.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

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
