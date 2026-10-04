#include "shapelists.h"

#include "decomp.h"
#include "shape.h"
#include "shapelisthead.h"
#include "types.h"

#include <stddef.h>

DECOMP_SIZE_ASSERT(ShapeListHead, 0x18)

// The shape lists, each a ShapeListHead posing as a shape: the scene's shapes (drawn; its head
// also starts the collision list), the hidden ones (flag 0x1000: counted, not drawn), and the
// detached ones, drawn on their own (the sky objects SecondRender finds).
// GLOBAL: MW2 0x100ad5e8
Shape* g_sceneShapes = NULL;

// GLOBAL: MW2 0x100ad5ec
Shape* g_hiddenShapes = NULL;

// GLOBAL: MW2 0x100bef10
ShapeListHead g_sceneShapeHead;

// GLOBAL: MW2 0x100bef28
ShapeListHead g_hiddenShapeHead;

// GLOBAL: MW2 0x100bef40
ShapeListHead g_detachedShapeHead;

// FUNCTION: MW2 0x1006d680
void InitShapeLists(void)
{
	g_sceneShapeHead.m_prev = g_sceneShapeHead.m_prevCollider = NULL;
	g_sceneShapeHead.m_next = g_sceneShapeHead.m_nextCollider = NULL;
	g_sceneShapeHead.m_flags = 0x4000;
	g_hiddenShapeHead.m_prev = g_hiddenShapeHead.m_prevCollider = NULL;
	g_hiddenShapeHead.m_next = g_hiddenShapeHead.m_nextCollider = NULL;
	g_hiddenShapeHead.m_flags = 0x4000;
	g_detachedShapeHead.m_prev = g_detachedShapeHead.m_prevCollider = NULL;
	g_detachedShapeHead.m_next = g_detachedShapeHead.m_nextCollider = NULL;
	g_detachedShapeHead.m_flags = 0x4000;
	g_sceneShapes = (Shape*) &g_sceneShapeHead;
	g_hiddenShapes = (Shape*) &g_hiddenShapeHead;
}

// Links a new shape into the scene, or the hidden list for flag 0x1000, and into the collision
// list unless it has collision type 4 (none) or flag 0x800.
// FUNCTION: MW2 0x1006d732
void AddSceneShape(Shape* p_shape)
{
	Shape* list;

	if (g_sceneShapes != (Shape*) &g_sceneShapeHead || !p_shape) {
		return;
	}

	if (p_shape->m_flags & 0x1000) {
		list = (Shape*) &g_hiddenShapeHead;
	}
	else {
		list = (Shape*) &g_sceneShapeHead;
	}

	LinkShape(p_shape, list);
	if (p_shape->m_collisionType == 4) {
		p_shape->m_flags |= 0x800;
	}

	if (!(p_shape->m_flags & 0x800)) {
		if (g_sceneShapeHead.m_nextCollider) {
			g_sceneShapeHead.m_nextCollider->m_prevCollider = p_shape;
		}

		p_shape->m_nextCollider = g_sceneShapeHead.m_nextCollider;
		g_sceneShapeHead.m_nextCollider = p_shape;
		p_shape->m_prevCollider = (Shape*) &g_sceneShapeHead;
	}
}

// Operand order: the original compares p_shape with g_sceneShapes the other way round.
// FUNCTION: MW2 0x1006d7fb
void RemoveSceneShape(Shape* p_shape)
{
	if (!p_shape || p_shape == g_sceneShapes || p_shape == (Shape*) &g_hiddenShapeHead) {
		return;
	}

	UnlinkShape(p_shape);
	if (p_shape->m_nextCollider) {
		p_shape->m_nextCollider->m_prevCollider = p_shape->m_prevCollider;
	}

	if (p_shape->m_prevCollider) {
		p_shape->m_prevCollider->m_nextCollider = p_shape->m_nextCollider;
	}

	p_shape->m_nextCollider = p_shape->m_prevCollider = NULL;
}

// FUNCTION: MW2 0x1006d88a
void DetachShape(Shape* p_shape)
{
	if (g_sceneShapes != (Shape*) &g_sceneShapeHead || !p_shape) {
		return;
	}

	UnlinkShape(p_shape);
	LinkShape(p_shape, (Shape*) &g_detachedShapeHead);
}

