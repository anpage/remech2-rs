#include "shapecollision.h"

#include "boundbox.h"
#include "collision.h"
#include "face.h"
#include "quadtree.h"
#include "ray.h"
#include "shape.h"
#include "shapegeom.h"
#include "types.h"
#include "vertex.h"

/* The collision tests of the shape types (collision.c's g_shapeCollisionFns): a point or a
   ray against a shape's bounding box, its floor or ceiling faces, or the quadtree of a
   terrain shape. Each hit leaves the surface normal in g_hitNormal. */

int32_t g_rayBoxEntryBehind = 0;

int32_t TestPointInBox(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t inColumn;
	int32_t top;
	int32_t inside;

	ClassifyPointInBox(p_shape, p_x, p_y, p_z, &inside, &inColumn, &top);
	return inside;
}

// Tests whether (p_x, p_z) lies over the shape's bounding box; if it does, returns the top in
// p_top and an upward normal.
int32_t GetBoxTop(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z, int32_t* p_top)
{
	int32_t top;
	int32_t inColumn;
	int32_t inside;

	inColumn = 0;
	ClassifyPointInBox(p_shape, p_x, p_y, p_z, &inside, &inColumn, &top);
	if (inColumn) {
		*p_top = top;
		g_hitNormalX = g_hitNormalZ = 0;
		g_hitNormalY = 0x10000;
	}

	return inColumn;
}

// Tests the point against the shape's bounding box: p_inColumn is set when (p_x, p_z) lies
// within it, with the box's top in p_top, and p_inside when p_y does as well.
void ClassifyPointInBox(
	Shape* p_shape,
	int32_t p_x,
	int32_t p_y,
	int32_t p_z,
	int32_t* p_inside,
	int32_t* p_inColumn,
	int32_t* p_top
)
{
	BoundBox* box;

	EnsureBoundBox(p_shape);
	box = p_shape->m_collisionData;
	if (!box) {
		*p_inColumn = 0;
		*p_inside = 0;
	}
	else if (p_x >= box->m_minX && p_x <= box->m_maxX && p_z >= box->m_minZ && p_z <= box->m_maxZ) {
		*p_inColumn = 1;
		*p_top = box->m_maxY;
		if (p_y >= box->m_minY && p_y <= box->m_maxY) {
			*p_inside = 1;
		}
		else {
			*p_inside = 0;
		}
	}
	else {
		*p_inColumn = 0;
		*p_inside = 0;
	}
}

// Clips the ray against the shape's bounding box, one slab per axis. On a hit, advances the
// ray's start to the entry point and sets the normal of the face it entered through.
int32_t TestRayBox(Shape* p_shape, Ray* p_ray)
{
	int32_t tNear;
	int32_t axis;
	int32_t tEnter;
	BoundBox* box;
	int32_t tFar;
	int32_t tExit;

	EnsureBoundBox(p_shape);
	box = p_shape->m_collisionData;
	if (!box) {
		return 0;
	}

	if (ClipRaySlab(p_ray->m_x0, p_ray->m_dirX, box->m_minX, box->m_maxX, &tNear, &tFar)) {
		return 0;
	}

	axis = 0;
	if (ClipRaySlab(p_ray->m_y0, p_ray->m_dirY, box->m_minY, box->m_maxY, &tEnter, &tExit)) {
		return 0;
	}

	if (tNear < tEnter) {
		axis = 1;
		tNear = tEnter;
	}
	if (tFar > tExit) {
		tFar = tExit;
	}

	if (ClipRaySlab(p_ray->m_z0, p_ray->m_dirZ, box->m_minZ, box->m_maxZ, &tEnter, &tExit)) {
		return 0;
	}

	if (tNear < tEnter) {
		tNear = tEnter;
		axis = 2;
	}
	if (tFar > tExit) {
		tFar = tExit;
	}

	if (tFar < tNear) {
		return 0;
	}

	if (tNear < 0) {
		g_rayBoxEntryBehind = tNear;
		tNear = 0;
	}

	tEnter = GetRayLength(p_ray);
	if (tFar > tEnter) {
		tFar = tEnter;
	}

	if (tFar < tNear) {
		return 0;
	}

	SetRayLength(p_ray, tNear);
	switch (axis) {
	case 0:
		g_hitNormalY = g_hitNormalZ = 0;
		if (p_ray->m_dx > 0) {
			g_hitNormalX = -0x10000;
		}
		else {
			g_hitNormalX = 0x10000;
		}
		break;
	case 1:
		g_hitNormalX = g_hitNormalZ = 0;
		if (p_ray->m_dy > 0) {
			g_hitNormalY = -0x10000;
		}
		else {
			g_hitNormalY = 0x10000;
		}
		break;
	case 2:
		g_hitNormalX = g_hitNormalY = 0;
		if (p_ray->m_dz > 0) {
			g_hitNormalZ = -0x10000;
		}
		else {
			g_hitNormalZ = 0x10000;
		}
		break;
	}

	return 1;
}

