#ifndef GRIDOBJECT_H
#define GRIDOBJECT_H

#include "object.h"
#include "types.h"
#include "vector3.h"

// The functions and globals of gridobject.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_gridObjectShown;
	extern SceneObject* g_gridObject;
	extern int32_t g_gridCellSize;
	extern int32_t g_gridObjectSet;
	extern int32_t g_gridObjectPlaced;
	extern int32_t g_gridObjectSnaps;
	extern Vector3 g_gridCell;
	extern Vector3 g_gridAnchor;
	extern int32_t g_gridSnapDistance;

	void SetGridObject(SceneObject* p_obj);
	void UpdateGridObject(void);
	void ShowGridObject(int32_t p_enable);

#ifdef __cplusplus
}
#endif

#endif // GRIDOBJECT_H
