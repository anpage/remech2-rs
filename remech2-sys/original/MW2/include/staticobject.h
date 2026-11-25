#ifndef STATICOBJECT_H
#define STATICOBJECT_H

#include "callbacks.h"
#include "shape.h"
#include "transform.h"
#include "types.h"
#include "xform.h"

struct SceneObject;

// An entry of the static object cache (g_staticObjects, 0x402 of them): the resource
// LoadStaticObject loads as its shape, the block and parent it hangs from, its placement and
// its timed callbacks. ResetStaticObject resets it.
typedef struct StaticObject {
	int32_t m_resource;    // resource ID, -1: none
	int32_t m_block;       // block, -1: none
	int32_t m_parent;      // parent entry; -1: none, -2: a shape only
	uint32_t m_flags;      // bits 0-8: flags; 12-15: kind
	uint32_t m_shapeKind;  // the kind its shape gets (0x200 once a game thing owns it)
	int32_t m_replacement; // LinkStaticObjectThing
	int32_t m_thing;       // an index into g_gameThings, -1: none
	Shape* m_shape;
	struct SceneObject* m_object;
	Xform m_xform; // the placement: scale, rotation and translation
	Matrix m_matrix;
	TimedCallback* m_callback; // AttachTaskToObj
} StaticObject;

#endif // STATICOBJECT_H
