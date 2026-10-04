#include "gridobject.h"

#include "decomp.h"
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

// GLOBAL: MW2 0x100a7114
MechS32 g_gridCellSize = 0;

// GLOBAL: MW2 0x100a7118
MechS32 g_gridObjectSet = 0;

// GLOBAL: MW2 0x100a711c
MechS32 g_gridObjectPlaced = 0;

// GLOBAL: MW2 0x100a7120
MechS32 g_gridObjectShown = 0;

// GLOBAL: MW2 0x100a7124
MechS32 g_gridObjectSnaps = 0;

// GLOBAL: MW2 0x100a7128
SceneObject* g_gridObject = NULL;

// The eyepoint's cell and the object's position.

// GLOBAL: MW2 0x100be9e0
static Vector3 g_gridCell;

// GLOBAL: MW2 0x100be9f0
static Vector3 g_gridAnchor;

// How far the eyepoint may stray from the object before it snaps to another cell.
// GLOBAL: MW2 0x100be9fc
static MechS32 g_gridSnapDistance;

// Makes p_obj the object that follows the eyepoint on a grid, a cell at a time: the cells are its
// model's span (with children) or a third of it, in steps of 0x4000, and it is snapped to the
// nearest cell once the eyepoint strays past 9/16 of a cell (with children) or kept on the cell
// under the eyepoint.
// Stack-slot permutation; depth > span compares in the other operand order.
// FUNCTION: MW2 0x1004b130
void SetGridObject(SceneObject* p_obj)
{
	MechS32 depth;
	MechS32 minX;
	Model* model;
	MechS32 minZ;
	Vertex* vertex;
	MechS32 span;
	MechS32 count;
	MechS32 maxX;
	MechS32 maxZ;

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
// Stack-slot permutation; dz > g_gridSnapDistance compares in the other operand order.
// FUNCTION: MW2 0x1004b344
void UpdateGridObject(void)
{
	MechS32 dx;
	MechS32 dz;
	MechS32 halfZ;
	MechS32 halfX;
	MechS32 x;
	MechS32 cellZ;
	MechS32 cellX;
	MechS32 z;

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

// FUNCTION: MW2 0x1004b539
void ShowGridObject(MechS32 p_enable)
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
