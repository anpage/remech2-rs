#include "object.h"

#include "classtable.h"
#include "clock.h"
#include "collision.h"
#include "debris.h"
#include "decomp.h"
#include "poolsizes.h"
#include "shape.h"
#include "shapegeom.h"
#include "shapelists.h"
#include "simmain.h"
#include "staticmem.h"
#include "transform.h"
#include "types.h"

#include <mbstring.h>
#include <string.h>
#include <windows.h>

DECOMP_SIZE_ASSERT(SceneObject, 0x7c)

// The countdown the next object starts with, stepped through 64 to 191 so that objects
// renormalize on different frames.
// GLOBAL: MW2 0x100a00c8
MechU32 g_nextRenormalizeCountdown = 100;

// The only diff is a stack-slot permutation of obj and last.
// FUNCTION: MW2 0x100012d0
SceneObject* CreateObj(SceneObject* p_parent, MechU32 p_flags)
{
	SceneObject* obj;
	SceneObject* last;

	if (p_flags & 2) {
		obj = StaticPoolAlloc(sizeof(SceneObject), g_staticPoolTags[2]);
		if (obj == NULL) {
			return NULL;
		}

		obj->m_flags = c_objectPooled;
	}
	else if (p_flags & 4) {
		obj = MechHeapAlloc(g_primaryHeap, sizeof(SceneObject));
		if (obj == NULL) {
			return NULL;
		}

		obj->m_flags = c_objectHeap;
	}
	else {
		return NULL;
	}

	obj->m_parent = p_parent;
	obj->m_firstChild = NULL;
	obj->m_nextSibling = NULL;

	if (p_parent) {
		if (p_flags & 0x10) {
			last = GetObjLastChild(p_parent);
			if (last) {
				last->m_nextSibling = obj;
			}
			else {
				p_parent->m_firstChild = obj;
			}
		}
		else {
			obj->m_nextSibling = p_parent->m_firstChild;
			p_parent->m_firstChild = obj;
		}
	}

	SetIdentityMatrix(&obj->m_local);

	if (p_parent) {
		obj->m_world = p_parent->m_world;
	}
	else {
		SetIdentityMatrix(&obj->m_world);
	}

	obj->m_shape = NULL;
	obj->m_flags |= c_objectDirty;
	obj->m_name = NULL;
	obj->m_renormalizeCountdown = g_nextRenormalizeCountdown;
	g_nextRenormalizeCountdown = ((g_nextRenormalizeCountdown + 7) & 0x7f) + 0x40;
	return obj;
}

// FUNCTION: MW2 0x1000145a
SceneObject* InitObj(SceneObject* p_parent, void* p_memory)
{
	SceneObject* obj = p_memory;

	obj->m_parent = p_parent;
	obj->m_firstChild = NULL;

	if (p_parent) {
		obj->m_nextSibling = p_parent->m_firstChild;
		p_parent->m_firstChild = obj;
	}
	else {
		obj->m_nextSibling = NULL;
	}

	SetIdentityMatrix(&obj->m_local);

	if (p_parent) {
		obj->m_world = p_parent->m_world;
	}
	else {
		SetIdentityMatrix(&obj->m_world);
	}

	obj->m_shape = NULL;
	obj->m_flags = c_objectDirty;
	obj->m_name = NULL;
	obj->m_renormalizeCountdown = g_nextRenormalizeCountdown;
	g_nextRenormalizeCountdown = ((g_nextRenormalizeCountdown + 7) & 0x7f) + 0x40;
	return obj;
}

// FUNCTION: MW2 0x10001532
void SetObjShape(SceneObject* p_obj, Shape* p_shape)
{
	p_obj->m_shape = p_shape;
	p_obj->m_flags |= c_objectDirty;
}

// FUNCTION: MW2 0x1000154d
Shape* GetObjShape(SceneObject* p_obj)
{
	return p_obj->m_shape;
}

// FUNCTION: MW2 0x10001563
MechChar* GetObjName(SceneObject* p_obj)
{
	return p_obj->m_name;
}

// FUNCTION: MW2 0x10001579
void SetObjName(SceneObject* p_obj, MechChar* p_name)
{
	p_obj->m_name = (MechChar*) _mbsdup((unsigned char*) p_name);
}

