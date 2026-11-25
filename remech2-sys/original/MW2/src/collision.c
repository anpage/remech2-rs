#include "collision.h"

#include "face.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "fixedmul29.h"
#include "fixedsqrt.h"
#include "muldiv.h"
#include "ray.h"
#include "shape.h"
#include "shapecollision.h"
#include "shapegeom.h"
#include "shapelists.h"
#include "types.h"
#include "vertex.h"

#include <stdlib.h>

// The collision tests of each shape type (Shape::m_collisionType).
ShapeCollisionFns g_shapeCollisionFns[8] = {
	{TestPointInBox, TestRayBox, GetBoxTop},
	{TestPointInBoxColumn, TestRayBox, NULL},
	{TestPointAboveFaces, TestRayFaces, NULL},
	{NULL, NULL, NULL},
	{TestPointNever, TestRayNever, NULL},
	{TestPointTerrain, TestRayTerrain, GetTerrainShapeTop},
	{NULL, NULL, NULL},
	{NULL, TestRayFaces, NULL}
};

// The normal of the surface the last collision test hit, 16.16.
int32_t g_hitNormalX = 0;

int32_t g_hitNormalY = 0;

int32_t g_hitNormalZ = 0;

// The normal of the ground GetTerrainHeight found.
int32_t g_groundNormalX = 0;

int32_t g_groundNormalY = 0;

int32_t g_groundNormalZ = 0;

// The normal of the surface TestSegmentCollision hit.
int32_t g_segmentNormalX = 0;

int32_t g_segmentNormalY = 0;

int32_t g_segmentNormalZ = 0;

// The background color the 3D view is cleared to.
int32_t g_backgroundColor = 0;

// The sky's color (DrawSkyAndGround; the ground's is g_groundColor).
int32_t g_skyColor = 0xe0;

// The ground's color (DrawSkyAndGround; the sky's is g_skyColor).
int32_t g_groundColor = 0xef;

// The horizon map's (LoadMapBitmap, from the world stream's hrzm record).
int32_t g_horizonMapColor = 0xea;

void SetShapeCollisionType(Shape* p_shape, int32_t p_collisionType)
{
	p_shape->m_collisionType = p_collisionType;

	if (p_collisionType == 4) {
		DisableShapeCollision(p_shape);
	}
	else {
		EnableShapeCollision(p_shape);
	}
}

// Returns whether p_y lies below the plane of the face at (p_x, p_z), with the plane's height
// there in p_height, and sets the hit normal to the face's.
int32_t IsBelowFacePlane(Face* p_face, Vertex* p_vertices, int32_t p_x, int32_t p_y, int32_t p_z, int32_t* p_height)
{
	Vertex* vertex;

	vertex = &p_vertices[*((uint8_t*) p_face + p_face->m_indexOffset)];
	*p_height = vertex->m_worldY - SolvePlaneY(
									   p_face->m_normal[0],
									   p_face->m_normal[1],
									   p_face->m_normal[2],
									   0,
									   p_x - vertex->m_worldX,
									   p_z - vertex->m_worldZ
								   );
	g_hitNormalX = p_face->m_normal[0] >> 13;
	g_hitNormalY = p_face->m_normal[1] >> 13;
	g_hitNormalZ = p_face->m_normal[2] >> 13;
	return p_y < *p_height;
}

// Returns the height of the ground at (p_x, p_z): the highest surface of the world's shapes
// within 1000 units of p_y, else the highest one below p_y, else 0. Its normal goes to
// g_groundNormal.
int32_t GetTerrainHeight(int32_t p_x, int32_t p_y, int32_t p_z)
{
	Shape* root;
	int32_t best;
	int32_t below;
	int32_t top;
	Shape* shape;
	int32_t (*getHeight)(Shape*, int32_t, int32_t, int32_t, int32_t*);
	int32_t type;

	root = g_sceneShapes;
	if (!root) {
		return 0;
	}

	below = best = -0x7fffffff;
	for (shape = root->m_nextCollider; shape; shape = shape->m_nextCollider) {
		type = shape->m_collisionType;
		if (type == 5) {
			getHeight = GetTerrainShapeTop;
		}
		else if (type == 0) {
			getHeight = GetBoxTop;
		}
		else {
			continue;
		}

		if (getHeight(shape, p_x, p_y, p_z, &top)) {
			if (best == -0x7fffffff && top < p_y && below < top) {
				below = top;
				g_groundNormalX = g_hitNormalX;
				g_groundNormalY = g_hitNormalY;
				g_groundNormalZ = g_hitNormalZ;
			}

			if (top + 1000 > p_y && top - 1000 < p_y && best < top) {
				best = top;
				g_groundNormalX = g_hitNormalX;
				g_groundNormalY = g_hitNormalY;
				g_groundNormalZ = g_hitNormalZ;
			}
		}
	}

	if (best > -0x7fffffff) {
		return best;
	}
	else if (below > -0x7fffffff) {
		return below;
	}
	else {
		return 0;
	}
}