// Tests whether (p_x, p_z) lies over the shape's bounding box.
int32_t TestPointInBoxColumn(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z)
{
	BoundBox* box;

	EnsureBoundBox(p_shape);
	box = p_shape->m_collisionData;
	if (!box) {
		return 0;
	}

	return p_x >= box->m_minX && p_x <= box->m_maxX && p_z >= box->m_minZ && p_z <= box->m_maxZ;
}

int32_t TestPointNever(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z)
{
	return 0;
}

int32_t TestRayNever(Shape* p_shape, Ray* p_ray)
{
	return 0;
}

// Finds the upward-facing face under (p_x, p_z) and tests whether p_y lies below it.
int32_t TestPointUnderFloor(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t i;
	int32_t height;
	int32_t done;
	Face* face;

	i = 0;
	done = FALSE;
	while (!done) {
		face = (Face*) ((uint8_t*) p_shape->m_model + p_shape->m_model->m_faceOffset) + i;
		if (face->m_normal[1] > 0 && IsPointInFaceXZ(face, (Vertex*) (p_shape->m_model + 1), p_x, p_z)) {
			done = TRUE;
			if (IsBelowFacePlane(face, (Vertex*) (p_shape->m_model + 1), p_x, p_y, p_z, &height)) {
				return 1;
			}
			else {
				return 0;
			}
		}
		else {
			i++;
			if (i == p_shape->m_model->m_faceCount) {
				done = TRUE;
			}
		}
	}

	return 0;
}

int32_t TestPointTerrain(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z)
{
	Model* model;

	model = p_shape->m_models;
	if (p_shape->m_collisionData) {
		return ClassifyQuadtreePoint(p_shape->m_collisionData, model, p_x, p_y, p_z) & 1;
	}
	else {
		return TestPointUnderFloor(p_shape, p_x, p_y, p_z);
	}
}

int32_t TestRayTerrain(Shape* p_shape, Ray* p_ray)
{
	return TestQuadtreeRay(p_shape->m_collisionData, p_shape->m_models, p_ray) & 1;
}

int32_t GetTerrainShapeTop(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z, int32_t* p_top)
{
	return GetQuadtreeTop(p_shape->m_collisionData, p_shape->m_models, p_x, p_y, p_z, p_top);
}

// Finds the downward-facing face over (p_x, p_z) and tests whether p_y lies above it.
int32_t TestPointAboveFaces(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t i;
	Vertex* vertices;
	int32_t done;
	Face* face;
	int32_t height;
	Vertex* vertex;

	i = 0;
	done = FALSE;
	vertices = (Vertex*) (p_shape->m_model + 1);
	while (!done) {
		face = (Face*) ((uint8_t*) p_shape->m_model + p_shape->m_model->m_faceOffset) + i;
		if (face->m_normal[1] < 0 && IsPointInFaceXZ(face, vertices, p_x, p_z)) {
			done = TRUE;
			vertex = &vertices[*((uint8_t*) face + face->m_indexOffset)];
			height = vertex->m_worldY - SolvePlaneY(
											face->m_normal[0],
											face->m_normal[1],
											face->m_normal[2],
											0,
											p_x - vertex->m_worldX,
											p_z - vertex->m_worldZ
										);
			g_hitNormalX = face->m_normal[0] >> 13;
			g_hitNormalY = face->m_normal[1] >> 13;
			g_hitNormalZ = face->m_normal[2] >> 13;
			if (p_y > height) {
				return 1;
			}
			else {
				return 0;
			}
		}
		else {
			i++;
			if (i == p_shape->m_model->m_faceCount) {
				done = TRUE;
			}
		}
	}

	return 0;
}

// Tests the ray against each face of the shape's model.
int32_t TestRayFaces(Shape* p_shape, Ray* p_ray)
{
	Model* model;
	Vertex* vertices;
	int32_t i;
	Face* face;

	model = p_shape->m_model;
	if (!model) {
		return 0;
	}

	vertices = (Vertex*) (model + 1);
	for (i = 0; i < model->m_faceCount; i++) {
		face = (Face*) ((uint8_t*) model + model->m_faceOffset) + i;
		if (IntersectRayFace(face, vertices, p_ray)) {
			return 1;
		}
	}

	return 0;
}
