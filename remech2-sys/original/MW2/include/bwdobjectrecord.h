#ifndef BWDOBJECTRECORD_H
#define BWDOBJECTRECORD_H

#include "bwdrecord.h"
#include "types.h"
#include "xform.h"

#pragma pack(push, 1)

// A world stream's object record (CreateObjectNode): a shape, from a resource or a file, with its
// scale, rotation and position.
typedef struct BwdObjectRecord {
	BwdRecord m_header;
	int16_t m_id;
	int16_t m_parent;         // an object id; -1 none, -2 placed in the world
	int16_t m_kind;           // SetShapeCollisionType's, 0 to 7
	Xform m_xform;            // scale, rotation, position
	int16_t m_flags;          // SetShapeFlags'
	int32_t m_shapeKind;      // the shape's m_kind
	int16_t m_resource;       // a shape resource, or -1 for m_file
	char m_file[0x46 - 0x3a]; // the shape file, .wtb added if it has no extension
} BwdObjectRecord;

#pragma pack(pop)

#endif // BWDOBJECTRECORD_H
