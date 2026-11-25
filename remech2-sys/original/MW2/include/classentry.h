#ifndef CLASSENTRY_H
#define CLASSENTRY_H

#include "types.h"

struct SceneObject;
struct Shape;

// An entry of g_classTable: a shape a player's mech uses, with up to five levels of detail.
typedef struct ClassEntry {
	int32_t m_owner;          // a player index; -1 free, -2 until ClaimNewClassEntries
	int32_t m_loadedLevel;    // -1 while m_shape isn't loaded
	int32_t m_resourceIds[5]; // by level; -1 unset
	int32_t m_parent;
	int32_t m_partId;
	struct Shape* m_shape;
	struct SceneObject* m_obj;
	uint32_t m_x;
	uint32_t m_y;
	uint32_t m_z;
	uint16_t m_kinds[5]; // by level; the shape kind in bits 4-7
	int16_t m_released;
} ClassEntry;

#endif // CLASSENTRY_H
