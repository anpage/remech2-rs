#ifndef DEBRIS_H
#define DEBRIS_H

#include "debrischunk.h"
#include "debrispiece.h"
#include "object.h"
#include "types.h"

// The functions and globals of debris.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_debrisCount;
	extern DebrisChunk g_emptyDebrisChunk;
	extern DebrisPiece g_debrisPieces[0x80];
	extern DebrisChunk g_debrisChunks[0x80];

	int32_t IsDebrisFull(void);
	int32_t AddDebrisPiece(SceneObject* p_obj);
	void ThrowDebrisPiece(int32_t p_index);
	void BlowOffChunk(SceneObject* p_obj, ObjectCallback p_callback, uint32_t p_destroyPartZero);
	void BlowOffObjTree(SceneObject* p_obj, ObjectCallback p_callback, uint32_t p_destroyPartZero);
	void DisposeDebris(SceneObject* p_obj, ObjectCallback p_callback);
	void UpdateDebris(void);
	void ExplodeChunk(int32_t p_index);
	void DamageChunk(int32_t p_index, int32_t p_damage);
	void UpdateDebrisPiece(int32_t p_index);
	void PushDebrisPiece(int32_t p_index, int32_t p_x, int32_t p_y, int32_t p_z);
	void ZeroChunx(void);
	void ResetDebrisPiece(int32_t p_index);
	int32_t FindDebrisPiece(SceneObject* p_obj);
	void DamageChunksInRadius(int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_radius, int32_t p_damage);
	void RemoveChunk(SceneObject* p_obj, ObjectCallback p_callback);
	void HideDebrisObj(SceneObject* p_obj);

#ifdef __cplusplus
}
#endif

#endif // DEBRIS_H