// FUNCTION: MW2 0x10001596
void GetObjWorldAngles(SceneObject* p_obj, undefined4* p_angleX, undefined4* p_angleY, undefined4* p_angleZ)
{
	GetMatrixAngles(&p_obj->m_world, p_angleX, p_angleY, p_angleZ);
}

// FUNCTION: MW2 0x100015bc
void GetObjAngles(SceneObject* p_obj, undefined4* p_angleX, undefined4* p_angleY, undefined4* p_angleZ)
{
	GetMatrixAngles(&p_obj->m_local, p_angleX, p_angleY, p_angleZ);
}

// FUNCTION: MW2 0x100015e2
void GetObjPosition(SceneObject* p_obj, MechS32* p_x, MechS32* p_y, MechS32* p_z)
{
	*p_x = p_obj->m_world.m_rows[3][0];
	*p_y = p_obj->m_world.m_rows[3][1];
	*p_z = p_obj->m_world.m_rows[3][2];
}

// FUNCTION: MW2 0x1000160e
void GetObjLocalPosition(SceneObject* p_obj, MechS32* p_x, MechS32* p_y, MechS32* p_z)
{
	*p_x = p_obj->m_local.m_rows[3][0];
	*p_y = p_obj->m_local.m_rows[3][1];
	*p_z = p_obj->m_local.m_rows[3][2];
}

// FUNCTION: MW2 0x1000163a
void SetObjPosition(SceneObject* p_obj, MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	p_obj->m_local.m_rows[3][0] = p_x;
	p_obj->m_local.m_rows[3][1] = p_y;
	p_obj->m_local.m_rows[3][2] = p_z;
	p_obj->m_flags |= c_objectDirty;
}

// FUNCTION: MW2 0x10001667
void MoveObj(SceneObject* p_obj, MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	p_obj->m_local.m_rows[3][0] += p_x;
	p_obj->m_local.m_rows[3][1] += p_y;
	p_obj->m_local.m_rows[3][2] += p_z;
	p_obj->m_flags |= c_objectDirty;
}

// FUNCTION: MW2 0x10001694
void SetObjTransform(SceneObject* p_obj, Matrix* p_matrix)
{
	p_obj->m_local = *p_matrix;
	p_obj->m_flags |= c_objectDirty;
}

// Multiplies the object's local matrix by p_matrix, renormalizing the rotation now and then.
// FUNCTION: MW2 0x100016b6
void TransformObj(SceneObject* p_obj, Matrix* p_matrix)
{
	MultiplyMatrix(&p_obj->m_local, p_matrix, &p_obj->m_local);
	p_obj->m_flags |= c_objectDirty;

	p_obj->m_renormalizeCountdown--;
	if (p_obj->m_renormalizeCountdown <= 0) {
		p_obj->m_renormalizeCountdown = g_nextRenormalizeCountdown;
		g_nextRenormalizeCountdown = ((g_nextRenormalizeCountdown + 7) & 0x7f) + 0x40;
		NormalizeRotation(&p_obj->m_local);
	}
}

// FUNCTION: MW2 0x10001722
void SetObjRotationMatrix(SceneObject* p_obj, Matrix* p_matrix)
{
	p_obj->m_local.m_rows[0][0] = p_matrix->m_rows[0][0];
	p_obj->m_local.m_rows[0][1] = p_matrix->m_rows[0][1];
	p_obj->m_local.m_rows[0][2] = p_matrix->m_rows[0][2];
	p_obj->m_local.m_rows[1][0] = p_matrix->m_rows[1][0];
	p_obj->m_local.m_rows[1][1] = p_matrix->m_rows[1][1];
	p_obj->m_local.m_rows[1][2] = p_matrix->m_rows[1][2];
	p_obj->m_local.m_rows[2][0] = p_matrix->m_rows[2][0];
	p_obj->m_local.m_rows[2][1] = p_matrix->m_rows[2][1];
	p_obj->m_local.m_rows[2][2] = p_matrix->m_rows[2][2];
	p_obj->m_flags |= c_objectDirty;
}

