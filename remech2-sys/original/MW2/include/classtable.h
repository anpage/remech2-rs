#ifndef CLASSTABLE_H
#define CLASSTABLE_H

#include "classentry.h"
#include "shape.h"
#include "types.h"

struct SceneObject;
struct Player;

// The functions and globals of classtable.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_classEntryCount;
	extern int32_t g_classTableReady;
	extern ClassEntry g_classTable[0x30c];

	int32_t LoadBaseLevelShapes(struct Player* p_player);
	int32_t AddClassEntryLevel(
		int32_t p_resourceId,
		uint32_t p_x,
		uint32_t p_y,
		uint32_t p_z,
		int32_t p_parent,
		int32_t p_level,
		int32_t p_index,
		int16_t p_kind
	);
	void ResetClassTable(void);
	void ClaimNewClassEntries(struct Player* p_player);
	int32_t LoadClassLevel(int32_t p_owner, int32_t p_level);
	void ReleaseClassLevel(int32_t p_owner, int32_t p_level);
	int32_t LoadClassEntryShape(int32_t p_index, int32_t p_level, void* p_buffer);
	void ReleasePendingDetailLevel(void);
	void ReleaseClassEntryShape(int32_t p_index, int32_t p_level);
	struct SceneObject* GetClassObject(int32_t p_index);
	Shape* GetClassShape(int32_t p_index);
	void SetClassEntryPartId(int32_t p_index, uint16_t p_value);
	void ChoosePlayerDetailLevels(void);
	void ReleaseObjShape(struct SceneObject* p_obj);
	void ForgetObjShape(struct SceneObject* p_obj);

#ifdef __cplusplus
}
#endif

#endif // CLASSTABLE_H
