#ifndef OBJECT_H
#define OBJECT_H

#include "decomp.h"
#include "shape.h"
#include "transform.h"
#include "types.h"

typedef struct SceneObject SceneObject;

typedef void (*ShapeCallback)(Shape* p_shape);
typedef void (*ObjectCallback)(SceneObject* p_obj);

/* A node of the scene tree: its transform relative to the parent, the world transform
   UpdateObjWorld derives from it, and an optional shape. CreateObj allocates it. */
// SIZE 0x7c
struct SceneObject {
	SceneObject* m_parent;          // 0x00
	SceneObject* m_firstChild;      // 0x04
	SceneObject* m_nextSibling;     // 0x08
	Matrix m_local;                 // 0x0c
	Matrix m_world;                 // 0x3c
	Shape* m_shape;                 // 0x6c
	MechChar* m_name;               // 0x70
	MechU32 m_flags;                // 0x74
	MechU32 m_renormalizeCountdown; // 0x78 — rotations left before NormalizeRotation (64 to 191, staggered)
};

enum {
	c_objectDirty = 0x01,  // m_world is out of date
	c_objectPooled = 0x02, // allocated from the static pool
	c_objectHeap = 0x04,   // allocated from the primary heap, freed by DestroyObjTree
};

// The functions and globals of object.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechU32 g_nextRenormalizeCountdown;

	SceneObject* CreateObj(SceneObject* p_parent, MechU32 p_flags);
	SceneObject* InitObj(SceneObject* p_parent, void* p_memory);
	void SetObjShape(SceneObject* p_obj, Shape* p_shape);
	Shape* GetObjShape(SceneObject* p_obj);
	MechChar* GetObjName(SceneObject* p_obj);
	void SetObjName(SceneObject* p_obj, MechChar* p_name);
	void GetObjWorldAngles(SceneObject* p_obj, undefined4* p_angleX, undefined4* p_angleY, undefined4* p_angleZ);
	void GetObjAngles(SceneObject* p_obj, undefined4* p_angleX, undefined4* p_angleY, undefined4* p_angleZ);
	void GetObjPosition(SceneObject* p_obj, MechS32* p_x, MechS32* p_y, MechS32* p_z);
	void GetObjLocalPosition(SceneObject* p_obj, MechS32* p_x, MechS32* p_y, MechS32* p_z);
	void SetObjPosition(SceneObject* p_obj, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	void MoveObj(SceneObject* p_obj, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	void SetObjTransform(SceneObject* p_obj, Matrix* p_matrix);
	void TransformObj(SceneObject* p_obj, Matrix* p_matrix);
	void SetObjRotationMatrix(SceneObject* p_obj, Matrix* p_matrix);
	void RotateObjMatrix(SceneObject* p_obj, Matrix* p_matrix);
	void SetObjRotation(SceneObject* p_obj, MechS32 p_angleX, MechS32 p_angleY, MechS32 p_angleZ, MechU32 p_flags);
	void RotateObj(SceneObject* p_obj, MechS32 p_angleX, MechS32 p_angleY, MechS32 p_angleZ, MechU32 p_flags);
	void TransformShapeModel(Shape* p_shape);
	void HideObjTree(SceneObject* p_obj);
	void ShowObjTree(SceneObject* p_obj);
	void DisableObjTreeCollision(SceneObject* p_obj);
	void EnableObjTreeCollision(SceneObject* p_obj);
	void DetachObjTreeShapes(SceneObject* p_obj);
	void SetObjTreeKind(SceneObject* p_obj, MechS32 p_kind);
	void SetObjTreeOwner(SceneObject* p_obj, MechS32 p_owner);
	void SetObjTreeCollisionType(SceneObject* p_obj, MechS32 p_collisionType);
	void ClearObjTreeKind(SceneObject* p_obj, MechS32 p_flags);
	void UpdateObjWorld(SceneObject* p_obj);
	void UpdateObj(SceneObject* p_obj);
	SceneObject* GetObjRoot(SceneObject* p_obj);
	SceneObject* GetObjParent(SceneObject* p_obj);
	SceneObject* GetObjFirstChild(SceneObject* p_obj);
	SceneObject* GetObjNextSibling(SceneObject* p_obj);
	Matrix* GetObjLocalMatrix(SceneObject* p_obj);
	void SetObjLocalMatrix(SceneObject* p_obj, Matrix* p_matrix);
	Matrix* GetObjWorldMatrix(SceneObject* p_obj);
	void SetObjWorldMatrix(SceneObject* p_obj, Matrix* p_matrix);
	void DetachObj(SceneObject* p_obj);
	void AttachObj(SceneObject* p_obj, SceneObject* p_parent);
	void DestroyObjTree(SceneObject* p_obj, ShapeCallback p_callback);
	SceneObject* FindObjByName(SceneObject* p_obj, const MechChar* p_name);
	SceneObject* GetObjLastChild(SceneObject* p_obj);
	SceneObject* GetObjChild(SceneObject* p_obj, MechS32 p_index);
	SceneObject* FindObjByPart(SceneObject* p_obj, MechU32 p_partId);
	void RaisePartDamageLevel(SceneObject* p_obj, MechS32 p_level, MechU32 p_partId);
	void BlowOffPart(SceneObject* p_obj, MechU32 p_partId);
	MechS32 GetObjSize(void);

#ifdef __cplusplus
}
#endif

#endif // OBJECT_H
