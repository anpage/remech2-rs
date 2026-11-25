#ifndef GEOCACHE_H
#define GEOCACHE_H

#include "callbacks.h"
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
typedef struct GeoClass {
	int32_t m_id;          // 0x00
	struct Shape* m_class; // 0x04
} GeoClass;

// The functions and globals of geocache.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t* g_thingIndices;
	extern int32_t* g_thingIds;
	extern int32_t g_inRepeat;
	extern int32_t g_repeatPass;
	extern int32_t g_blockDepth;
	extern int32_t g_currentBlock;
	extern int32_t g_thingCapacity;
	extern int32_t g_thingCount;
	extern int32_t* g_starIndices;
	extern int32_t* g_starIds;
	extern int32_t g_classCount;
	extern int32_t g_classCapacity;
	extern GeoClass* g_classes;
	extern int32_t g_staticObjectCount;
	extern uint32_t g_staticCacheReady;
	extern int32_t g_nextBlock;
	extern Xform g_pendingXform;
	extern Xform g_defaultXform;
	extern int32_t g_explosionChunks;
	extern StaticBlock g_staticBlocks[32];
	extern StaticObject g_staticObjects[0x402];
	extern int32_t g_starCapacity;
	extern int32_t g_blockStack[32];
	extern int32_t g_staticObjectsChanged;
	extern Shape* g_blockBoxes[32];
	extern int32_t g_blockBoxesShown;
	extern int32_t g_starCount;

	int32_t AllocGeoTables(void);
	int32_t AddClass(int32_t p_id, Shape* p_class);
	void FreeGeoTables(void);
	Shape** GetStaticShapeSlot(int32_t p_index);
	Shape* GetStaticShape(int32_t p_index);
	int32_t PlaceStaticObject(
		int32_t p_id,
		int32_t p_resource,
		Xform p_xform,
		int32_t p_block,
		int32_t p_parent,
		uint32_t p_flags,
		uint32_t p_kind,
		uint32_t p_shapeKind
	);
	void UpdateGeoCache(void);
	void BeginBlock(struct BwdBlockRecord* p_record);
	int32_t AllocStaticObject(uint32_t p_id);
	void HandleElseBlock(void);
	void EndBlock(struct BwdStream* p_stream);
	int32_t FindStarIdxById(int32_t p_id);
	struct Shape* FindClassById(int32_t p_id);
	int32_t FindThingIdxById(int32_t p_id);
	void ApplyBlockXform(Xform p_xform);
	void TransformBlockPoint(int32_t* p_point);
	void ResetStaticObject(int32_t p_index);
	void ResetStaticCache(void);
	void FirstStaticCache(void);
	void AttachTaskToObj(int32_t p_index, TimedCallbackFn p_fn, int32_t p_period, char* p_data);
	void RunStaticObjectTasks(void);
	void RemoveStaticObjectTask(int32_t p_index, TimedCallback* p_callback);
	void RemoveStaticObjectTasks(int32_t p_index);
	void SignalStaticObjectTasks(int32_t p_index);
	void SetStaticObjectKind(int32_t p_index, uint32_t p_kind);
	void LinkStaticObjectThing(int32_t p_index, int32_t p_replacement, int32_t p_thing);
	int32_t DestroyStaticObject(int32_t p_index);
	void DestroyThingObject(struct GameThing* p_thing);
	void PropagateStaticObjectStates(void);
	int32_t AreStaticObjectsComplete(void);
	int32_t LoadStaticObject(int32_t p_index, int32_t p_block);
	void UnloadStaticObject(int32_t p_index);
	struct SceneObject* GetStaticSceneObject(int32_t p_index);
	Shape* GetStaticObjectShape(int32_t p_index);
	void GetStaticObjectPosition(int32_t p_index, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	int32_t ToggleBlockBoxes(void);
	void ShowQuadtreeBoxes(QuadtreeNode* p_root);
	void LoadQuadtreeBoxes(QuadtreeNode* p_node, uint8_t* p_data, int32_t p_size);
	int32_t FreeStaticObjectTree(uint32_t p_index);
	void DestroyObjTreeAndShapes(struct SceneObject* p_obj);
	int32_t GetExplosionChunks(int32_t p_arg);
	void SetExplosionChunks(int32_t p_arg, int32_t p_explosionChunks);

#ifdef __cplusplus
}
#endif

#endif // GEOCACHE_H
