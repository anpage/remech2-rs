#ifndef WTBSHAPES_H
#define WTBSHAPES_H

#include "decomp.h"
#include "object.h"
#include "shape.h"
#include "types.h"

// The functions and globals of wtbshapes.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_shapeLoadError;
	extern MechS32 g_unk0x100ba660;
	extern MechS32 g_unk0x100ba664;
	extern MechU32 g_shapeFlags;
	extern MechS32 g_subShapeCollisionType;
	extern MechS32 g_shapeOwnerSet;
	extern MechS32 g_shapeOwnerKind;
	extern MechS32 g_shapeOwner;

	void SetFaceIds(MechU32* p_ids, MechU32 p_count);
	void SetShapeOffset(MechS32 p_x, MechS32 p_y, MechS32 p_z);
	void SetShapeScale(MechS32 p_x, MechS32 p_y, MechS32 p_z);
	void SetShapeFlags(MechU32 p_flags);
	Shape* LoadShapes(MechU8* p_data, MechS32* p_offset, MechS32 p_size, SceneObject* p_parent);
	MechS32 LoadShapeRecord(
		MechU8* p_data,
		MechS32* p_offset,
		Shape** p_shape,
		SceneObject* p_parent,
		MechS32* p_count
	);
	MechU32 MapFaceId(MechU32 p_id);

#ifdef __cplusplus
}
#endif

#endif // WTBSHAPES_H
