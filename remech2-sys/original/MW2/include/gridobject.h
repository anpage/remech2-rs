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

	extern MechS32 g_gridObjectShown;
	extern SceneObject* g_gridObject;
	extern MechS32 g_gridCellSize;
	extern MechS32 g_gridObjectSet;
	extern MechS32 g_gridObjectPlaced;
	extern MechS32 g_gridObjectSnaps;
	extern Vector3 g_gridCell;
	extern Vector3 g_gridAnchor;
	extern MechS32 g_gridSnapDistance;

	void SetGridObject(SceneObject* p_obj);
	void UpdateGridObject(void);
	void ShowGridObject(MechS32 p_enable);

#ifdef __cplusplus
}
#endif

#endif // GRIDOBJECT_H
