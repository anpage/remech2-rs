/* Builds shapes from "WTBO" shape records: the vertices, scaled and offset by the settings
   here, the faces with their mapped ids, and the objects that carry the shapes. */
#include "wtbshapes.h"

#include "collision.h"
#include "decomp.h"
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

// GLOBAL: MW2 0x1009de38
const MechS32 g_wtbTag = 0x4f425457;

// GLOBAL: MW2 0x100ba65c
MechS32 g_shapeLoadError = 0;

// GLOBAL: MW2 0x100ba660
MechS32 g_unk0x100ba660 = 0;

// GLOBAL: MW2 0x100ba664
MechS32 g_unk0x100ba664 = 0;

// GLOBAL: MW2 0x100ba668
MechS32 g_shapeOffsetX = 0;

// GLOBAL: MW2 0x100ba66c
MechS32 g_shapeOffsetY = 0;

// GLOBAL: MW2 0x100ba670
MechS32 g_shapeOffsetZ = 0;

// GLOBAL: MW2 0x100ba674
MechS32 g_shapeScaleX = 1;

// GLOBAL: MW2 0x100ba678
MechS32 g_shapeScaleY = 1;

// GLOBAL: MW2 0x100ba67c
MechS32 g_shapeScaleZ = 1;

// GLOBAL: MW2 0x100ba680
MechU32 g_shapeFlags = 0;

// GLOBAL: MW2 0x100ba684
MechU32 g_faceIdCount = 0;

// GLOBAL: MW2 0x100ba688
MechS32 g_subShapeCollisionType = 4;

// GLOBAL: MW2 0x100ba68c
MechS32 g_unk0x100ba68c = 0;

// GLOBAL: MW2 0x100bfabc
MechS32 g_shapeHasObject;

// GLOBAL: MW2 0x100bfac0
MechU32* g_faceIds;

// GLOBAL: MW2 0x100bfac4
MechS32 g_shapeHasKey;

// GLOBAL: MW2 0x100bfd40
MechS32 g_shapeOwnerSet;

// GLOBAL: MW2 0x100bfd44
MechS32 g_shapeOwnerKind;

// GLOBAL: MW2 0x100bfd48
MechS32 g_shapeOwner;

// FUNCTION: MW2 0x1007f140
void SetFaceIds(MechU32* p_ids, MechU32 p_count)
{
	g_faceIds = p_ids;
	g_faceIdCount = p_count;
}

// FUNCTION: MW2 0x1007f15b
void SetShapeOffset(MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	g_shapeOffsetX = p_x;
	g_shapeOffsetY = p_y;
	g_shapeOffsetZ = p_z;
}

// FUNCTION: MW2 0x1007f17e
void SetShapeScale(MechS32 p_x, MechS32 p_y, MechS32 p_z)
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

// FUNCTION: MW2 0x1007f1e6
void SetShapeFlags(MechU32 p_flags)
{
	g_shapeFlags = p_flags;
}

// Reads the shape records from p_offset up to p_size; the first becomes the shape, the others
// its levels of detail.
// Stack slots: next and count are swapped.
// FUNCTION: MW2 0x1007f1f9
Shape* LoadShapes(MechU8* p_data, MechS32* p_offset, MechS32 p_size, SceneObject* p_parent)
{
	Shape* next;
	Shape* shape;
	MechS32 count;

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

// Stack slots: the locals are permuted. The recompiled slots take shorter encodings, so reccmp
// compares only as many bytes of the original as the recompiled function has. Operand order:
// j < count.
// FUNCTION: MW2 0x1007f2d5
MechS32 LoadShapeRecord(MechU8* p_data, MechS32* p_offset, Shape** p_shape, SceneObject* p_parent, MechS32* p_count)
{
	MechChar* suffix;
	MechS32 checksum;
	MechU16* indices;
	MechS32 key;
	MechS32 facesSize;
	MechU8* extra;
	MechS32 extraSize;
	SceneObject* obj;
	MechS32 count;
	WtbVertex* vertices;
	MechS32 id;
	MechS32 u;
	MechS32 vertexCount;
	MechS32 v;
	MechS32 x;
	MechChar name[40];
	MechS32 size;
	MechU8* face;
	MechU8* faces;
	MechS32 y;
	MechS32 i;
	WtbHeader* header;
	MechU8* record;
	MechS32 z;
	MechS32 j;
	struct Face* polygon;
	WtbVertex* vertex;

	key = 0;
	checksum = 0;
	if (!*p_shape) {
		g_shapeHasObject = 0;
		g_shapeHasKey = 0;
		g_unk0x100ba660 = 0;
		g_unk0x100ba664 = 0;
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

	switch (header->m_flags & ~0x3000) {
	case 1:
		g_unk0x100ba660 = 0;
		g_unk0x100ba664 = 1;
		break;
	case 2:
		g_unk0x100ba664 = 0;
		g_unk0x100ba660 = 1;
		break;
	default:
		g_unk0x100ba660 = g_unk0x100ba664 = 0;
		break;
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
	faces = (MechU8*) (vertices + header->m_vertexCount);
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
// FUNCTION: MW2 0x1007fae3
MechU32 MapFaceId(MechU32 p_id)
{
	MechS32 team;
	MechS32 offset;

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
