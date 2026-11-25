/* Builds shapes from "WTBO" shape records: the vertices, scaled and offset by the settings
   here, the faces with their mapped ids, and the objects that carry the shapes. */
#include "wtbshapes.h"

#include "collision.h"
#include "object.h"
#include "players.h"
#include "shape.h"
#include "shapelists.h"
#include "team.h"
#include "types.h"
#include "wtbface.h"
#include "wtbheader.h"
#include "wtbvertex.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

const int32_t g_wtbTag = 0x4f425457;

int32_t g_shapeLoadError = 0;

int32_t g_shapeOffsetX = 0;

int32_t g_shapeOffsetY = 0;

int32_t g_shapeOffsetZ = 0;

int32_t g_shapeScaleX = 1;

int32_t g_shapeScaleY = 1;

int32_t g_shapeScaleZ = 1;

uint32_t g_shapeFlags = 0;

uint32_t g_faceIdCount = 0;

int32_t g_subShapeCollisionType = 4;

int32_t g_shapeHasObject;

uint32_t* g_faceIds;

int32_t g_shapeHasKey;

int32_t g_shapeOwnerSet;

int32_t g_shapeOwnerKind;

int32_t g_shapeOwner;

void SetFaceIds(uint32_t* p_ids, uint32_t p_count)
{
	g_faceIds = p_ids;
	g_faceIdCount = p_count;
}

void SetShapeOffset(int32_t p_x, int32_t p_y, int32_t p_z)
{
	g_shapeOffsetX = p_x;
	g_shapeOffsetY = p_y;
	g_shapeOffsetZ = p_z;
}

void SetShapeScale(int32_t p_x, int32_t p_y, int32_t p_z)
{
	g_shapeScaleX = p_x;
	if (!g_shapeScaleX) {
		g_shapeScaleX = 1;
	}

	g_shapeScaleY = p_y;
	if (!g_shapeScaleY) {
		g_shapeScaleY = 1;
	}

	g_shapeScaleZ = p_z;
	if (!g_shapeScaleZ) {
		g_shapeScaleZ = 1;
	}
}

void SetShapeFlags(uint32_t p_flags)
{
	g_shapeFlags = p_flags;
}

// Reads the shape records from p_offset up to p_size; the first becomes the shape, the others
// its levels of detail.
Shape* LoadShapes(uint8_t* p_data, int32_t* p_offset, int32_t p_size, SceneObject* p_parent)
{
	Shape* next;
	Shape* shape;
	int32_t count;

	shape = NULL;
	next = NULL;
	count = 0;
	g_shapeLoadError = 0;

	if (*p_offset < p_size && LoadShapeRecord(p_data, p_offset, &shape, p_parent, &count) == 0) {
		next = shape;
		while (*p_offset < p_size && LoadShapeRecord(p_data, p_offset, &next, p_parent, &count) == 0) {
			if (next) {
				SetShapeLoadFlags(next, g_shapeFlags);
			}
		}
	}

	if (shape) {
		SetShapeLoadFlags(shape, g_shapeFlags);
	}

	return shape;
}

