#include "shape.h"

#include "approxlen.h"
#include "boundbox.h"
#include "face.h"
#include "object.h"
#include "shapegeom.h"
#include "shapelists.h"
#include "simmain.h"
#include "types.h"
#include "vertex.h"

#include <math.h>
#include <string.h>

// The flags a new shape starts with (CreateShape).
uint32_t g_newShapeFlags = 0;

uint32_t GetNewShapeFlags(void)
{
	return g_newShapeFlags;
}

// Sets the flags new shapes start with; returns the previous ones.
uint32_t SetNewShapeFlags(uint32_t p_flags)
{
	uint32_t previous;

	previous = g_newShapeFlags;
	g_newShapeFlags = p_flags;
	return previous;
}

// Selects the shape's first model.
void SelectFirstModel(Shape* p_shape)
{
	SelectModel(p_shape, p_shape->m_models);
}

void SelectModel(Shape* p_shape, Model* p_model)
{
	p_shape->m_model = p_model;
}

// Selects the next model, wrapping around to the first.
void SelectNextModel(Shape* p_shape)
{
	if (!p_shape->m_model || !p_shape->m_model->m_next) {
		SelectModel(p_shape, p_shape->m_models);
	}
	else {
		SelectModel(p_shape, p_shape->m_model->m_next);
	}
}

// Allocates a model with room for the vertices, the faces and p_extra more bytes (returned in
// p_extraData), inserts it into the shape's list by p_key and selects it.
Model* AddModel(
	Shape* p_shape,
	int32_t p_key,
	int32_t p_vertexCount,
	int32_t p_faceCount,
	int32_t p_extra,
	void** p_extraData
)
{
	Model* model;
	int32_t vertexBytes;
	Model* cursor;
	void* memory;
	uint32_t size;
	int32_t faceBytes;

	size = 0;
	faceBytes = 0;
	vertexBytes = 0;
	if (!p_shape) {
		return NULL;
	}

	vertexBytes = p_vertexCount * sizeof(Vertex);
	faceBytes = p_faceCount * sizeof(Face);
	size = p_extra + vertexBytes + faceBytes + sizeof(Model);
	memory = MechHeapAlloc(g_primaryHeap, size);
	if (!memory) {
		return NULL;
	}

	memset(memory, 0, size);
	model = memory;
	model->m_key = p_key;
	model->m_vertexCount = 0;
	model->m_faceCount = 0;
	model->m_transformCount = 0;
	model->m_faceOffset = (uint16_t) (vertexBytes + sizeof(Model));
	*p_extraData = (Face*) ((uint8_t*) model + model->m_faceOffset) + p_faceCount;
	SelectModel(p_shape, model);

	if (!p_shape->m_models) {
		p_shape->m_models = model;
		model->m_next = NULL;
		return model;
	}

	cursor = p_shape->m_models;
	if (cursor->m_key > p_key) {
		model->m_next = cursor;
		p_shape->m_models = model;
		return model;
	}

	while (cursor->m_next) {
		if (cursor->m_next->m_key > p_key) {
			break;
		}

		cursor = cursor->m_next;
	}

	model->m_next = cursor->m_next;
	cursor->m_next = model;
	return model;
}

// Selects the first model whose key is at most p_key.
void SelectModelByKey(Shape* p_shape, int32_t p_key)
{
	Model* model;

	for (model = p_shape->m_models; model; model = model->m_next) {
		if (model->m_key <= p_key) {
			SelectModel(p_shape, model);
			break;
		}
	}
}

void SetModelKey(Shape* p_shape, int32_t p_key)
{
	Model* model;

	model = p_shape->m_model;
	if (!model) {
		return;
	}

	model->m_key = p_key;
}

int32_t GetModelKey(Shape* p_shape)
{
	Model* model;

	model = p_shape->m_model;
	if (!model) {
		return 0;
	}

	return model->m_key;
}

