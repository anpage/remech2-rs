#ifndef CLASSTABLE_H
#define CLASSTABLE_H

#include "classentry.h"
#include "decomp.h"
#include "shape.h"
#include "types.h"

struct SceneObject;
struct Player;

// The functions and globals of classtable.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_classEntryCount;
	extern MechS32 g_classTableReady;
	extern ClassEntry g_classTable[0x30c];

	MechS32 LoadBaseLevelShapes(struct Player* p_player);
	MechS32 AddClassEntryLevel(
		MechS32 p_unk0x00,
		undefined4 p_unk0x04,
		undefined4 p_unk0x08,
		undefined4 p_unk0x0c,
		MechS32 p_unk0x10,
		MechS32 p_level,
		MechS32 p_index,
		MechS16 p_unk0x1c
	);
	void ResetClassTable(void);
	void ClaimNewClassEntries(struct Player* p_player);
	MechS32 LoadClassLevel(MechS32 p_owner, MechS32 p_level);
	void ReleaseClassLevel(MechS32 p_owner, MechS32 p_level);
	MechS32 LoadClassEntryShape(MechS32 p_index, MechS32 p_level, void* p_buffer);
	void ReleasePendingDetailLevel(void);
	void ReleaseClassEntryShape(MechS32 p_index, MechS32 p_level);
	struct SceneObject* GetClassObject(MechS32 p_index);
	Shape* GetClassShape(MechS32 p_index);
	void SetClassEntryPartId(MechS32 p_index, MechU16 p_value);
	void ChoosePlayerDetailLevels(void);
	void ReleaseObjShape(struct SceneObject* p_obj);
	void ForgetObjShape(struct SceneObject* p_obj);

#ifdef __cplusplus
}
#endif

#endif // CLASSTABLE_H