int32_t LoadShapeRecord(uint8_t* p_data, int32_t* p_offset, Shape** p_shape, SceneObject* p_parent, int32_t* p_count)
{
	char* suffix;
	int32_t checksum;
	uint16_t* indices;
	int32_t key;
	int32_t facesSize;
	uint8_t* extra;
	int32_t extraSize;
	SceneObject* obj;
	int32_t count;
	WtbVertex* vertices;
	int32_t id;
	int32_t u;
	int32_t vertexCount;
	int32_t v;
	int32_t x;
	char name[40];
	int32_t size;
	uint8_t* face;
	uint8_t* faces;
	int32_t y;
	int32_t i;
	WtbHeader* header;
	uint8_t* record;
	int32_t z;
	int32_t j;
	struct Face* polygon;
	WtbVertex* vertex;

	key = 0;
	checksum = 0;
	if (!*p_shape) {
		g_shapeHasObject = 0;
		g_shapeHasKey = 0;
	}

	record = p_data + *p_offset;
	header = (WtbHeader*) record;
	if (header->m_tag != g_wtbTag) {
		g_shapeLoadError = -2;
		return -1;
	}

	if (header->m_flags & 0x2000) {
		g_shapeHasObject = 1;
	}
	else if (header->m_flags & 0x1000) {
		g_shapeHasKey = 1;
	}

	if (header->m_name[0] > 0x80) {
		for (i = 0; i < 16; i++) {
			j = header->m_name[i];
			j = 0x100 - j;
			header->m_name[i] = j;
		}
	}

	strncpy(name, header->m_name, 16);
	key = 0;
	if (g_shapeHasKey && (suffix = strchr(name, '_')) != NULL) {
		*suffix = '\0';
		suffix++;
		if (isdigit(*suffix)) {
			key = atol(suffix);
		}
	}

	vertices = (WtbVertex*) (record + sizeof(WtbHeader));
	faces = (uint8_t*) (vertices + header->m_vertexCount);
	face = faces;
	extraSize = 0;
	i = header->m_faceCount;
	while (i--) {
		count = ((WtbFace*) face)->m_count;
		if (count > 4) {
			size = 0x12;
		}
		else {
			size = 0xc;
		}

		extraSize += count * 4;
		face += size;
	}

	if (g_shapeHasObject) {
		vertexCount = header->m_vertexCount - 1;
	}
	else {
		vertexCount = header->m_vertexCount;
	}

	if (!*p_shape || g_shapeHasObject) {
		*p_shape = CreateShape(vertexCount, header->m_faceCount, extraSize, (void**) &extra);
		if (!*p_shape) {
			g_shapeLoadError = -2;
			return -1;
		}

		SetModelKey(*p_shape, key);
	}
	else if (g_shapeHasKey && !AddModel(*p_shape, key, vertexCount, header->m_faceCount, extraSize, (void**) &extra)) {
		g_shapeLoadError = -2;
		return -1;
	}

	for (i = 0; i < header->m_vertexCount; i++) {
		vertex = &vertices[i];
		x = vertex->m_x;
		y = vertex->m_y;
		z = vertex->m_z;
		u = vertex->m_u;
		v = vertex->m_v;
		if (i < vertexCount) {
			AddShapeVertex(
				*p_shape,
				x * g_shapeScaleX + g_shapeOffsetX,
				y * g_shapeScaleY + g_shapeOffsetY,
				z * g_shapeScaleZ + g_shapeOffsetZ,
				u,
				v
			);
		}

		checksum += vertex->m_x;
		checksum += vertex->m_y;
		checksum += vertex->m_z;
		checksum += vertex->m_u;
		checksum += vertex->m_v;
		checksum = checksum % 0x100000;
	}

	face = faces;
	facesSize = 0;
	for (i = 0; i < header->m_faceCount; i++) {
		count = ((WtbFace*) face)->m_count;
		if (count > 4) {
			size = 0x12;
		}
		else {
			size = 0xc;
		}

		id = MapFaceId(((WtbFace*) face)->m_id);
		polygon = AddShapeFace(*p_shape, id, extra);
		if (!polygon) {
			g_shapeLoadError = -9;
			return -1;
		}

		extra += count * 4;
		indices = ((WtbFace*) face)->m_indices;
		for (j = 0; j < count; j++) {
			AddShapeFaceIndex(*p_shape, polygon, indices[j]);
			checksum += indices[j];
		}

		for (j = count; j < 4; j++) {
			checksum += indices[j];
		}

		checksum += ((WtbFace*) face)->m_id;
		checksum += count;
		checksum = checksum % 0x100000;
		facesSize += size;
		face += size;
	}

	*p_offset = *p_offset + sizeof(WtbHeader);
	*p_offset += header->m_vertexCount * sizeof(WtbVertex);
	*p_offset += facesSize;

	if (header->m_checksum != checksum) {
		g_shapeLoadError = -2;
		return -1;
	}

	ComputeNormalsAndBounds(*p_shape);
	g_shapeLoadError = 0;

	if (g_shapeHasObject) {
		if ((!p_parent || (obj = GetObjChild(p_parent, *p_count)) == NULL) &&
			(obj = CreateObj(p_parent, 0x14)) == NULL) {
			g_shapeLoadError = -2;
			return -1;
		}

		if (obj->m_shape && (obj = CreateObj(p_parent, 0xc)) == NULL) {
			g_shapeLoadError = -2;
			return -1;
		}

		SetObjPosition(obj, vertex->m_x, vertex->m_y, vertex->m_z);
		SetObjShape(obj, *p_shape);
		SetShapeObject(*p_shape, obj);
		UpdateObj(obj);
		if (*p_count) {
			AddSceneShape(*p_shape);
			SetShapeCollisionType(*p_shape, g_subShapeCollisionType);
			SetShapeKind(*p_shape, 0x50);
		}

		(*p_count)++;
	}

	return 0;
}

// Maps a face id: ids 0 and 0x14 take the team colour of player g_shapeOwner while
// g_shapeOwnerSet is set; ids with bit 15 index g_faceIds.
uint32_t MapFaceId(uint32_t p_id)
{
	int32_t team;
	int32_t offset;

	offset = 0;
	if (g_shapeOwnerSet && ((p_id & 0xff) == 0 || (p_id & 0xff) == 0x14)) {
		if (g_shapeOwnerKind == 0x100) {
			team = g_players[g_shapeOwner]->m_team;
			offset = g_teams[team].m_affiliation;
		}
		else if (g_gameThings[g_shapeOwner].m_affiliation != -1) {
			offset = g_gameThings[g_shapeOwner].m_affiliation;
		}

		p_id += offset;
		return p_id;
	}

	if (!(p_id & 0x8000)) {
		return p_id;
	}

	p_id &= 0x7fff;
	if (!g_faceIds) {
		return p_id;
	}

	if (p_id > g_faceIdCount) {
		return p_id;
	}

	return g_faceIds[p_id];
}
