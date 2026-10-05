#ifndef SHAPECOLLISION_H
#define SHAPECOLLISION_H

#include "ray.h"
#include "shape.h"
#include "types.h"

// The functions and globals of shapecollision.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_rayBoxEntryBehind;

	MechS32 TestPointInBox(Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 GetBoxTop(Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32* p_top);
	void ClassifyPointInBox(
		Shape* p_shape,
		MechS32 p_x,
		MechS32 p_y,
		MechS32 p_z,
		MechS32* p_inside,
		MechS32* p_inColumn,
		MechS32* p_top
	);
	MechS32 TestRayBox(Shape* p_shape, Ray* p_ray);
	MechS32 TestPointInBoxColumn(Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 TestPointNever(Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 TestRayNever(Shape* p_shape, Ray* p_ray);
	MechS32 TestPointUnderFloor(Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 TestPointTerrain(Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 TestRayTerrain(Shape* p_shape, Ray* p_ray);
	MechS32 GetTerrainShapeTop(Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32* p_top);
	MechS32 TestPointAboveFaces(Shape* p_shape, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 TestRayFaces(Shape* p_shape, Ray* p_ray);

#ifdef __cplusplus
}
#endif

#endif // SHAPECOLLISION_H
