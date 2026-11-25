#ifndef BOUNDBOX_H
#define BOUNDBOX_H

#include "shape.h"
#include "types.h"

// The bounding box of a shape's selected model (Shape::m_collisionData), from the
// transformed vertices. CreateShapeBoundBox allocates it inside a larger 0x78-byte block.
typedef struct BoundBox {
	int32_t m_minX;
	int32_t m_maxX;
	int32_t m_minY;
	int32_t m_maxY;
	int32_t m_minZ;
	int32_t m_maxZ;
} BoundBox;

// The functions and globals of boundbox.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	BoundBox* CreateBoundBox(void);
	void EnsureBoundBox(Shape* p_shape);
	BoundBox* CreateShapeBoundBox(Shape* p_shape);
	void AttachShapeBoundBox(Shape* p_shape);
	void ComputeModelBounds(
		Shape* p_shape,
		int32_t* p_minX,
		int32_t* p_maxX,
		int32_t* p_minY,
		int32_t* p_maxY,
		int32_t* p_minZ,
		int32_t* p_maxZ
	);
	void FreeBoundBox(Shape* p_shape);
	int32_t GetBoundBoxSize(Shape* p_shape);

#ifdef __cplusplus
}
#endif

#endif // BOUNDBOX_H
