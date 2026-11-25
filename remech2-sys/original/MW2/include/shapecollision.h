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

	extern int32_t g_rayBoxEntryBehind;

	int32_t TestPointInBox(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t GetBoxTop(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z, int32_t* p_top);
	void ClassifyPointInBox(
		Shape* p_shape,
		int32_t p_x,
		int32_t p_y,
		int32_t p_z,
		int32_t* p_inside,
		int32_t* p_inColumn,
		int32_t* p_top
	);
	int32_t TestRayBox(Shape* p_shape, Ray* p_ray);
	int32_t TestPointInBoxColumn(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t TestPointNever(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t TestRayNever(Shape* p_shape, Ray* p_ray);
	int32_t TestPointUnderFloor(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t TestPointTerrain(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t TestRayTerrain(Shape* p_shape, Ray* p_ray);
	int32_t GetTerrainShapeTop(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z, int32_t* p_top);
	int32_t TestPointAboveFaces(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t TestRayFaces(Shape* p_shape, Ray* p_ray);

#ifdef __cplusplus
}
#endif

#endif // SHAPECOLLISION_H