// Allocates a shape with one model (AddModel).
Shape* CreateShape(int32_t p_vertexCount, int32_t p_faceCount, int32_t p_extra, void** p_extraData)
{
	Shape* shape;

	shape = MechHeapAlloc(g_primaryHeap, sizeof(Shape));
	if (!shape) {
		return NULL;
	}

	shape->m_flags = g_newShapeFlags | 0x8000;
	shape->m_object = NULL;
	shape->m_model = shape->m_models = NULL;
	if (!AddModel(shape, 0, p_vertexCount, p_faceCount, p_extra, p_extraData)) {
		MechHeapFree(g_primaryHeap, shape);
		return NULL;
	}

	shape->m_kind = 0;
	shape->m_partId = 0;
	shape->m_owner = 0;
	shape->m_centerX = shape->m_centerY = shape->m_centerZ = 0;
	shape->m_modelCenterX = shape->m_modelCenterY = shape->m_modelCenterZ = 0;
	shape->m_radius = 0;
	shape->m_collisionData = NULL;
	shape->m_prev = shape->m_next = NULL;
	shape->m_prevCollider = shape->m_nextCollider = NULL;
	shape->m_collisionType = 4;
	shape->m_transformCount = 0;
	return shape;
}

// Appends a vertex to the selected model.
void AddShapeVertex(Shape* p_shape, int32_t p_x, int32_t p_y, int32_t p_z, uint32_t p_u, uint32_t p_v)
{
	Model* model;
	Vertex* vertex;

	model = p_shape->m_model;
	if (!model) {
		return;
	}

	vertex = (Vertex*) (model + 1) + model->m_vertexCount;
	vertex->m_worldX = vertex->m_modelX = p_x;
	vertex->m_worldY = vertex->m_modelY = p_y;
	vertex->m_worldZ = vertex->m_modelZ = p_z;
	vertex->m_u = p_u;
	vertex->m_v = p_v;
	model->m_vertexCount++;
}

// Appends a face to the selected model, its vertex indices at p_indices.
Face* AddShapeFace(Shape* p_shape, uint16_t p_color, uint8_t* p_indices)
{
	Model* model;
	Face* face;

	model = p_shape->m_model;
	if (!model) {
		return NULL;
	}

	face = (Face*) ((uint8_t*) model + model->m_faceOffset) + model->m_faceCount;
	face->m_indexOffset = (uint16_t) (p_indices - (uint8_t*) face);
	face->m_shape = p_shape;
	face->m_color = p_color;
	face->m_indexCount = 0;
	model->m_faceCount++;
	return face;
}

// Appends a vertex index to a face of the selected model.
void AddShapeFaceIndex(Shape* p_shape, Face* p_face, uint32_t p_index)
{
	Model* model;

	model = p_shape->m_model;
	if (!model) {
		return;
	}

	((uint8_t*) p_face)[p_face->m_indexOffset + p_face->m_indexCount] = (uint8_t) p_index;
	p_face->m_indexCount++;
}

void FreeModel(Model* p_model)
{
	if (!p_model) {
		return;
	}

	MechHeapFree(g_primaryHeap, p_model);
}

// Removes the selected model from the shape's list and frees it.
void RemoveSelectedModel(Shape* p_shape)
{
	Model* model;
	Model* cursor;

	model = p_shape->m_model;
	if (!model || !p_shape->m_models) {
		return;
	}

	if (p_shape->m_models == model) {
		p_shape->m_models = model->m_next;
	}
	else {
		for (cursor = p_shape->m_models; cursor->m_next; cursor = cursor->m_next) {
			if (cursor->m_next == model) {
				break;
			}
		}

		if (!cursor->m_next) {
			return;
		}

		cursor->m_next = model->m_next;
	}

	FreeModel(model);
}

// Frees the shape and its models.
void FreeShape(Shape* p_shape)
{
	Model* model;
	Model* current;

	model = p_shape->m_models;
	while (model) {
		current = model;
		model = current->m_next;
		FreeModel(current);
	}

	FreeBoundBox(p_shape);
	MechHeapFree(g_primaryHeap, p_shape);
}

void SetShapeState(Shape* p_shape, int32_t p_flags)
{
	if (p_shape) {
		p_shape->m_flags = (p_shape->m_flags & ~0x7ef0) | (p_flags & 0x7ef0) | 0x8000;
	}
}