// Returns the height of the highest surface of the world's shapes at (p_x, p_z), or 0. Its
// normal goes to g_groundNormal.
int32_t GetHighestSurface(int32_t p_x, int32_t p_y, int32_t p_z)
{
	Shape* root;
	int32_t best;
	int32_t top;
	Shape* shape;
	int32_t (*getHeight)(Shape*, int32_t, int32_t, int32_t, int32_t*);
	int32_t type;

	root = g_sceneShapes;
	if (!root) {
		return 0;
	}

	best = -0x7fffffff;
	for (shape = root->m_nextCollider; shape; shape = shape->m_nextCollider) {
		type = shape->m_collisionType;
		if (type == 5) {
			getHeight = GetTerrainShapeTop;
		}
		else if (type == 0) {
			getHeight = GetBoxTop;
		}
		else {
			continue;
		}

		if (getHeight(shape, p_x, p_y, p_z, &top) && best < top) {
			best = top;
			g_groundNormalX = g_hitNormalX;
			g_groundNormalY = g_hitNormalY;
			g_groundNormalZ = g_hitNormalZ;
		}
	}

	return best > -0x7fffffff ? best : 0;
}

int32_t HasHeightTest(Shape* p_shape)
{
	return g_shapeCollisionFns[p_shape->m_collisionType].m_getHeight != NULL;
}

// Tests the point against the world's shape nearest to it, returned in p_hit.
int32_t TestPointCollision(int32_t p_x, int32_t p_y, int32_t p_z, Shape** p_hit)
{
	*p_hit = FindNearestShape(g_sceneShapes, p_x, p_y, p_z);
	if (*p_hit && TestShapePoint(*p_hit, p_x, p_y, p_z)) {
		return 1;
	}
	else {
		return 0;
	}
}

// Returns the shape under p_root nearest to (p_x, p_y, p_z).
Shape* FindNearestShape(Shape* p_root, int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t best;
	int32_t distance;
	Shape* nearest;
	Shape* shape;

	nearest = NULL;
	best = 0x7fffffff;
	if (!p_root) {
		return NULL;
	}

	for (shape = p_root->m_nextCollider; shape; shape = shape->m_nextCollider) {
		distance = ApproximateShapeDistance(shape, p_x, p_y, p_z);
		if (distance < best) {
			best = distance;
			nearest = shape;
		}
	}

	return nearest;
}

// Tests the point against the shape; a shape type without a point test always hits.
int32_t TestShapePoint(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t (*testPoint)(Shape*, int32_t, int32_t, int32_t);

	testPoint = g_shapeCollisionFns[p_shape->m_collisionType].m_testPoint;
	if (testPoint) {
		return testPoint(p_shape, p_x, p_y, p_z);
	}

	return 1;
}

