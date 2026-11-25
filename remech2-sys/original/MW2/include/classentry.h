#ifndef CLASSENTRY_H
#define CLASSENTRY_H

#include "types.h"

struct SceneObject;
struct Shape;

// An entry of g_classTable: a shape a player's mech uses, with up to five levels of detail.
typedef struct ClassEntry {
	int32_t m_owner;           // 0x00 — a player index; -1 free, -2 until ClaimNewClassEntries
	int32_t m_loadedLevel;     // 0x04 — -1 while m_shape isn't loaded
	int32_t m_resourceIds[5];  // 0x08 — by level; -1 unset
	int32_t m_parent;          // 0x1c
	int32_t m_partId;          // 0x20
	struct Shape* m_shape;     // 0x24
	struct SceneObject* m_obj; // 0x28
	uint32_t m_x;              // 0x2c
	uint32_t m_y;              // 0x30
	uint32_t m_z;              // 0x34
	uint16_t m_kinds[5];       // 0x38 — by level; the shape kind in bits 4-7
	int16_t m_released;        // 0x42
} ClassEntry;

#endif // CLASSENTRY_H