uint32_t GetShapeState(Shape* p_shape)
{
	if (p_shape) {
		return p_shape->m_flags & 0x7ef0;
	}
	else {
		return 0;
	}
}

void SetShapeKind(Shape* p_shape, int32_t p_kind)
{
	if (p_shape) {
		p_shape->m_kind = p_kind;
	}
}

void SetShapePartId(Shape* p_shape, uint16_t p_partId)
{
	p_shape->m_partId = p_partId;
}

void SetShapeOwner(Shape* p_shape, uint16_t p_owner)
{
	p_shape->m_owner = p_owner;
}

uint32_t GetShapeKind(Shape* p_shape)
{
	return p_shape->m_kind;
}

uint32_t GetShapePartId(Shape* p_shape)
{
	return p_shape->m_partId;
}

uint32_t GetShapeOwner(Shape* p_shape)
{
	return p_shape->m_owner;
}

int32_t GetShapeBounds(Shape* p_shape, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	if (p_x) {
		*p_x = p_shape->m_centerX;
	}

	if (p_y) {
		*p_y = p_shape->m_centerY;
	}

	if (p_z) {
		*p_z = p_shape->m_centerZ;
	}

	return p_shape->m_radius;
}

// Computes the selected model's face normals, then the shape's center and radius (a shape
// LoadShapeRecord has just built).
void ComputeNormalsAndBounds(Shape* p_shape)
{
	Model* model;
	int32_t i;

	model = p_shape->m_model;
	if (!model) {
		return;
	}

	i = model->m_faceCount;
	while (i--) {
		ComputeFaceNormal((Face*) ((uint8_t*) model + model->m_faceOffset) + i, (Vertex*) (model + 1));
	}

	ComputeShapeBounds(p_shape);
}

// Sets p_shape's center to the middle of its first model's bounding box, and its radius to the
// distance from there to the farthest vertex.
void ComputeShapeBounds(Shape* p_shape)
{
	double dz;
	Model* model;
	double max;
	int32_t minX;
	int32_t i;
	int32_t minY;
	int32_t minZ;
	double d;
	int32_t maxX;
	int32_t maxY;
	double dx;
	int32_t maxZ;
	Vertex* v;
	double dy;

	model = p_shape->m_models;
	if (!model) {
		return;
	}

	i = model->m_vertexCount - 1;
	v = (Vertex*) (model + 1) + i;
	minX = maxX = v->m_worldX;
	minY = maxY = v->m_worldY;
	minZ = maxZ = v->m_worldZ;
	while (i--) {
		v = (Vertex*) (model + 1) + i;
		if (v->m_worldX < minX) {
			minX = v->m_worldX;
		}
		if (v->m_worldY < minY) {
			minY = v->m_worldY;
		}
		if (v->m_worldZ < minZ) {
			minZ = v->m_worldZ;
		}
		if (v->m_worldX > maxX) {
			maxX = v->m_worldX;
		}
		if (v->m_worldY > maxY) {
			maxY = v->m_worldY;
		}
		if (v->m_worldZ > maxZ) {
			maxZ = v->m_worldZ;
		}
	}

	p_shape->m_modelCenterX = p_shape->m_centerX = (maxX + minX) >> 1;
	p_shape->m_modelCenterY = p_shape->m_centerY = (maxY + minY) >> 1;
	p_shape->m_modelCenterZ = p_shape->m_centerZ = (maxZ + minZ) >> 1;

	max = 0.0;
	i = model->m_vertexCount;
	while (i--) {
		v = (Vertex*) (model + 1) + i;
		dx = v->m_worldX - p_shape->m_centerX;
		dy = v->m_worldY - p_shape->m_centerY;
		dz = v->m_worldZ - p_shape->m_centerZ;
		d = dx * dx + dy * dy + dz * dz;
		if (d > max) {
			max = d;
		}
	}

	p_shape->m_radius = (int32_t) sqrt(max);
}

