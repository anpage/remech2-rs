#ifndef SHAPELISTS_H
#define SHAPELISTS_H

#include "shape.h"
#include "shapelisthead.h"
#include "types.h"

// The functions and globals of shapelists.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern Shape* g_sceneShapes;
	extern Shape* g_hiddenShapes;

	extern ShapeListHead g_sceneShapeHead;
	extern ShapeListHead g_hiddenShapeHead;
	extern ShapeListHead g_detachedShapeHead;

	void InitShapeLists(void);
	void AddSceneShape(Shape* p_shape);
	void RemoveSceneShape(Shape* p_shape);
	void DetachShape(Shape* p_shape);
	void EnableShapeCollision(Shape* p_shape);
	void DisableShapeCollision(Shape* p_shape);
	void HideShape(Shape* p_shape);
	void ShowShape(Shape* p_shape);
	void FreeSceneShapes(void);
	void UnlinkShape(Shape* p_shape);
	void LinkShape(Shape* p_shape, Shape* p_list);
	void ShowDensityShapes(MechS32 p_enable);

#ifdef __cplusplus
}
#endif

#endif // SHAPELISTS_H