// FUNCTION: MW2 0x1000179f
void RotateObjMatrix(SceneObject* p_obj, Matrix* p_matrix)
{
	MultiplyRotations(&p_obj->m_local, p_matrix, &p_obj->m_local);
	p_obj->m_flags |= c_objectDirty;

	p_obj->m_renormalizeCountdown--;
	if (p_obj->m_renormalizeCountdown <= 0) {
		p_obj->m_renormalizeCountdown = g_nextRenormalizeCountdown;
		g_nextRenormalizeCountdown = ((g_nextRenormalizeCountdown + 7) & 0x7f) + 0x40;
		NormalizeRotation(&p_obj->m_local);
	}
}

// FUNCTION: MW2 0x1000180b
void SetObjRotation(SceneObject* p_obj, MechS32 p_angleX, MechS32 p_angleY, MechS32 p_angleZ, MechU32 p_flags)
{
	Matrix matrix;

	BuildMatrixEx(&matrix, p_angleX, p_angleY, p_angleZ, 0, 0, 0, p_flags);
	SetObjRotationMatrix(p_obj, &matrix);
}

// FUNCTION: MW2 0x1000184b
void RotateObj(SceneObject* p_obj, MechS32 p_angleX, MechS32 p_angleY, MechS32 p_angleZ, MechU32 p_flags)
{
	Matrix matrix;

	BuildMatrixEx(&matrix, p_angleX, p_angleY, p_angleZ, 0, 0, 0, p_flags);
	RotateObjMatrix(p_obj, &matrix);
}

// The only diff is a stack-slot permutation of model and obj.
// FUNCTION: MW2 0x1000188b
void TransformShapeModel(Shape* p_shape)
{
	Model* model = p_shape->m_model;
	SceneObject* obj = p_shape->m_object;

	TransformModel(model, &obj->m_world);
	model->m_transformCount = p_shape->m_transformCount;
}

// FUNCTION: MW2 0x100018ca
void HideObjTree(SceneObject* p_obj)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		HideShape(p_obj->m_shape);
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		HideObjTree(child);
	}
}

// FUNCTION: MW2 0x10001926
void ShowObjTree(SceneObject* p_obj)
{
	SceneObject* child;

	if (p_obj->m_shape && (p_obj->m_shape->m_kind & 0xf0) != 0x70) {
		ShowShape(p_obj->m_shape);
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		ShowObjTree(child);
	}
}

// FUNCTION: MW2 0x1000199a
void DisableObjTreeCollision(SceneObject* p_obj)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		DisableShapeCollision(p_obj->m_shape);
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		DisableObjTreeCollision(child);
	}
}

// FUNCTION: MW2 0x100019f6
void EnableObjTreeCollision(SceneObject* p_obj)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		EnableShapeCollision(p_obj->m_shape);
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		EnableObjTreeCollision(child);
	}
}

// FUNCTION: MW2 0x10001a52
void DetachObjTreeShapes(SceneObject* p_obj)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		DetachShape(p_obj->m_shape);
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		DetachObjTreeShapes(child);
	}
}

// FUNCTION: MW2 0x10001aae
void SetObjTreeKind(SceneObject* p_obj, MechS32 p_kind)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		p_obj->m_shape->m_kind = p_kind;
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		SetObjTreeKind(child, p_kind);
	}
}

// FUNCTION: MW2 0x10001b0c
void SetObjTreeOwner(SceneObject* p_obj, MechS32 p_owner)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		p_obj->m_shape->m_owner = p_owner;
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		SetObjTreeOwner(child, p_owner);
	}
}

// FUNCTION: MW2 0x10001b6a
void SetObjTreeCollisionType(SceneObject* p_obj, MechS32 p_collisionType)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		SetShapeCollisionType(p_obj->m_shape, p_collisionType);
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		SetObjTreeCollisionType(child, p_collisionType);
	}
}

// FUNCTION: MW2 0x10001bce
void ClearObjTreeKind(SceneObject* p_obj, MechS32 p_flags)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		p_obj->m_shape->m_kind &= (MechS16) ~p_flags;
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		ClearObjTreeKind(child, p_flags);
	}
}

