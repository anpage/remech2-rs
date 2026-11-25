#include "gridobject.h"

#include "eyepoint.h"
#include "object.h"
#include "polydraw.h"
#include "shape.h"
#include "shapegeom.h"
#include "types.h"
#include "vector3.h"
#include "vertex.h"

#include <stdlib.h>

// The grid SetGridObject sets up: its cell size, whether the object is on, has been shown and
// snaps to the nearest cell.

int32_t g_gridCellSize = 0;

int32_t g_gridObjectSet = 0;

int32_t g_gridObjectPlaced = 0;

int32_t g_gridObjectShown = 0;

int32_t g_gridObjectSnaps = 0;

SceneObject* g_gridObject = NULL;

// The eyepoint's cell and the object's position.

Vector3 g_gridCell;

Vector3 g_gridAnchor;

// How far the eyepoint may stray from the object before it snaps to another cell.
int32_t g_gridSnapDistance;

// Makes p_obj the object that follows the eyepoint on a grid, a cell at a time: the cells are its
// model's span (with children) or a third of it, in steps of 0x4000, and it is snapped to the
// nearest cell once the eyepoint strays past 9/16 of a cell (with children) or kept on the cell
// under the eyepoint.
void SetGridObject(SceneObject* p_obj)
{
	int32_t depth;
	int32_t minX;
	Model* model;
	int32_t minZ;
	Vertex* vertex;
	int32_t span;
	int32_t count;
	int32_t maxX;
	int32_t maxZ;

	if (!p_obj) {
		return;
	}

	if (!p_obj->m_shape) {
		return;
	}

	model = p_obj->m_shape->m_models;
	if (!model) {
		return;
	}

	vertex = (Vertex*) (model + 1);
	minX = minZ = 0x7fffffff;
	maxX = maxZ = -0x7fffffff;
	for (count = model->m_vertexCount; count--; vertex++) {
		if (vertex->m_modelX > maxX) {
			maxX = vertex->m_modelX;
		}

		if (vertex->m_modelZ > maxZ) {
			maxZ = vertex->m_modelZ;
		}

		if (vertex->m_modelX < minX) {
			minX = vertex->m_modelX;
		}

		if (vertex->m_modelZ < minZ) {
			minZ = vertex->m_modelZ;
		}
	}

	span = maxX - minX;
	depth = maxZ - minZ;
	if (depth > span) {
		span = depth;
	}

	if (span <= 0) {
		return;
	}

	if (!p_obj->m_firstChild) {
		g_gridCellSize = (span / 3 / 0x4000 + 1) * 0x4000;
		g_gridObjectSnaps = 0;
	}
	else {
		g_gridCellSize = (span / 0x4000 + 1) * 0x4000;
		g_gridObjectSnaps = 1;
	}

	g_gridSnapDistance = ((g_gridCellSize >> 3) + g_gridCellSize) >> 1;
	g_gridObject = p_obj;
	g_gridObjectSet = 1;
	g_gridObjectShown = 1;
	g_gridObjectPlaced = 0;
	g_gridAnchor.m_x = g_gridAnchor.m_y = g_gridAnchor.m_z = 0;
	g_gridCell.m_x = g_gridCell.m_y = g_gridCell.m_z = 0;
	HideObjTree(p_obj);
	DisableObjTreeCollision(p_obj);
}

// Moves the grid object (SetGridObject) to the eyepoint's cell when that changes.
void UpdateGridObject(void)
{
	int32_t dx;
	int32_t dz;
	int32_t halfZ;
	int32_t halfX;
	int32_t x;
	int32_t cellZ;
	int32_t cellX;
	int32_t z;

	if (g_gridObjectShown && g_gridObjectSet) {
		x = g_eyepoint->m_x;
		z = g_eyepoint->m_z;
		if (g_gridObjectSnaps) {
			dx = abs(x - g_gridAnchor.m_x);
			dz = abs(z - g_gridAnchor.m_z);
			if (dx > g_gridSnapDistance || dz > g_gridSnapDistance) {
				if (x >= 0) {
					halfX = g_gridCellSize >> 1;
				}
				else {
					halfX = -g_gridCellSize >> 1;
				}

				if (z >= 0) {
					halfZ = g_gridCellSize >> 1;
				}
				else {
					halfZ = -g_gridCellSize >> 1;
				}

				cellX = (x + halfX) / g_gridCellSize;
				cellZ = (z + halfZ) / g_gridCellSize;
			}
			else {
				cellX = g_gridCell.m_x;
				cellZ = g_gridCell.m_z;
			}
		}
		else {
			cellX = x / g_gridCellSize;
			cellZ = z / g_gridCellSize;
		}

		if (!g_gridObjectPlaced) {
			ShowObjTree(g_gridObject);
			g_gridObjectPlaced = 1;
		}

		if (g_gridCell.m_x != cellX || g_gridCell.m_z != cellZ) {
			g_gridAnchor.m_x = cellX * g_gridCellSize;
			g_gridAnchor.m_y = 0;
			g_gridAnchor.m_z = cellZ * g_gridCellSize;
			if (g_gridObject) {
				ShowObjTree(g_gridObject);
				SetObjPosition(g_gridObject, g_gridAnchor.m_x, g_gridAnchor.m_y, g_gridAnchor.m_z);
				UpdateObj(g_gridObject);
			}
		}

		g_gridCell.m_x = cellX;
		g_gridCell.m_y = 0;
		g_gridCell.m_z = cellZ;
	}
}

void ShowGridObject(int32_t p_enable)
{
	if (g_gridObject) {
		if (p_enable) {
			ShowObjTree(g_gridObject);
		}
		else {
			HideObjTree(g_gridObject);
		}

		UpdateObj(g_gridObject);
		g_gridObjectShown = p_enable;
	}
}