// Tests the segment against the world's shapes, other than those of player p_exclude (-1 for
// none). On a hit, returns the shape in p_hit and moves the ray's end to the nearest point of
// impact; its normal goes to g_segmentNormal.
int32_t TestSegmentCollision(Ray* p_ray, Shape** p_hit, int32_t p_exclude)
{
	uint16_t surface;
	int32_t best;
	Shape* root;
	Ray hitRay;
	int32_t distance;
	Shape* nearest;
	Ray ray;
	Shape* shape;
	int32_t length;

	nearest = NULL;
	best = 0x7fffffff;
	if (GetRayLength(p_ray) <= 0) {
		return TestPointCollision(p_ray->m_x0, p_ray->m_y0, p_ray->m_z0, p_hit);
	}

	root = g_sceneShapes;
	if (!root) {
		return 0;
	}

	for (shape = root->m_nextCollider; shape; shape = shape->m_nextCollider) {
		surface = shape->m_kind;
		if ((surface & 0x100) && (shape->m_owner == p_exclude || p_exclude < 0)) {
			continue;
		}

		distance = RayShapeDistance(shape, p_ray);
		if (distance < best) {
			CopyRay(&ray, p_ray);
			if (TestShapeRay(shape, &ray, distance)) {
				length = GetRayLength(&ray);
				if (length < best) {
					best = length;
					CopyRay(&hitRay, &ray);
					nearest = shape;
					g_segmentNormalX = g_hitNormalX;
					g_segmentNormalY = g_hitNormalY;
					g_segmentNormalZ = g_hitNormalZ;
				}
			}
		}
	}

	*p_hit = nearest;
	if (nearest) {
		CopyRay(p_ray, &hitRay);
		return 1;
	}

	return 0;
}

// TestSegmentCollision against the world's scenery only: no mechs, nor shapes of type 6.
int32_t TestSceneryCollision(Ray* p_ray, Shape** p_hit)
{
	uint16_t surface;
	int32_t best;
	Shape* root;
	Ray hitRay;
	int32_t distance;
	Shape* nearest;
	Ray ray;
	Shape* shape;

	nearest = NULL;
	best = 0x7fffffff;
	if (GetRayLength(p_ray) <= 0) {
		return TestPointCollision(p_ray->m_x0, p_ray->m_y0, p_ray->m_z0, p_hit);
	}

	root = g_sceneShapes;
	if (!root) {
		return 0;
	}

	for (shape = root->m_nextCollider; shape; shape = shape->m_nextCollider) {
		surface = shape->m_kind;
		if ((surface & 0x100) || shape->m_collisionType == 6) {
			continue;
		}

		distance = RayShapeDistance(shape, p_ray);
		if (distance < best) {
			CopyRay(&ray, p_ray);
			if (TestShapeRay(shape, &ray, distance)) {
				best = GetRayLength(&ray);
				CopyRay(&hitRay, &ray);
				nearest = shape;
				g_segmentNormalX = g_hitNormalX;
				g_segmentNormalY = g_hitNormalY;
				g_segmentNormalZ = g_hitNormalZ;
			}
		}
	}

	*p_hit = nearest;
	if (nearest) {
		CopyRay(p_ray, &hitRay);
		return 1;
	}

	return 0;
}

// Tests the ray against the shape: with the type's ray test, else by its point test at four
// steps along the ray (cutting the ray at the first hit), else by the shape's bounding sphere.
int32_t TestShapeRay(Shape* p_shape, Ray* p_ray, int32_t p_distance)
{
	int32_t (*testRay)(Shape*, Ray*);
	int32_t (*testPoint)(Shape*, int32_t, int32_t, int32_t);
	int32_t type;
	int32_t y;
	int32_t z;
	int32_t dx;
	int32_t dy;
	int32_t dz;
	int32_t i;
	int32_t x;

	if (GetRayLength(p_ray) <= 0) {
		return 1;
	}

	type = p_shape->m_collisionType;
	testRay = g_shapeCollisionFns[type].m_testRay;
	if (testRay) {
		return testRay(p_shape, p_ray);
	}

	testPoint = g_shapeCollisionFns[type].m_testPoint;
	if (testPoint) {
		x = p_ray->m_x0;
		y = p_ray->m_y0;
		z = p_ray->m_z0;
		dx = p_ray->m_x1 - x >> 2;
		dy = p_ray->m_y1 - y >> 2;
		dz = p_ray->m_z1 - z >> 2;
		for (i = 0; i < 4; i++) {
			x += dx;
			y += dy;
			z += dz;
			if (testPoint(p_shape, x, y, z)) {
				SetRayEnd(p_ray, x, y, z);
				g_hitNormalX = g_hitNormalY = g_hitNormalZ = 0;
				return 1;
			}
		}

		return 0;
	}

	EndRayAtShape(p_shape, p_ray, p_distance);
	return 1;
}

