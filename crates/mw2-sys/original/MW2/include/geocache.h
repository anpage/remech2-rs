#ifndef GEOCACHE_H
#define GEOCACHE_H

#include "callbacks.h"
#include "decomp.h"
#include "quadtree.h"
#include "shape.h"
#include "staticblock.h"
#include "staticobject.h"
#include "types.h"
#include "xform.h"

struct GameThing;

struct SceneObject;
struct BwdBlockRecord;
struct BwdStream;

// An entry of the class table: an ID and its class.
// SIZE 0x08
typedef struct GeoClass {
	MechS32 m_id;          // 0x00
	struct Shape* m_class; // 0x04
} GeoClass;

// The functions and globals of geocache.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32* g_thingIndices;
	extern MechS32* g_thingIds;
	extern MechS32 g_inRepeat;
	extern MechS32 g_repeatPass;
	extern MechS32 g_blockDepth;
	extern MechS32 g_currentBlock;
	extern MechS32 g_thingCapacity;
	extern MechS32 g_thingCount;
	extern MechS32* g_starIndices;
	extern MechS32* g_starIds;
	extern MechS32 g_classCount;
	extern MechS32 g_classCapacity;
	extern GeoClass* g_classes;
	extern MechS32 g_staticObjectCount;
	extern undefined4 g_staticCacheReady;
	extern MechS32 g_nextBlock;
	extern Xform g_pendingXform;
	extern Xform g_defaultXform;
	extern MechS32 g_explosionChunks;
	extern StaticBlock g_staticBlocks[32];
	extern StaticObject g_staticObjects[0x402];
	extern MechS32 g_starCapacity;
	extern MechS32 g_blockStack[32];
	extern MechS32 g_staticObjectsChanged;
	extern Shape* g_blockBoxes[32];
	extern MechS32 g_blockBoxesShown;
	extern MechS32 g_starCount;

	MechS32 AllocGeoTables(void);
	MechS32 AddClass(MechS32 p_id, Shape* p_class);
	void FreeGeoTables(void);
	Shape** GetStaticShapeSlot(MechS32 p_index);
	Shape* GetStaticShape(MechS32 p_index);
	MechS32 PlaceStaticObject(
		MechS32 p_id,
		MechS32 p_resource,
		Xform p_xform,
		MechS32 p_block,
		MechS32 p_parent,
		MechS32 p_unk0x3c,
		MechU32 p_flags,
		MechU32 p_kind,
		undefined4 p_shapeKind
	);
	void UpdateGeoCache(void);
	void BeginBlock(struct BwdBlockRecord* p_record);
	MechS32 AllocStaticObject(undefined4 p_id);
	void HandleElseBlock(void);
	void EndBlock(struct BwdStream* p_stream);
	MechS32 FindStarIdxById(MechS32 p_id);
	struct Shape* FindClassById(MechS32 p_id);
	MechS32 FindThingIdxById(MechS32 p_id);
	void ApplyBlockXform(Xform p_xform);
	void TransformBlockPoint(MechS32* p_point);
	void ResetStaticObject(MechS32 p_index);
	void ResetStaticCache(void);
	void FirstStaticCache(void);
	void AttachTaskToObj(MechS32 p_index, TimedCallbackFn p_fn, MechS32 p_period, MechChar* p_data);
	void RunStaticObjectTasks(void);
	void RemoveStaticObjectTask(MechS32 p_index, TimedCallback* p_callback);
	void RemoveStaticObjectTasks(MechS32 p_index);
	void SignalStaticObjectTasks(MechS32 p_index);
	void SetStaticObjectKind(MechS32 p_index, MechU32 p_kind);
	void LinkStaticObjectThing(MechS32 p_index, MechS32 p_replacement, MechS32 p_thing);
	MechS32 DestroyStaticObject(MechS32 p_index);
	void DestroyThingObject(struct GameThing* p_thing);
	void PropagateStaticObjectStates(void);
	MechS32 AreStaticObjectsComplete(void);
	MechS32 LoadStaticObject(MechS32 p_index, MechS32 p_block);
	void UnloadStaticObject(MechS32 p_index);
	struct SceneObject* GetStaticSceneObject(MechS32 p_index);
	Shape* GetStaticObjectShape(MechS32 p_index);
	void GetStaticObjectPosition(MechS32 p_index, MechS32* p_x, MechS32* p_y, MechS32* p_z);
	MechS32 ToggleBlockBoxes(void);
	void ShowQuadtreeBoxes(QuadtreeNode* p_root);
	void LoadQuadtreeBoxes(QuadtreeNode* p_node, MechU8* p_data, MechS32 p_size);
	MechS32 FreeStaticObjectTree(MechU32 p_index);
	void DestroyObjTreeAndShapes(struct SceneObject* p_obj);
	MechS32 GetExplosionChunks(MechS32 p_arg);
	void SetExplosionChunks(MechS32 p_arg, MechS32 p_explosionChunks);

#ifdef __cplusplus
}
#endif

#endif // GEOCACHE_H
