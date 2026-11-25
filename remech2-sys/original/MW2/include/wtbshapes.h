#ifndef WTBSHAPES_H
#define WTBSHAPES_H

#include "object.h"
#include "shape.h"
#include "types.h"

// The functions and globals of wtbshapes.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_shapeLoadError;
	extern uint32_t g_shapeFlags;
	extern int32_t g_subShapeCollisionType;
	extern int32_t g_shapeOwnerSet;
	extern int32_t g_shapeOwnerKind;
	extern int32_t g_shapeOwner;
	extern int32_t g_shapeOffsetX;
	extern int32_t g_shapeOffsetY;
	extern int32_t g_shapeOffsetZ;
	extern int32_t g_shapeScaleX;
	extern int32_t g_shapeScaleY;
	extern int32_t g_shapeScaleZ;
	extern uint32_t g_faceIdCount;
	extern int32_t g_shapeHasObject;
	extern uint32_t* g_faceIds;
	extern int32_t g_shapeHasKey;

	void SetFaceIds(uint32_t* p_ids, uint32_t p_count);
	void SetShapeOffset(int32_t p_x, int32_t p_y, int32_t p_z);
	void SetShapeScale(int32_t p_x, int32_t p_y, int32_t p_z);
	void SetShapeFlags(uint32_t p_flags);
	Shape* LoadShapes(uint8_t* p_data, int32_t* p_offset, int32_t p_size, SceneObject* p_parent);
	int32_t LoadShapeRecord(
		uint8_t* p_data,
		int32_t* p_offset,
		Shape** p_shape,
		SceneObject* p_parent,
		int32_t* p_count
	);
	uint32_t MapFaceId(uint32_t p_id);

#ifdef __cplusplus
}
#endif

#endif // WTBSHAPES_H