// Cuts the ray at p_distance along it and sets the hit normal to point from the shape's
// centre to the ray's end (to its start when p_distance is 10).
void EndRayAtShape(Shape* p_shape, Ray* p_ray, int32_t p_distance)
{
	if (p_distance > 0) {
		SetRayLength(p_ray, p_distance);
	}

	if (p_distance == 10) {
		g_hitNormalX = p_ray->m_x0 - p_shape->m_centerX;
		g_hitNormalY = p_ray->m_y0 - p_shape->m_centerY;
		g_hitNormalZ = p_ray->m_z0 - p_shape->m_centerZ;
	}
	else {
		g_hitNormalX = p_ray->m_x1 - p_shape->m_centerX;
		g_hitNormalY = p_ray->m_y1 - p_shape->m_centerY;
		g_hitNormalZ = p_ray->m_z1 - p_shape->m_centerZ;
	}

	NormalizeVectorGuarded(&g_hitNormalX, &g_hitNormalY, &g_hitNormalZ);
}

// Intersects p_ray with the face's plane from its front side and, if the point lies within the
// face, ends the ray there and keeps the face's normal as the hit normal.
int32_t IntersectRayFace(Face* p_face, Vertex* p_vertices, Ray* p_ray)
{
	int32_t nx;
	int32_t denom;
	int32_t t;
	int32_t d;
	Vertex* vertex;
	int32_t ny;
	int32_t nz;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t num;

	nx = p_face->m_normal[0];
	ny = p_face->m_normal[1];
	nz = p_face->m_normal[2];
	denom = FixedMul29(nx, p_ray->m_dirX) + FixedMul29(ny, p_ray->m_dirY) + FixedMul29(nz, p_ray->m_dirZ);
	if (denom >= 0) {
		return FALSE;
	}

	vertex = &p_vertices[((uint8_t*) p_face)[p_face->m_indexOffset]];
	d = -(FixedMul29(nx, vertex->m_worldX) + FixedMul29(ny, vertex->m_worldY) + FixedMul29(nz, vertex->m_worldZ));
	num = FixedMul29(nx, p_ray->m_x0) + FixedMul29(ny, p_ray->m_y0) + FixedMul29(nz, p_ray->m_z0) + d;
	if (num <= 0) {
		return FALSE;
	}

	t = -(num / denom);
	if (t > 0x7fff) {
		return FALSE;
	}

	t = FixedDiv16(-num, denom);
	if (t <= 0 || p_ray->m_length < t) {
		return FALSE;
	}

	x = p_ray->m_x0 + FixedMul16(p_ray->m_dirX, t);
	y = p_ray->m_y0 + FixedMul16(p_ray->m_dirY, t);
	z = p_ray->m_z0 + FixedMul16(p_ray->m_dirZ, t);
	if (IsPointInFace(p_face, p_vertices, x, y, z)) {
		SetRayEnd(p_ray, x, y, z);
		g_hitNormalX = nx >> 13;
		g_hitNormalY = ny >> 13;
		g_hitNormalZ = nz >> 13;
		return TRUE;
	}

	return FALSE;
}

// Tests whether the point (p_x, p_y, p_z) lies within the face, projected on the plane its normal
// is closest to.
int32_t IsPointInFace(Face* p_face, Vertex* p_vertices, int32_t p_x, int32_t p_y, int32_t p_z)
{
	int32_t nx;
	int32_t ny;
	int32_t nz;

	if (p_face->m_indexCount < 3) {
		return FALSE;
	}

	nx = abs(p_face->m_normal[0]);
	ny = abs(p_face->m_normal[1]);
	nz = abs(p_face->m_normal[2]);
	if (ny > nx && ny > nz) {
		return IsPointInFaceXZ(p_face, p_vertices, p_x, p_z);
	}
	else if (nz < nx) {
		return IsPointInFaceYZ(p_face, p_vertices, p_y, p_z);
	}
	else {
		return IsPointInFaceXY(p_face, p_vertices, p_x, p_y);
	}
}

