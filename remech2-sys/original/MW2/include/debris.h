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

	extern MechS32 g_debrisCount;
	extern DebrisChunk g_emptyDebrisChunk;
	extern DebrisPiece g_debrisPieces[0x80];
	extern DebrisChunk g_debrisChunks[0x80];

	MechS32 IsDebrisFull(void);
	MechS32 AddDebrisPiece(SceneObject* p_obj, MechS32 p_unk0x00);
	void ThrowDebrisPiece(MechS32 p_index);
	void BlowOffChunk(SceneObject* p_obj, ObjectCallback p_callback, MechU32 p_unk0x16);
	void BlowOffObjTree(SceneObject* p_obj, ObjectCallback p_callback, MechU32 p_unk0x16);
	void DisposeDebris(SceneObject* p_obj, ObjectCallback p_callback);
	void UpdateDebris(void);
	void ExplodeChunk(MechS32 p_index);
	void DamageChunk(MechS32 p_index, MechS32 p_damage);
	void UpdateDebrisPiece(MechS32 p_index);
	void PushDebrisPiece(MechS32 p_index, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	void ZeroChunx(void);
	void ResetDebrisPiece(MechS32 p_index);
	MechS32 FindDebrisPiece(SceneObject* p_obj);
	void DamageChunksInRadius(MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_radius, MechS32 p_damage);
	void RemoveChunk(SceneObject* p_obj, ObjectCallback p_callback);
	void HideDebrisObj(SceneObject* p_obj);

#ifdef __cplusplus
}
#endif

#endif // DEBRIS_H
