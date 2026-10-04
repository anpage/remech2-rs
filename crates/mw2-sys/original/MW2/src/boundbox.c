#include "boundbox.h"

#include "decomp.h"
#include "object.h"
#include "quadtree.h"
#include "shape.h"
#include "simmain.h"
#include "types.h"
#include "vertex.h"

DECOMP_SIZE_ASSERT(BoundBox, 0x18)

// FUNCTION: MW2 0x1006e970
BoundBox* CreateBoundBox(void)
{
	BoundBox* box;

	box = MechHeapAlloc(g_primaryHeap, sizeof(BoundBox));
	if (box) {
		box->m_minX = box->m_minY = box->m_minZ = 0;
		box->m_maxX = box->m_maxY = box->m_maxZ = 0;
	}

	return box;
}

// Gives the shape a bounding box, once.
// FUNCTION: MW2 0x1006e9e6
void EnsureBoundBox(Shape* p_shape)
{
	BoundBox* box;

	if (!p_shape) {
		return;
	}

	if (p_shape->m_flags & 0x200) {
		return;
	}

	if (!p_shape->m_collisionData) {
		p_shape->m_collisionData = CreateBoundBox();
	}

	box = p_shape->m_collisionData;
	if (!box) {
		return;
	}

	ComputeModelBounds(p_shape, &box->m_minX, &box->m_maxX, &box->m_minY, &box->m_maxY, &box->m_minZ, &box->m_maxZ);
	p_shape->m_flags |= 0x200;
}

// FUNCTION: MW2 0x1006ea90
BoundBox* CreateShapeBoundBox(Shape* p_shape)
{
	BoundBox* box;

	box = MechHeapAlloc(g_primaryHeap, 0x78);
	if (box) {
		memset(box, 0, 0x78);
		ComputeModelBounds(p_shape, &box->m_minX, &box->m_maxX, &box->m_minY, &box->m_maxY, &box->m_minZ, &box->m_maxZ);
	}

	return box;
}

// FUNCTION: MW2 0x1006eb02
void AttachShapeBoundBox(Shape* p_shape)
{
	BoundBox* box;

	if (!p_shape) {
		return;
	}

	if (p_shape->m_flags & 0x200) {
		return;
	}

	if (!p_shape->m_collisionData) {
		p_shape->m_collisionData = CreateShapeBoundBox(p_shape);
	}

	box = p_shape->m_collisionData;
	if (!box) {
		return;
	}

	p_shape->m_flags |= 0x200;
}

// Computes the bounding box of the shape's first model, transforming it first if it is out of date.
// The model/shape stamp comparison loads its operands in the other order (the unit's symbol
// table), and model, vertex and selected sit in permuted stack slots.
// FUNCTION: MW2 0x1006eb80
void ComputeModelBounds(
	Shape* p_shape,
	MechS32* p_minX,
	MechS32* p_maxX,
	MechS32* p_minY,
	MechS32* p_maxY,
	MechS32* p_minZ,
	MechS32* p_maxZ
)
{
	Model* model;
	Vertex* vertex;
	MechS32 count;
	Model* selected;

	model = p_shape->m_models;
	if (!model) {
		*p_minX = *p_maxX = *p_minY = *p_maxY = *p_minZ = *p_maxZ = 0;
		return;
	}

	if (model->m_transformCount != p_shape->m_transformCount) {
		selected = p_shape->m_model;
		p_shape->m_model = model;
		TransformShapeModel(p_shape);
		p_shape->m_model = selected;
	}

	vertex = (Vertex*) (model + 1);
	*p_minX = *p_minY = *p_minZ = 0x7fffffff;
	*p_maxX = *p_maxY = *p_maxZ = -0x7fffffff;
	for (count = model->m_vertexCount; count--; vertex++) {
		if (vertex->m_worldX > *p_maxX) {
			*p_maxX = vertex->m_worldX;
		}

		if (vertex->m_worldY > *p_maxY) {
			*p_maxY = vertex->m_worldY;
		}

		if (vertex->m_worldZ > *p_maxZ) {
			*p_maxZ = vertex->m_worldZ;
		}

		if (vertex->m_worldX < *p_minX) {
			*p_minX = vertex->m_worldX;
		}

		if (vertex->m_worldY < *p_minY) {
			*p_minY = vertex->m_worldY;
		}

		if (vertex->m_worldZ < *p_minZ) {
			*p_minZ = vertex->m_worldZ;
		}
	}
}

// Frees the shape's bounding data.
// FUNCTION: MW2 0x1006ed30
void FreeBoundBox(Shape* p_shape)
{
	void* data;

	if (!p_shape) {
		return;
	}

	data = p_shape->m_collisionData;
	if (!data) {
		return;
	}

	switch (p_shape->m_collisionType) {
	case 0:
		MechHeapFree(g_primaryHeap, data);
		break;
	case 5:
		FreeQuadtree(data);
		break;
	}

	p_shape->m_collisionData = NULL;
}

// Returns the bytes the shape's bounding data take.
// FUNCTION: MW2 0x1006edc3
MechS32 GetBoundBoxSize(Shape* p_shape)
{
	MechS32 size;
	void* data;

	if (!p_shape) {
		return 0;
	}

	data = p_shape->m_collisionData;
	if (!data) {
		return 0;
	}

	switch (p_shape->m_collisionType) {
	case 0:
		size = 0x78;
		break;
	case 5:
		size = GetQuadtreeSize(data);
		break;
	default:
		size = 0;
	}

	return size;
}