// Sets the face's normal (both copies) from its vertices: a triangle's directly, otherwise from
// its longest edge and the vertex that gives the largest area with it, stopping at the first
// area over 16. A face of fewer than 3 vertices keeps the default (0x20000000, 0, 0).
void ComputeFaceNormal(Face* p_face, Vertex* p_vertices)
{
	int32_t length;
	int32_t nx;
	Vertex* vertex;
	int32_t ny;
	int32_t nz;
	Vertex* prev;
	Vertex* a;
	int32_t best;
	int32_t i;
	int32_t area;
	Vertex* b;
	int32_t bestArea;
	Vertex* v0;
	Vertex* v1;
	Vertex* v2;

	i = 0;
	best = 0;
	p_face->m_modelNormalX = p_face->m_normal[0] = 0x20000000;
	p_face->m_modelNormalY = p_face->m_normal[1] = 0;
	p_face->m_modelNormalZ = p_face->m_normal[2] = 0;
	if (p_face->m_indexCount < 3) {
		return;
	}

	if (p_face->m_indexCount == 3) {
		v0 = &p_vertices[((uint8_t*) p_face)[p_face->m_indexOffset]];
		v1 = &p_vertices[((uint8_t*) p_face)[p_face->m_indexOffset + 1]];
		v2 = &p_vertices[((uint8_t*) p_face)[p_face->m_indexOffset + 2]];
		ComputeTriangleNormal(
			v0->m_modelX,
			v0->m_modelY,
			v0->m_modelZ,
			v1->m_modelX,
			v1->m_modelY,
			v1->m_modelZ,
			v2->m_modelX,
			v2->m_modelY,
			v2->m_modelZ,
			&nx,
			&ny,
			&nz
		);
		p_face->m_modelNormalX = p_face->m_normal[0] = nx;
		p_face->m_modelNormalY = p_face->m_normal[1] = ny;
		p_face->m_modelNormalZ = p_face->m_normal[2] = nz;
		return;
	}

	prev = &p_vertices[((uint8_t*) p_face)[p_face->m_indexOffset]];
	for (i = p_face->m_indexCount; i--; prev = vertex) {
		vertex = &p_vertices[((uint8_t*) p_face)[p_face->m_indexOffset + i]];
		length = ApproximateVectorLength(
			prev->m_modelX - vertex->m_modelX,
			prev->m_modelY - vertex->m_modelY,
			prev->m_modelZ - vertex->m_modelZ
		);
		if (length > best) {
			best = length;
			a = vertex;
			b = prev;
		}
	}

	if (best == 0) {
		return;
	}

	bestArea = -1;
	i = p_face->m_indexCount;
	while (i--) {
		vertex = &p_vertices[((uint8_t*) p_face)[p_face->m_indexOffset + i]];
		if (vertex != a && vertex != b) {
			area = ComputeTriangleNormal(
				a->m_modelX,
				a->m_modelY,
				a->m_modelZ,
				b->m_modelX,
				b->m_modelY,
				b->m_modelZ,
				vertex->m_modelX,
				vertex->m_modelY,
				vertex->m_modelZ,
				&nx,
				&ny,
				&nz
			);
			if (area > bestArea) {
				p_face->m_modelNormalX = p_face->m_normal[0] = nx;
				p_face->m_modelNormalY = p_face->m_normal[1] = ny;
				p_face->m_modelNormalZ = p_face->m_normal[2] = nz;
				bestArea = area;
				if (area > 16) {
					return;
				}
			}
		}
	}
}

// Returns the selected model's vertex and face counts.
void GetModelCounts(Shape* p_shape, int32_t* p_vertexCount, int32_t* p_faceCount)
{
	Model* model;

	model = p_shape->m_model;
	if (!model) {
		return;
	}

	if (p_vertexCount) {
		*p_vertexCount = model->m_vertexCount;
	}

	if (p_faceCount) {
		*p_faceCount = model->m_faceCount;
	}
}

// Calls p_fn for each shape of the list after p_shape.
void ForEachShape(Shape* p_shape, void (*p_fn)(Shape*))
{
	Shape* shape;
	Shape* next;

	if (!p_shape) {
		return;
	}

	shape = p_shape->m_next;
	while (shape) {
		next = shape->m_next;
		p_fn(shape);
		shape = next;
	}
}

