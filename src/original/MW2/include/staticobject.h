#ifndef STATICOBJECT_H
#define STATICOBJECT_H

#include "callbacks.h"
#include "decomp.h"
#include "shape.h"
#include "transform.h"
#include "types.h"
#include "xform.h"

struct SceneObject;

// An entry of the static object cache (g_staticObjects, 0x402 of them): the resource
// LoadStaticObject loads as its shape, the block and parent it hangs from, its placement and
// its timed callbacks. ResetStaticObject resets it.
// SIZE 0x7c
typedef struct StaticObject {
	MechS32 m_resource;           // 0x00 — resource ID, -1: none
	MechS32 m_block;              // 0x04 — block, -1: none
	MechS32 m_parent;             // 0x08 — parent entry; -1: none, -2: a shape only
	MechU32 m_flags;              // 0x0c — bits 0-8: flags; 12-15: kind
	undefined4 m_shapeKind;       // 0x10 — the kind its shape gets (0x200 once a game thing owns it)
	MechS32 m_replacement;        // 0x14 — LinkStaticObjectThing
	MechS32 m_thing;              // 0x18 — an index into g_gameThings, -1: none
	Shape* m_shape;               // 0x1c
	struct SceneObject* m_object; // 0x20
	Xform m_xform;                // 0x24 — the placement: scale, rotation and translation
	Matrix m_matrix;              // 0x48
	TimedCallback* m_callback;    // 0x78 — AttachTaskToObj
} StaticObject;

#endif // STATICOBJECT_H