// Recomputes the world matrices of the object and its subtree, and their shapes' centers.
// FUNCTION: MW2 0x10001c3f
void UpdateObjWorld(SceneObject* p_obj)
{
	Shape* shape;
	SceneObject* child;

	p_obj->m_flags &= ~c_objectDirty;

	if (p_obj->m_parent) {
		MultiplyMatrix(&p_obj->m_parent->m_world, &p_obj->m_local, &p_obj->m_world);
	}
	else {
		p_obj->m_world = p_obj->m_local;
	}

	shape = p_obj->m_shape;
	if (shape) {
		TransformShapeCenter(shape, &p_obj->m_world);
		shape->m_object = p_obj;
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		UpdateObjWorld(child);
	}
}

// FUNCTION: MW2 0x10001cf8
void UpdateObj(SceneObject* p_obj)
{
	SceneObject* child;

	if (p_obj->m_flags & c_objectDirty) {
		UpdateObjWorld(p_obj);
	}
	else if (p_obj->m_firstChild) {
		for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
			UpdateObj(child);
		}
	}
}

// FUNCTION: MW2 0x10001d63
SceneObject* GetObjRoot(SceneObject* p_obj)
{
	while (p_obj->m_parent) {
		p_obj = p_obj->m_parent;
	}

	return p_obj;
}

// FUNCTION: MW2 0x10001d8f
SceneObject* GetObjParent(SceneObject* p_obj)
{
	return p_obj->m_parent;
}

// FUNCTION: MW2 0x10001da4
SceneObject* GetObjFirstChild(SceneObject* p_obj)
{
	return p_obj->m_firstChild;
}

// FUNCTION: MW2 0x10001dba
SceneObject* GetObjNextSibling(SceneObject* p_obj)
{
	return p_obj->m_nextSibling;
}

// FUNCTION: MW2 0x10001dd0
Matrix* GetObjLocalMatrix(SceneObject* p_obj)
{
	return &p_obj->m_local;
}

// FUNCTION: MW2 0x10001de6
void SetObjLocalMatrix(SceneObject* p_obj, Matrix* p_matrix)
{
	p_obj->m_local = *p_matrix;
}

// FUNCTION: MW2 0x10001e01
Matrix* GetObjWorldMatrix(SceneObject* p_obj)
{
	return &p_obj->m_world;
}

// FUNCTION: MW2 0x10001e17
void SetObjWorldMatrix(SceneObject* p_obj, Matrix* p_matrix)
{
	p_obj->m_world = *p_matrix;
}

// The only diff is a stack-slot permutation of parent and first.
// Unlinks the object from its parent, keeping its world placement.
// FUNCTION: MW2 0x10001e32
void DetachObj(SceneObject* p_obj)
{
	SceneObject* parent;
	SceneObject* sibling;
	SceneObject* first;

	parent = p_obj->m_parent;
	if (parent) {
		if (parent->m_firstChild == p_obj) {
			first = p_obj->m_nextSibling;
			parent->m_firstChild = first;
		}
		else {
			first = parent->m_firstChild;
			sibling = first;
			while (sibling) {
				if (sibling->m_nextSibling == p_obj) {
					sibling->m_nextSibling = p_obj->m_nextSibling;
					break;
				}

				sibling = sibling->m_nextSibling;
			}
		}
	}

	p_obj->m_parent = NULL;
	p_obj->m_nextSibling = NULL;
	p_obj->m_local = p_obj->m_world;
	p_obj->m_flags |= c_objectDirty;
	UpdateObj(p_obj);
}

// Links the object under p_parent, keeping its world placement.
// FUNCTION: MW2 0x10001ef8
void AttachObj(SceneObject* p_obj, SceneObject* p_parent)
{
	Matrix inverse;

	if (p_obj->m_parent) {
		DetachObj(p_obj);
	}

	p_obj->m_parent = p_parent;
	p_obj->m_nextSibling = p_parent->m_firstChild;
	p_parent->m_firstChild = p_obj;

	InvertMatrix(&p_parent->m_world, &inverse);
	MultiplyMatrix(&inverse, &p_obj->m_world, &p_obj->m_local);
	p_obj->m_flags &= ~c_objectDirty;
	p_parent->m_flags |= c_objectDirty;
	UpdateObj(p_parent);
}