// Tests whether (p_x, p_z) lies within the face, seen from above: the point must have vertices on
// each side, and the edges crossing its z must pass it on both sides.
int32_t IsPointInFaceXZ(Face* p_face, Vertex* p_vertices, int32_t p_x, int32_t p_z)
{
	int32_t count;
	Vertex* prev;
	int32_t sides;
	uint8_t* indices;
	int32_t i;
	int32_t right;
	int32_t offset;
	Vertex* vertex;
	int32_t left;

	left = FALSE;
	right = FALSE;
	count = p_face->m_indexCount;
	if (count < 3) {
		return FALSE;
	}

	indices = (uint8_t*) p_face + p_face->m_indexOffset;
	sides = 0;
	i = count;
	while (i--) {
		vertex = &p_vertices[indices[i]];
		if (vertex->m_worldX <= p_x) {
			sides |= 1;
		}
		else {
			sides |= 2;
		}

		if (vertex->m_worldZ <= p_z) {
			sides |= 4;
		}
		else {
			sides |= 8;
		}

		if (sides == 15) {
			break;
		}
	}

	if (sides != 15) {
		return FALSE;
	}

	prev = &p_vertices[indices[0]];
	for (i = count; i--; prev = vertex) {
		vertex = &p_vertices[indices[i]];
		if (vertex->m_worldZ < p_z && prev->m_worldZ < p_z) {
			continue;
		}

		if (vertex->m_worldZ > p_z && prev->m_worldZ > p_z) {
			continue;
		}

		if (vertex->m_worldZ == p_z && prev->m_worldZ == p_z) {
			if (vertex->m_worldX < p_x && prev->m_worldX < p_x) {
				return FALSE;
			}

			if (vertex->m_worldX > p_x && prev->m_worldX > p_x) {
				return FALSE;
			}

			return TRUE;
		}

		if (vertex->m_worldZ == p_z) {
			continue;
		}

		if (vertex->m_worldX < p_x && prev->m_worldX < p_x) {
			if (left) {
				return FALSE;
			}

			if (right) {
				return TRUE;
			}

			left = TRUE;
			continue;
		}

		if (vertex->m_worldX > p_x && prev->m_worldX > p_x) {
			if (right) {
				return FALSE;
			}

			if (left) {
				return TRUE;
			}

			right = TRUE;
			continue;
		}

		offset =
			vertex->m_worldX +
			MulDiv64(prev->m_worldX - vertex->m_worldX, p_z - vertex->m_worldZ, prev->m_worldZ - vertex->m_worldZ) -
			p_x;
		if (offset < 0) {
			if (left) {
				return FALSE;
			}

			if (right) {
				return TRUE;
			}

			left = TRUE;
		}
		else if (offset > 0) {
			if (right) {
				return FALSE;
			}

			if (left) {
				return TRUE;
			}

			right = TRUE;
		}
		else {
			return TRUE;
		}
	}

	return FALSE;
}

// Tests whether (p_x, p_y) lies within the face, seen along z.
int32_t IsPointInFaceXY(Face* p_face, Vertex* p_vertices, int32_t p_x, int32_t p_y)
{
	int32_t count;
	Vertex* prev;
	int32_t sides;
	uint8_t* indices;
	int32_t i;
	int32_t right;
	int32_t offset;
	Vertex* vertex;
	int32_t left;

	left = FALSE;
	right = FALSE;
	count = p_face->m_indexCount;
	if (count < 3) {
		return FALSE;
	}

	indices = (uint8_t*) p_face + p_face->m_indexOffset;
	sides = 0;
	i = count;
	while (i--) {
		vertex = &p_vertices[indices[i]];
		if (vertex->m_worldX <= p_x) {
			sides |= 1;
		}
		else {
			sides |= 2;
		}

		if (vertex->m_worldY <= p_y) {
			sides |= 4;
		}
		else {
			sides |= 8;
		}

		if (sides == 15) {
			break;
		}
	}

	if (sides != 15) {
		return FALSE;
	}

	prev = &p_vertices[indices[0]];
	for (i = count; i--; prev = vertex) {
		vertex = &p_vertices[indices[i]];
		if (vertex->m_worldY < p_y && prev->m_worldY < p_y) {
			continue;
		}

		if (vertex->m_worldY > p_y && prev->m_worldY > p_y) {
			continue;
		}

		if (vertex->m_worldY == p_y && prev->m_worldY == p_y) {
			if (vertex->m_worldX < p_x && prev->m_worldX < p_x) {
				return FALSE;
			}

			if (vertex->m_worldX > p_x && prev->m_worldX > p_x) {
				return FALSE;
			}

			return TRUE;
		}

		if (vertex->m_worldY == p_y) {
			continue;
		}

		if (vertex->m_worldX < p_x && prev->m_worldX < p_x) {
			if (left) {
				return FALSE;
			}

			if (right) {
				return TRUE;
			}

			left = TRUE;
			continue;
		}

		if (vertex->m_worldX > p_x && prev->m_worldX > p_x) {
			if (right) {
				return FALSE;
			}

			if (left) {
				return TRUE;
			}

			right = TRUE;
			continue;
		}

		offset =
			vertex->m_worldX +
			MulDiv64(prev->m_worldX - vertex->m_worldX, p_y - vertex->m_worldY, prev->m_worldY - vertex->m_worldY) -
			p_x;
		if (offset < 0) {
			if (left) {
				return FALSE;
			}

			if (right) {
				return TRUE;
			}

			left = TRUE;
		}
		else if (offset > 0) {
			if (right) {
				return FALSE;
			}

			if (left) {
				return TRUE;
			}

			right = TRUE;
		}
		else {
			return TRUE;
		}
	}

	return FALSE;
}

