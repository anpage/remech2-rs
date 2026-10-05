#ifndef QUADTREE_H
#define QUADTREE_H

#include "decomp.h"
#include "ray.h"
#include "types.h"

struct Face;
struct Shape;
#include "shapegeom.h"

// A quadtree node: its bounds, its four children (m_unk0x18 == 0) and m_unk0x18 entries
// after the header (undefined4 each). AllocQuadtreeNode allocates it; Shape::m_collisionData holds the root
// when the shape's m_collisionType is 5.
// SIZE 0x2c
typedef struct QuadtreeNode {
	MechS32 m_minX;                     // 0x00
	MechS32 m_maxX;                     // 0x04
	MechS32 m_minY;                     // 0x08
	MechS32 m_maxY;                     // 0x0c
	MechS32 m_minZ;                     // 0x10
	MechS32 m_maxZ;                     // 0x14
	MechS32 m_faceCount;                // 0x18
	struct QuadtreeNode* m_children[4]; // 0x1c
} QuadtreeNode;

// The functions and globals of quadtree.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_quadtreesDisabled;

	void BuildShapeQuadtree(struct Shape* p_shape);
	QuadtreeNode* BuildQuadtreeChild(QuadtreeNode* p_node, MechS32 p_quadrant, Model* p_model);
	QuadtreeNode* AllocQuadtreeNode(
		undefined4 p_minX,
		undefined4 p_maxX,
		undefined4 p_minY,
		undefined4 p_maxY,
		undefined4 p_minZ,
		undefined4 p_maxZ,
		MechS32 p_faceCount
	);
	void FreeQuadtree(QuadtreeNode* p_node);
	MechS32 FaceOverlapsBox(
		struct Face* p_face,
		Model* p_model,
		MechS32 p_minX,
		MechS32 p_maxX,
		MechS32 p_minZ,
		MechS32 p_maxZ,
		MechS32* p_minY,
		MechS32* p_maxY
	);
	MechS32 ClassifyQuadtreePoint(QuadtreeNode* p_node, Model* p_model, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 TestQuadtreeRay(QuadtreeNode* p_node, Model* p_model, Ray* p_ray);
	MechS32 TestQuadtreeChildrenRay(QuadtreeNode* p_node, Model* p_model, Ray* p_ray);
	MechS32 GetQuadtreeTop(QuadtreeNode* p_node, Model* p_model, MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32* p_top);
	void DisableQuadtrees(void);
	MechS32 GetQuadtreeSize(QuadtreeNode* p_node);

#ifdef __cplusplus
}
#endif

#endif // QUADTREE_H
