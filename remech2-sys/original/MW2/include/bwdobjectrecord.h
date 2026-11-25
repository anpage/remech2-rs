#ifndef BWDOBJECTRECORD_H
#define BWDOBJECTRECORD_H

#include "bwdrecord.h"
#include "types.h"
#include "xform.h"

#pragma pack(push, 1)

// A world stream's object record (CreateObjectNode): a shape, from a resource or a file, with its
// scale, rotation and position.
typedef struct BwdObjectRecord {
	BwdRecord m_header;       // 0x00
	int16_t m_id;             // 0x08
	int16_t m_parent;         // 0x0a — an object id; -1 none, -2 placed in the world
	int16_t m_kind;           // 0x0c — SetShapeCollisionType's, 0 to 7
	Xform m_xform;            // 0x0e — scale, rotation, position
	int16_t m_flags;          // 0x32 — SetShapeFlags'
	int32_t m_shapeKind;      // 0x34 — the shape's m_kind
	int16_t m_resource;       // 0x38 — a shape resource, or -1 for m_file
	char m_file[0x46 - 0x3a]; // 0x3a — the shape file, .wtb added if it has no extension
} BwdObjectRecord;

#pragma pack(pop)

#endif // BWDOBJECTRECORD_H