// Tests whether (p_y, p_z) lies within the face, seen along x.
int32_t IsPointInFaceYZ(Face* p_face, Vertex* p_vertices, int32_t p_y, int32_t p_z)
{
	int32_t count;
	Vertex* prev;
	int32_t sides;
	uint8_t* indices;
	int32_t i;
	int32_t right;
	int32_t offset;
	Vertex* vertex;
	int32_t left;

	left = FALSE;
	right = FALSE;
	count = p_face->m_indexCount;
	if (count < 3) {
		return FALSE;
	}

	indices = (uint8_t*) p_face + p_face->m_indexOffset;
	sides = 0;
	i = count;
	while (i--) {
		vertex = &p_vertices[indices[i]];
		if (vertex->m_worldY <= p_y) {
			sides |= 1;
		}
		else {
			sides |= 2;
		}

		if (vertex->m_worldZ <= p_z) {
			sides |= 4;
		}
		else {
			sides |= 8;
		}

		if (sides == 15) {
			break;
		}
	}

	if (sides != 15) {
		return FALSE;
	}

	prev = &p_vertices[indices[0]];
	for (i = count; i--; prev = vertex) {
		vertex = &p_vertices[indices[i]];
		if (vertex->m_worldZ < p_z && prev->m_worldZ < p_z) {
			continue;
		}

		if (vertex->m_worldZ > p_z && prev->m_worldZ > p_z) {
			continue;
		}

		if (vertex->m_worldZ == p_z && prev->m_worldZ == p_z) {
			if (vertex->m_worldY < p_y && prev->m_worldY < p_y) {
				return FALSE;
			}

			if (vertex->m_worldY > p_y && prev->m_worldY > p_y) {
				return FALSE;
			}

			return TRUE;
		}

		if (vertex->m_worldZ == p_z) {
			continue;
		}

		if (vertex->m_worldY < p_y && prev->m_worldY < p_y) {
			if (left) {
				return FALSE;
			}

			if (right) {
				return TRUE;
			}

			left = TRUE;
			continue;
		}

		if (vertex->m_worldY > p_y && prev->m_worldY > p_y) {
			if (right) {
				return FALSE;
			}

			if (left) {
				return TRUE;
			}

			right = TRUE;
			continue;
		}

		offset =
			vertex->m_worldY +
			MulDiv64(prev->m_worldY - vertex->m_worldY, p_z - vertex->m_worldZ, prev->m_worldZ - vertex->m_worldZ) -
			p_y;
		if (offset < 0) {
			if (left) {
				return FALSE;
			}

			if (right) {
				return TRUE;
			}

			left = TRUE;
		}
		else if (offset > 0) {
			if (right) {
				return FALSE;
			}

			if (left) {
				return TRUE;
			}

			right = TRUE;
		}
		else {
			return TRUE;
		}
	}

	return FALSE;
}