// Returns a vertex's transformed position.
void GetTransformedVertex(Shape* p_shape, int32_t p_index, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	Model* model;
	Vertex* vertex;

	model = p_shape->m_model;
	if (!model) {
		return;
	}

	vertex = &((Vertex*) (model + 1))[p_index];
	if (p_x) {
		*p_x = vertex->m_worldX;
	}

	if (p_y) {
		*p_y = vertex->m_worldY;
	}

	if (p_z) {
		*p_z = vertex->m_worldZ;
	}
}

// Returns a vertex's position in the model.
void GetVertexPosition(Shape* p_shape, int32_t p_index, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	Model* model;
	Vertex* vertex;

	model = p_shape->m_model;
	if (!model) {
		return;
	}

	vertex = &((Vertex*) (model + 1))[p_index];
	if (p_x) {
		*p_x = vertex->m_modelX;
	}

	if (p_y) {
		*p_y = vertex->m_modelY;
	}

	if (p_z) {
		*p_z = vertex->m_modelZ;
	}
}

// Returns a face of the selected model and up to p_max of its vertex indices.
void GetShapeFace(
	Shape* p_shape,
	int32_t p_index,
	uint32_t* p_color,
	uint32_t* p_count,
	uint32_t* p_indices,
	int32_t p_max
)
{
	Model* model;
	Face* face;
	int32_t count;

	model = p_shape->m_model;
	if (!model) {
		return;
	}

	face = (Face*) ((uint8_t*) model + model->m_faceOffset) + p_index;
	if (p_color) {
		*p_color = face->m_color;
	}

	count = face->m_indexCount;
	if (p_count) {
		*p_count = count;
	}

	if (p_indices) {
		if (count > p_max) {
			count = p_max;
		}

		while (count--) {
			p_indices[count] = ((uint8_t*) face)[face->m_indexOffset + count];
		}
	}
}

void SetFaceColor(Shape* p_shape, int32_t p_index, int32_t p_color)
{
	Model* model;

	model = p_shape->m_model;
	if (!model) {
		return;
	}

	if (p_index < model->m_faceCount) {
		((Face*) ((uint8_t*) model + model->m_faceOffset) + p_index)->m_color = p_color;
	}
}

struct SceneObject* GetShapeObject(Shape* p_shape)
{
	return p_shape->m_object;
}

void SetShapeObject(Shape* p_shape, struct SceneObject* p_object)
{
	p_shape->m_object = p_object;
}

uint32_t GetShapeLoadFlags(Shape* p_shape)
{
	return p_shape->m_flags & 0x10f;
}

void SetShapeLoadFlags(Shape* p_shape, uint32_t p_flags)
{
	p_shape->m_flags = (p_shape->m_flags & ~0x10f) | (p_flags & 0x10f) | 0x8000;
}

// Unlinks the shape and frees it.
void DestroyShape(Shape* p_shape)
{
	if (p_shape) {
		RemoveSceneShape(p_shape);
		FreeShape(p_shape);
	}
}

// Detaches the shape from its scene object and frees it. A ShapeCallback (DestroyObjTree).
void DestroyObjShape(Shape* p_shape)
{
	struct SceneObject* obj;

	obj = GetShapeObject(p_shape);
	if (obj) {
		SetObjShape(obj, NULL);
	}

	DestroyShape(p_shape);
}

// Returns the bytes the shape, its models and its bounding data take.
int32_t GetShapeMemorySize(Shape* p_shape)
{
	Model* model;
	Vertex* vertex;
	int32_t size;
	int32_t i;
	Face* face;

	size = sizeof(Shape);
	if (p_shape->m_object) {
		size += sizeof(struct SceneObject);
	}

	for (model = p_shape->m_models; model; model = model->m_next) {
		size += sizeof(Model);
		vertex = (Vertex*) (model + 1);
		for (i = model->m_vertexCount; i--; vertex++) {
			size += sizeof(Vertex);
		}

		face = (Face*) ((uint8_t*) model + model->m_faceOffset);
		for (i = model->m_faceCount; i--; face++) {
			size += face->m_indexCount + sizeof(Face);
		}
	}

	return size + GetBoundBoxSize(p_shape);
}
