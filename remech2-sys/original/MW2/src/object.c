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

#include <string.h>
#include <strings.h>

// The countdown the next object starts with, stepped through 64 to 191 so that objects
// renormalize on different frames.
uint32_t g_nextRenormalizeCountdown = 100;

SceneObject* CreateObj(SceneObject* p_parent, uint32_t p_flags)
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

void SetObjShape(SceneObject* p_obj, Shape* p_shape)
{
	p_obj->m_shape = p_shape;
	p_obj->m_flags |= c_objectDirty;
}

Shape* GetObjShape(SceneObject* p_obj)
{
	return p_obj->m_shape;
}

char* GetObjName(SceneObject* p_obj)
{
	return p_obj->m_name;
}

void SetObjName(SceneObject* p_obj, char* p_name)
{
	p_obj->m_name = strdup(p_name);
}

void GetObjWorldAngles(SceneObject* p_obj, undefined4* p_angleX, undefined4* p_angleY, undefined4* p_angleZ)
{
	GetMatrixAngles(&p_obj->m_world, p_angleX, p_angleY, p_angleZ);
}

void GetObjAngles(SceneObject* p_obj, undefined4* p_angleX, undefined4* p_angleY, undefined4* p_angleZ)
{
	GetMatrixAngles(&p_obj->m_local, p_angleX, p_angleY, p_angleZ);
}

void GetObjPosition(SceneObject* p_obj, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	*p_x = p_obj->m_world.m_rows[3][0];
	*p_y = p_obj->m_world.m_rows[3][1];
	*p_z = p_obj->m_world.m_rows[3][2];
}

void GetObjLocalPosition(SceneObject* p_obj, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	*p_x = p_obj->m_local.m_rows[3][0];
	*p_y = p_obj->m_local.m_rows[3][1];
	*p_z = p_obj->m_local.m_rows[3][2];
}

void SetObjPosition(SceneObject* p_obj, int32_t p_x, int32_t p_y, int32_t p_z)
{
	p_obj->m_local.m_rows[3][0] = p_x;
	p_obj->m_local.m_rows[3][1] = p_y;
	p_obj->m_local.m_rows[3][2] = p_z;
	p_obj->m_flags |= c_objectDirty;
}

void MoveObj(SceneObject* p_obj, int32_t p_x, int32_t p_y, int32_t p_z)
{
	p_obj->m_local.m_rows[3][0] += p_x;
	p_obj->m_local.m_rows[3][1] += p_y;
	p_obj->m_local.m_rows[3][2] += p_z;
	p_obj->m_flags |= c_objectDirty;
}

void SetObjTransform(SceneObject* p_obj, Matrix* p_matrix)
{
	p_obj->m_local = *p_matrix;
	p_obj->m_flags |= c_objectDirty;
}

// Multiplies the object's local matrix by p_matrix, renormalizing the rotation now and then.
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

void SetObjRotation(SceneObject* p_obj, int32_t p_angleX, int32_t p_angleY, int32_t p_angleZ, uint32_t p_flags)
{
	Matrix matrix;

	BuildMatrixEx(&matrix, p_angleX, p_angleY, p_angleZ, 0, 0, 0, p_flags);
	SetObjRotationMatrix(p_obj, &matrix);
}

void RotateObj(SceneObject* p_obj, int32_t p_angleX, int32_t p_angleY, int32_t p_angleZ, uint32_t p_flags)
{
	Matrix matrix;

	BuildMatrixEx(&matrix, p_angleX, p_angleY, p_angleZ, 0, 0, 0, p_flags);
	RotateObjMatrix(p_obj, &matrix);
}

void TransformShapeModel(Shape* p_shape)
{
	Model* model = p_shape->m_model;
	SceneObject* obj = p_shape->m_object;

	TransformModel(model, &obj->m_world);
	model->m_transformCount = p_shape->m_transformCount;
}

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

void SetObjTreeKind(SceneObject* p_obj, int32_t p_kind)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		p_obj->m_shape->m_kind = p_kind;
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		SetObjTreeKind(child, p_kind);
	}
}

void SetObjTreeOwner(SceneObject* p_obj, int32_t p_owner)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		p_obj->m_shape->m_owner = p_owner;
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		SetObjTreeOwner(child, p_owner);
	}
}

void SetObjTreeCollisionType(SceneObject* p_obj, int32_t p_collisionType)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		SetShapeCollisionType(p_obj->m_shape, p_collisionType);
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		SetObjTreeCollisionType(child, p_collisionType);
	}
}

void ClearObjTreeKind(SceneObject* p_obj, int32_t p_flags)
{
	SceneObject* child;

	if (p_obj->m_shape) {
		p_obj->m_shape->m_kind &= (int16_t) ~p_flags;
	}

	for (child = p_obj->m_firstChild; child; child = child->m_nextSibling) {
		ClearObjTreeKind(child, p_flags);
	}
}

// Recomputes the world matrices of the object and its subtree, and their shapes' centers.
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

SceneObject* GetObjRoot(SceneObject* p_obj)
{
	while (p_obj->m_parent) {
		p_obj = p_obj->m_parent;
	}

	return p_obj;
}

SceneObject* GetObjParent(SceneObject* p_obj)
{
	return p_obj->m_parent;
}

SceneObject* GetObjFirstChild(SceneObject* p_obj)
{
	return p_obj->m_firstChild;
}

SceneObject* GetObjNextSibling(SceneObject* p_obj)
{
	return p_obj->m_nextSibling;
}

Matrix* GetObjLocalMatrix(SceneObject* p_obj)
{
	return &p_obj->m_local;
}

void SetObjLocalMatrix(SceneObject* p_obj, Matrix* p_matrix)
{
	p_obj->m_local = *p_matrix;
}

Matrix* GetObjWorldMatrix(SceneObject* p_obj)
{
	return &p_obj->m_world;
}

void SetObjWorldMatrix(SceneObject* p_obj, Matrix* p_matrix)
{
	p_obj->m_world = *p_matrix;
}

// Unlinks the object from its parent, keeping its world placement.
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

SceneObject* FindObjByName(SceneObject* p_obj, const char* p_name)
{
	SceneObject* child;

	if (p_obj->m_name && strcasecmp(p_obj->m_name, p_name) == 0) {
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
SceneObject* GetObjChild(SceneObject* p_obj, int32_t p_index)
{
	SceneObject* result;
	SceneObject* child;
	int32_t count;

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

SceneObject* FindObjByPart(SceneObject* p_obj, uint32_t p_partId)
{
	SceneObject* child;
	uint32_t id = p_partId;

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
void RaisePartDamageLevel(SceneObject* p_obj, int32_t p_level, uint32_t p_partId)
{
	int32_t current;
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
void BlowOffPart(SceneObject* p_obj, uint32_t p_partId)
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

int32_t GetObjSize(void)
{
	return sizeof(SceneObject);
}
