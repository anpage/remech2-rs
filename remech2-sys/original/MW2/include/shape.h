#ifndef SHAPE_H
#define SHAPE_H

#include "shapegeom.h"
#include "types.h"

struct SceneObject;
struct Face;
struct Vertex;

/* A shape hung on a scene object (SceneObject::m_shape): a list of models by level of
   detail and the one in use. CreateShape allocates it. */
typedef struct Shape Shape;

struct Shape {
	uint16_t m_flags; // 0x10f: the load flags (SetShapeLoadFlags), 0xf0: the damage level
					  // (RaisePartDamageLevel), 0x200: transformed bounds current, 0x800: not in the collision list,
					  // 0x1000: hidden (in g_hiddenShapes)
	uint16_t m_kind;  // 0xf00: the owner class (0x100 player, 0x200 thing), 0xf0: the type
	struct Shape* m_prev;         // the previous shape in its list
	struct Shape* m_next;         // the next shape in its list
	struct Shape* m_prevCollider; // the collision list
	struct Shape* m_nextCollider;
	uint16_t m_owner;  // a player index (kind 0x100) or thing index (0x200)
	uint16_t m_partId; // the part a scene-tree search looks for (a mech section)
	struct SceneObject* m_object;
	Model* m_models;
	Model* m_model;
	int32_t m_collisionType; // indexes g_shapeCollisionFns; picks m_collisionData
	int32_t m_modelCenterX;  // the center ComputeShapeBounds computes
	int32_t m_modelCenterY;
	int32_t m_modelCenterZ;
	int32_t m_centerX; // the center transformed (TransformShapeCenter)
	int32_t m_centerY;
	int32_t m_centerZ;
	int32_t m_radius;          // the bounding sphere around the center
	void* m_collisionData;     // a BoundBox (type 0) or a QuadtreeNode (type 5)
	uint32_t m_transformCount; // bumped per transform; Model::m_transformCount stamps it
};

// The functions and globals of shape.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern uint32_t g_newShapeFlags;

	uint32_t GetNewShapeFlags(void);
	uint32_t SetNewShapeFlags(uint32_t p_flags);
	void SelectFirstModel(Shape* p_shape);
	void SelectModel(Shape* p_shape, Model* p_model);
	void SelectNextModel(Shape* p_shape);
	Model* AddModel(
		Shape* p_shape,
		int32_t p_key,
		int32_t p_vertexCount,
		int32_t p_faceCount,
		int32_t p_extra,
		void** p_extraData
	);
	void SelectModelByKey(Shape* p_shape, int32_t p_key);
	void SetModelKey(Shape* p_shape, int32_t p_key);
	int32_t GetModelKey(Shape* p_shape);
	Shape* CreateShape(int32_t p_vertexCount, int32_t p_faceCount, int32_t p_extra, void** p_extraData);
	void AddShapeVertex(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z, uint32_t p_u, uint32_t p_v);
	struct Face* AddShapeFace(Shape* p_shape, uint16_t p_color, uint8_t* p_indices);
	void AddShapeFaceIndex(Shape* p_shape, struct Face* p_face, uint32_t p_index);
	void FreeModel(Model* p_model);
	void RemoveSelectedModel(Shape* p_shape);
	void FreeShape(Shape* p_shape);
	void SetShapeState(Shape* p_shape, int32_t p_flags);
	uint32_t GetShapeState(Shape* p_shape);
	void SetShapeKind(Shape* p_shape, int32_t p_kind);
	void SetShapePartId(Shape* p_shape, uint16_t p_partId);
	void SetShapeOwner(Shape* p_shape, uint16_t p_owner);
	uint32_t GetShapeKind(Shape* p_shape);
	uint32_t GetShapePartId(Shape* p_shape);
	uint32_t GetShapeOwner(Shape* p_shape);
	int32_t GetShapeBounds(Shape* p_shape, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	void ComputeNormalsAndBounds(Shape* p_shape);
	void ComputeShapeBounds(Shape* p_shape);
	void ComputeFaceNormal(struct Face* p_face, struct Vertex* p_vertices);
	void GetModelCounts(Shape* p_shape, int32_t* p_vertexCount, int32_t* p_faceCount);
	void ForEachShape(Shape* p_shape, void (*p_fn)(Shape*));
	void GetTransformedVertex(Shape* p_shape, int32_t p_index, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	void GetVertexPosition(Shape* p_shape, int32_t p_index, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	void GetShapeFace(
		Shape* p_shape,
		int32_t p_index,
		uint32_t* p_color,
		uint32_t* p_count,
		uint32_t* p_indices,
		int32_t p_max
	);
	void SetFaceColor(Shape* p_shape, int32_t p_index, int32_t p_color);
	struct SceneObject* GetShapeObject(Shape* p_shape);
	void SetShapeObject(Shape* p_shape, struct SceneObject* p_object);
	uint32_t GetShapeLoadFlags(Shape* p_shape);
	void SetShapeLoadFlags(Shape* p_shape, uint32_t p_flags);
	void DestroyShape(Shape* p_shape);
	void DestroyObjShape(Shape* p_shape);
	int32_t GetShapeMemorySize(Shape* p_shape);

#ifdef __cplusplus
}
#endif

#endif // SHAPE_H
