#ifndef QUADTREE_H
#define QUADTREE_H

#include "ray.h"
#include "types.h"

struct Face;
struct Shape;
#include "shapegeom.h"

// A quadtree node: its bounds, its four children (m_faceCount == 0) and m_faceCount entries
// after the header (uint32_t each). AllocQuadtreeNode allocates it; Shape::m_collisionData holds the root
// when the shape's m_collisionType is 5.
typedef struct QuadtreeNode {
	int32_t m_minX;                     // 0x00
	int32_t m_maxX;                     // 0x04
	int32_t m_minY;                     // 0x08
	int32_t m_maxY;                     // 0x0c
	int32_t m_minZ;                     // 0x10
	int32_t m_maxZ;                     // 0x14
	int32_t m_faceCount;                // 0x18
	struct QuadtreeNode* m_children[4]; // 0x1c
} QuadtreeNode;

// The functions and globals of quadtree.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_quadtreesDisabled;

	void BuildShapeQuadtree(struct Shape* p_shape);
	QuadtreeNode* BuildQuadtreeChild(QuadtreeNode* p_node, int32_t p_quadrant, Model* p_model);
	QuadtreeNode* AllocQuadtreeNode(
		int32_t p_minX,
		int32_t p_maxX,
		int32_t p_minY,
		int32_t p_maxY,
		int32_t p_minZ,
		int32_t p_maxZ,
		int32_t p_faceCount
	);
	void FreeQuadtree(QuadtreeNode* p_node);
	int32_t FaceOverlapsBox(
		struct Face* p_face,
		Model* p_model,
		int32_t p_minX,
		int32_t p_maxX,
		int32_t p_minZ,
		int32_t p_maxZ,
		int32_t* p_minY,
		int32_t* p_maxY
	);
	int32_t ClassifyQuadtreePoint(QuadtreeNode* p_node, Model* p_model, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t TestQuadtreeRay(QuadtreeNode* p_node, Model* p_model, Ray* p_ray);
	int32_t TestQuadtreeChildrenRay(QuadtreeNode* p_node, Model* p_model, Ray* p_ray);
	int32_t GetQuadtreeTop(QuadtreeNode* p_node, Model* p_model, int32_t p_x, int32_t p_y, int32_t p_z, int32_t* p_top);
	void DisableQuadtrees(void);
	int32_t GetQuadtreeSize(QuadtreeNode* p_node);

#ifdef __cplusplus
}
#endif

#endif // QUADTREE_H