// FUNCTION: MW2 0x10001f82
void DestroyObjTree(SceneObject* p_obj, ShapeCallback p_callback)
{
	SceneObject* child;
	SceneObject* next;

	DetachObj(p_obj);

	child = p_obj->m_firstChild;
	while (child) {
		next = child->m_nextSibling;
		DestroyObjTree(child, p_callback);
		child = next;
	}

	if (p_obj->m_shape && p_callback) {
		p_callback(p_obj->m_shape);
	}

	if (p_obj->m_flags & c_objectHeap) {
		MechHeapFree(g_primaryHeap, p_obj);
	}
}

// FUNCTION: MW2 0x10002016
SceneObject* FindObjByName(SceneObject* p_obj, const MechChar* p_name)
{
	SceneObject* child;

	if (p_obj->m_name && _strcmpi(p_obj->m_name, p_name) == 0) {
		return p_obj;
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		p_obj = FindObjByName(child, p_name);
		if (p_obj) {
			return p_obj;
		}
	}

	return NULL;
}

// The only diff is a stack-slot permutation of child and last.
// FUNCTION: MW2 0x100020a6
SceneObject* GetObjLastChild(SceneObject* p_obj)
{
	SceneObject* child;
	SceneObject* last;

	last = NULL;
	child = p_obj->m_firstChild;
	if (child) {
		last = child;
		while ((child = child->m_nextSibling)) {
			last = child;
		}
	}

	return last;
}

// Returns child p_index in the order the children were added (they are prepended), or NULL.
// The only diff is a stack-slot permutation of child and count.
// FUNCTION: MW2 0x100020fa
SceneObject* GetObjChild(SceneObject* p_obj, MechS32 p_index)
{
	SceneObject* result;
	SceneObject* child;
	MechS32 count;

	result = NULL;
	count = 0;

	if (p_obj == NULL) {
		return NULL;
	}

	child = p_obj->m_firstChild;
	if (child == NULL) {
		return NULL;
	}

	while (child) {
		count++;
		result = child;
		child = child->m_nextSibling;
	}

	count = count - p_index - 1;
	if (count < 0) {
		return NULL;
	}

	result = p_obj->m_firstChild;
	while (count-- && result) {
		result = result->m_nextSibling;
	}

	return result;
}

// FUNCTION: MW2 0x100021b6
SceneObject* FindObjByPart(SceneObject* p_obj, MechU32 p_partId)
{
	SceneObject* child;
	MechU32 id = p_partId;

	if (p_obj->m_shape && p_obj->m_shape->m_partId == id) {
		return p_obj;
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		p_obj = FindObjByPart(child, p_partId);
		if (p_obj) {
			return p_obj;
		}
	}

	return NULL;
}

// Raises the damage level (shape flags 0xf0) of mech part p_partId's shapes to p_level (1 to 15).
// The remaining diffs are a stack-slot permutation of current and child, and the operand
// order of p_level > current (declaration order didn't flip it).
// FUNCTION: MW2 0x10002246
void RaisePartDamageLevel(SceneObject* p_obj, MechS32 p_level, MechU32 p_partId)
{
	MechS32 current;
	SceneObject* child;
	Shape* shape;

	shape = p_obj->m_shape;
	current = (GetShapeState(shape) & 0xf0) >> 4;

	if (p_level > 0 && p_level < 0x10 && p_level > current) {
		p_level <<= 4;
	}
	else {
		return;
	}

	if (p_obj->m_shape && p_obj->m_shape->m_partId == p_partId) {
		SetShapeState(p_obj->m_shape, p_level);
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		RaisePartDamageLevel(child, p_level >> 4, p_partId);
	}
}

// Blows mech part p_partId's objects off the tree as debris.
// FUNCTION: MW2 0x10002314
void BlowOffPart(SceneObject* p_obj, MechU32 p_partId)
{
	if (p_obj == NULL) {
		return;
	}

	BlowOffPart(p_obj->m_nextSibling, p_partId);

	if (p_obj->m_shape && p_obj->m_shape->m_partId == p_partId) {
		BlowOffObjTree(p_obj->m_firstChild, ReleaseObjShape, p_partId);
		BlowOffChunk(p_obj, ReleaseObjShape, p_partId);
	}
	else {
		BlowOffPart(p_obj->m_firstChild, p_partId);
	}
}

// FUNCTION: MW2 0x100023a8
MechS32 GetObjSize(void)
{
	return sizeof(SceneObject);
}
