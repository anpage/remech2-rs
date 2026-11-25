#ifndef COLLISION_H
#define COLLISION_H

#include "ray.h"
#include "shape.h"
#include "types.h"

struct Face;
struct Vertex;

// A shape type's collision tests: a point, a ray, and the height of the surface under a point.
typedef struct ShapeCollisionFns {
	int32_t (*m_testPoint)(Shape*, int32_t, int32_t, int32_t);           // 0x00
	int32_t (*m_testRay)(Shape*, Ray*);                                  // 0x04
	int32_t (*m_getHeight)(Shape*, int32_t, int32_t, int32_t, int32_t*); // 0x08
} ShapeCollisionFns;

// The functions and globals of collision.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_groundColor;
	extern int32_t g_horizonMapColor;
	extern ShapeCollisionFns g_shapeCollisionFns[8];
	extern int32_t g_hitNormalX;
	extern int32_t g_hitNormalY;
	extern int32_t g_hitNormalZ;
	extern int32_t g_groundNormalX;
	extern int32_t g_groundNormalY;
	extern int32_t g_groundNormalZ;
	extern int32_t g_segmentNormalX;
	extern int32_t g_segmentNormalY;
	extern int32_t g_segmentNormalZ;
	extern int32_t g_backgroundColor;
	extern int32_t g_skyColor;

	void SetShapeCollisionType(Shape* p_shape, int32_t p_collisionType);
	int32_t IsBelowFacePlane(
		struct Face* p_face,
		struct Vertex* p_vertices,
		int32_t p_x,
		int32_t p_y,
		int32_t p_z,
		int32_t* p_height
	);
	int32_t GetTerrainHeight(int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t GetHighestSurface(int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t HasHeightTest(Shape* p_shape);
	int32_t TestPointCollision(int32_t p_x, int32_t p_y, int32_t p_z, Shape** p_hit);
	Shape* FindNearestShape(Shape* p_root, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t TestShapePoint(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t TestSegmentCollision(Ray* p_ray, Shape** p_hit, int32_t p_exclude);
	int32_t TestSceneryCollision(Ray* p_ray, Shape** p_hit);
	int32_t TestShapeRay(Shape* p_shape, Ray* p_ray, int32_t p_distance);
	void EndRayAtShape(Shape* p_shape, Ray* p_ray, int32_t p_distance);
	int32_t IntersectRayFace(struct Face* p_face, struct Vertex* p_vertices, Ray* p_ray);
	int32_t IsPointInFace(struct Face* p_face, struct Vertex* p_vertices, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t IsPointInFaceXZ(struct Face* p_face, struct Vertex* p_vertices, int32_t p_x, int32_t p_z);
	int32_t IsPointInFaceXY(struct Face* p_face, struct Vertex* p_vertices, int32_t p_x, int32_t p_y);
	int32_t IsPointInFaceYZ(struct Face* p_face, struct Vertex* p_vertices, int32_t p_y, int32_t p_z);

#ifdef __cplusplus
}
#endif

#endif // COLLISION_H