// FUNCTION: MW2 0x1006d8d1
void EnableShapeCollision(Shape* p_shape)
{
	if (!p_shape) {
		return;
	}

	if (!(p_shape->m_flags & 0x800) || p_shape->m_collisionType == 4) {
		return;
	}

	p_shape->m_flags &= ~0x800;
	if (!p_shape->m_prev) {
		return;
	}

	if (!g_sceneShapes) {
		return;
	}

	if (g_sceneShapes->m_nextCollider) {
		g_sceneShapes->m_nextCollider->m_prevCollider = p_shape;
	}

	p_shape->m_nextCollider = g_sceneShapes->m_nextCollider;
	g_sceneShapes->m_nextCollider = p_shape;
	p_shape->m_prevCollider = g_sceneShapes;
}

// FUNCTION: MW2 0x1006d989
void DisableShapeCollision(Shape* p_shape)
{
	if (!p_shape) {
		return;
	}

	if (p_shape->m_flags & 0x800) {
		return;
	}

	p_shape->m_flags |= 0x800;
	if (!g_sceneShapes) {
		return;
	}

	if (p_shape->m_nextCollider) {
		p_shape->m_nextCollider->m_prevCollider = p_shape->m_prevCollider;
	}

	if (p_shape->m_prevCollider) {
		p_shape->m_prevCollider->m_nextCollider = p_shape->m_nextCollider;
	}

	p_shape->m_nextCollider = p_shape->m_prevCollider = NULL;
}

// FUNCTION: MW2 0x1006da2d
void HideShape(Shape* p_shape)
{
	if (!p_shape) {
		return;
	}

	if (p_shape->m_flags & 0x1000) {
		return;
	}

	p_shape->m_flags |= 0x1000;
	if (!p_shape->m_prev) {
		return;
	}

	UnlinkShape(p_shape);
	LinkShape(p_shape, (Shape*) &g_hiddenShapeHead);
}

// FUNCTION: MW2 0x1006daa0
void ShowShape(Shape* p_shape)
{
	if (!p_shape) {
		return;
	}

	if (!(p_shape->m_flags & 0x1000) || (p_shape->m_kind & 0xf0) == 0x70) {
		return;
	}

	p_shape->m_flags &= ~0x1000;
	if (!p_shape->m_prev) {
		return;
	}

	UnlinkShape(p_shape);
	LinkShape(p_shape, (Shape*) &g_sceneShapeHead);
}

// Frees the shapes of all three lists.
// FUNCTION: MW2 0x1006db28
void FreeSceneShapes(void)
{
	Shape* shape;

	if (g_sceneShapes == (Shape*) &g_sceneShapeHead) {
		while (g_sceneShapeHead.m_next) {
			shape = g_sceneShapeHead.m_next;
			RemoveSceneShape(g_sceneShapeHead.m_next);
			FreeShape(shape);
		}

		while (g_hiddenShapeHead.m_next) {
			shape = g_hiddenShapeHead.m_next;
			RemoveSceneShape(g_hiddenShapeHead.m_next);
			FreeShape(shape);
		}

		while (g_detachedShapeHead.m_next) {
			shape = g_detachedShapeHead.m_next;
			RemoveSceneShape(g_detachedShapeHead.m_next);
			FreeShape(shape);
		}
	}
}

// Unlinks the shape from its list.
// FUNCTION: MW2 0x1006dbe2
void UnlinkShape(Shape* p_shape)
{
	if (p_shape->m_next) {
		p_shape->m_next->m_prev = p_shape->m_prev;
	}

	if (p_shape->m_prev) {
		p_shape->m_prev->m_next = p_shape->m_next;
	}

	p_shape->m_next = p_shape->m_prev = NULL;
}

// Links the shape in at the front of p_list.
// FUNCTION: MW2 0x1006dc3b
void LinkShape(Shape* p_shape, Shape* p_list)
{
	if (p_list->m_next) {
		p_list->m_next->m_prev = p_shape;
	}

	p_shape->m_next = p_list->m_next;
	p_list->m_next = p_shape;
	p_shape->m_prev = p_list;
}

// Shows the shapes of kind 0xc0 (the object density setting) with their collisions, or hides
// them without.
// FUNCTION: MW2 0x1006dc7d
void ShowDensityShapes(MechS32 p_enable)
{
	Shape* shape;
	Shape* next;

	if (p_enable) {
		for (shape = g_hiddenShapeHead.m_next; shape != NULL; shape = next) {
			next = shape->m_next;
			if ((shape->m_kind & 0xf0) == 0xc0) {
				ShowShape(shape);
				EnableShapeCollision(shape);
			}
		}
	}
	else {
		for (shape = g_sceneShapeHead.m_next; shape != NULL; shape = next) {
			next = shape->m_next;
			if ((shape->m_kind & 0xf0) == 0xc0) {
				HideShape(shape);
				DisableShapeCollision(shape);
			}
		}
	}
}
