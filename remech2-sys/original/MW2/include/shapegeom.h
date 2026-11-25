#ifndef SHAPEGEOM_H
#define SHAPEGEOM_H

#include "decomp.h"
#include "model.h"
#include "transform.h"
#include "types.h"

struct Ray;
struct Shape;

// The functions of shapegeom.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void TransformModel(Model* p_model, Matrix* p_matrix);
	void TransformShapeCenter(struct Shape* p_shape, Matrix* p_matrix);
	void TransformShape(struct Shape* p_shape, Matrix* p_matrix);
	int32_t SolvePlaneY(
		int32_t p_normalX,
		int32_t p_normalY,
		int32_t p_normalZ,
		int32_t p_distance,
		int32_t p_dx,
		int32_t p_dz
	);
	int32_t ApproximateShapeDistance(struct Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t ComputeTriangleNormal(
		int32_t p_x0,
		int32_t p_y0,
		int32_t p_z0,
		int32_t p_x1,
		int32_t p_y1,
		int32_t p_z1,
		int32_t p_x2,
		int32_t p_y2,
		int32_t p_z2,
		int32_t* p_nx,
		int32_t* p_ny,
		int32_t* p_nz
	);
	int32_t RayShapeDistance(struct Shape* p_shape, struct Ray* p_ray);

#ifdef __cplusplus
}
#endif

#endif // SHAPEGEOM_H
