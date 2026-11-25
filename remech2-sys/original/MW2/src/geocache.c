#include "geocache.h"

#include "audio.h"
#include "bwdblockrecord.h"
#include "callbacks.h"
#include "classtable.h"
#include "collision.h"
#include "debris.h"
#include "error.h"
#include "files.h"
#include "gamething.h"
#include "loadres.h"
#include "log.h"
#include "mw2prj.h"
#include "object.h"
#include "players.h"
#include "poolsizes.h"
#include "prjfile.h"
#include "quadtree.h"
#include "resourcename.h"
#include "resourceref.h"
#include "shape.h"
#include "shapegeom.h"
#include "shapelists.h"
#include "simmain.h"
#include "soundconfig.h"
#include "staticblock.h"
#include "staticobject.h"
#include "transform.h"
#include "types.h"
#include "wtbshapes.h"
#include "xform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Two maps from world stream IDs to indices: the game things' (FindThingIdxById, filled by
// CreateObjectNode), and the stars', the static objects' (FindStarIdxById, AllocStaticObject).
// Their sizes come from the static memory table (AllocGeoTables).
int32_t* g_thingIndices = NULL;

int32_t* g_thingIds = NULL;

// Set inside a world stream's repeated section (REPR to ENDR); g_repeatPass counts its passes.
int32_t g_inRepeat = 0;

int32_t g_repeatPass = 0;

int32_t* g_starIndices = NULL;

int32_t* g_starIds = NULL;

int32_t g_classCount = 0;

int32_t g_classCapacity = 0;

GeoClass* g_classes = NULL;

int32_t g_staticObjectCount = 0;

uint32_t g_staticCacheReady = 0;

// The block BeginBlock opens next.
int32_t g_nextBlock = 0;

// The block BeginBlock opened last, -1: none.
int32_t g_currentBlock = -1;

int32_t g_blockDepth = 0;

// The transform the next block opens with (ApplyBlockXform).
Xform g_pendingXform = {1, 1, 1, 0, 0, 0, 0, 0, 0};

// The transform BeginBlock resets g_pendingXform to.
Xform g_defaultXform = {1, 1, 1, 0, 0, 0, 0, 0, 0};

int32_t g_explosionChunks = 1;

StaticBlock g_staticBlocks[32];

StaticObject g_staticObjects[0x402];

int32_t g_starCapacity;

int32_t g_thingCount;

// The blocks BeginBlock opened.
int32_t g_blockStack[32];

// Set when PropagateStaticObjectStates changed an entry; UpdateGeoCache propagates again.
int32_t g_staticObjectsChanged;

// The shapes ToggleBlockBoxes shows for the blocks' boxes.
Shape* g_blockBoxes[32];

int32_t g_thingCapacity;

// Whether ToggleBlockBoxes shows the blocks' boxes.
int32_t g_blockBoxesShown;

int32_t g_starCount;

// Allocates the class, thing and star tables to the sizes in the mission's static memory table.
// Returns whether it could.
int32_t AllocGeoTables(void)
{
	uint32_t size;
	int32_t result;

	result = 0;
	FreeGeoTables();
	size = GetStaticPoolSize(7);
	if (size) {
		g_classCapacity = size / sizeof(GeoClass);
		g_classes = MemAlloc(size);
		if (g_classes) {
			size = GetStaticPoolSize(8);
			if (size) {
				g_thingCapacity = size >> 2;
				g_starCapacity = g_thingCapacity;
				g_starIndices = MemAlloc(size);
				if (g_starIndices) {
					g_thingIndices = MemAlloc(size);
					if (g_thingIndices) {
						size = GetStaticPoolSize(9);
						if (size) {
							g_starIds = MemAlloc(size);
							if (g_starIds) {
								g_thingIds = MemAlloc(size);
								if (g_thingIds) {
									result = 1;
								}
							}
						}
					}
				}
			}
		}
	}

	return result;
}

// Adds a class to the class table. Returns whether there was room.
int32_t AddClass(int32_t p_id, Shape* p_class)
{
	int32_t result;

	result = 0;
	if (g_classCount < g_classCapacity) {
		g_classes[g_classCount].m_id = p_id;
		g_classes[g_classCount].m_class = p_class;
		g_classCount++;
		result = 1;
	}

	return result;
}

// Returns the class of an ID (the last one added), or NULL.
Shape* FindClassById(int32_t p_id)
{
	Shape* result;
	int32_t i;

	result = NULL;
	for (i = g_classCount - 1; i >= 0; i--) {
		if (g_classes[i].m_id == p_id) {
			result = g_classes[i].m_class;
			break;
		}
	}

	return result;
}

// Frees the class, thing and star tables.
void FreeGeoTables(void)
{
	if (g_classes) {
		MechHeapFree(g_primaryHeap, g_classes);
		g_classes = NULL;
	}

	if (g_starIndices) {
		MechHeapFree(g_primaryHeap, g_starIndices);
		g_starIndices = NULL;
	}

	if (g_thingIndices) {
		MechHeapFree(g_primaryHeap, g_thingIndices);
		g_thingIndices = NULL;
	}

	if (g_starIds) {
		MechHeapFree(g_primaryHeap, g_starIds);
		g_starIds = NULL;
	}

	if (g_thingIds) {
		MechHeapFree(g_primaryHeap, g_thingIds);
		g_thingIds = NULL;
	}

	g_classCount = 0;
	g_classCapacity = 0;
	g_starCapacity = g_thingCapacity = 0;
	g_starCount = g_thingCount = 0;
}

// Returns the thing index of an ID (the last one listed), or -1.
int32_t FindThingIdxById(int32_t p_id)
{
	int32_t i;
	int32_t result;

	result = -1;
	for (i = 0; i < g_thingCount; i++) {
		if (g_thingIds[i] == p_id) {
			result = g_thingIndices[i];
		}
	}

	return result;
}

void UpdateGeoCache(void)
{
	if (g_staticObjectsChanged) {
		PropagateStaticObjectStates();
		g_staticObjectsChanged = 0;
	}

	ChoosePlayerDetailLevels();
	RunStaticObjectTasks();
}

// Returns the star index of an ID (the last one listed), or -1.
int32_t FindStarIdxById(int32_t p_id)
{
	int32_t i;
	int32_t result;

	result = -1;
	if (p_id >= 0) {
		for (i = 0; i < g_starCount; i++) {
			if (g_starIds[i] == p_id) {
				result = g_starIndices[i];
			}
		}
	}

	return result;
}

// Takes the next free static object for ID p_id, mapping the ID to it. Returns its index, or -1
// when the cache or the map is full.
int32_t AllocStaticObject(uint32_t p_id)
{
	int32_t result = -1;

	if (!g_staticCacheReady) {
		ResetStaticCache();
	}

	if (g_staticObjectCount < 0x402 && g_starCapacity > g_starCount) {
		result = g_staticObjectCount;
		g_starIndices[g_starCount] = result;
		g_starIds[g_starCount] = p_id;
		g_staticObjectCount++;
		g_starCount++;
	}

	return result;
}

Shape** GetStaticShapeSlot(int32_t p_index)
{
	return &g_staticObjects[p_index].m_shape;
}

Shape* GetStaticShape(int32_t p_index)
{
	return g_staticObjects[p_index].m_shape;
}

// Places static object p_id: its resource, transform, block and parent, and the object it hangs
// from (none for a shape-only entry, -2). Returns its index, or -1 if p_id isn't cached.
int32_t PlaceStaticObject(
	int32_t p_id,
	int32_t p_resource,
	Xform p_xform,
	int32_t p_block,
	int32_t p_parent,
	int32_t p_unk0x3c,
	uint32_t p_flags,
	uint32_t p_kind,
	uint32_t p_shapeKind
)
{
	int32_t index;
	StaticObject* entry;
	Matrix matrix;

	index = FindStarIdxById(p_id);
	if (index == -1) {
		index = AllocStaticObject(p_id);
	}

	if (index != -1) {
		entry = &g_staticObjects[index];
		BuildMatrix(
			&matrix,
			p_xform.m_angleX,
			p_xform.m_angleY,
			p_xform.m_angleZ,
			p_xform.m_x,
			p_xform.m_y,
			p_xform.m_z
		);
		CopyMatrix(&matrix, &entry->m_matrix);
		entry->m_xform = p_xform;
		entry->m_resource = p_resource;
		entry->m_parent = p_parent;
		entry->m_block = p_block;
		entry->m_shape = NULL;
		if (p_parent == -2) {
			entry->m_object = NULL;
		}
		else if (p_parent == -1) {
			entry->m_object = CreateObj(NULL, 10);
		}
		else {
			entry->m_object = CreateObj(g_staticObjects[p_parent].m_object, 10);
		}

		entry->m_flags |= p_flags & 0x1ff;
		entry->m_flags |= (p_kind << 12) & 0xf000;
		entry->m_shapeKind = p_shapeKind;
		PropagateStaticObjectStates();
	}

	return index;
}

// Opens the next block (g_nextBlock) inside the current one: its box, from a corner and a
// size, its center, and the matrix that places its objects, from the pending transform
// (g_pendingXform, reset afterwards) and every enclosing block's matrix. The outermost block
// also clears the placed-object list.
void BeginBlock(BwdBlockRecord* p_record)
{
	StaticBlock* block;
	Matrix* matrix;
	Matrix scale;
	int32_t i;
	StaticBlock* parent;
	Matrix translate;
	Matrix rotation;

	if (g_blockDepth == 0) {
		g_starCount = 0;
		for (i = 0; i < g_starCapacity; i++) {
			g_starIndices[i] = -1;
		}

		g_blockStack[g_blockDepth] = -1;
	}

	if (g_nextBlock < 32 && g_nextBlock >= 0) {
		g_blockDepth++;
		g_blockStack[g_blockDepth] = g_nextBlock;
		block = &g_staticBlocks[g_nextBlock];
		block->m_minX = p_record->m_origin[0];
		block->m_minY = p_record->m_origin[1];
		block->m_minZ = p_record->m_origin[2];
		block->m_maxX = p_record->m_size[0];
		block->m_maxY = p_record->m_size[1];
		block->m_maxZ = p_record->m_size[2];
		block->m_parent = g_blockStack[g_blockDepth - 1];
		block->m_xform = g_pendingXform;
		if (block->m_maxX < 0) {
			block->m_minX += block->m_maxX;
			block->m_maxX = -block->m_maxX;
		}

		block->m_maxX += block->m_minX;
		if (block->m_maxY < 0) {
			block->m_minY += block->m_maxY;
			block->m_maxY = -block->m_maxY;
		}

		block->m_maxY += block->m_minY;
		if (block->m_maxZ < 0) {
			block->m_minZ += block->m_maxZ;
			block->m_maxZ = -block->m_maxZ;
		}

		block->m_maxZ += block->m_minZ;
		block->m_centerX = (block->m_maxX + block->m_minX) / 2;
		block->m_centerY = (block->m_maxY + block->m_minY) / 2;
		block->m_centerZ = (block->m_maxZ + block->m_minZ) / 2;
		matrix = &block->m_matrix;
		SetIdentityMatrix(&rotation);
		rotation.m_rows[0][0] = block->m_xform.m_scaleX << 29;
		rotation.m_rows[1][1] = block->m_xform.m_scaleY << 29;
		rotation.m_rows[2][2] = block->m_xform.m_scaleZ << 29;
		BuildMatrix(&translate, 0, 0, 0, -block->m_centerX, -block->m_centerY, -block->m_centerZ);
		BuildMatrix(&scale, block->m_xform.m_angleX, block->m_xform.m_angleY, block->m_xform.m_angleZ, 0, 0, 0);
		MultiplyMatrix(&scale, &translate, matrix);
		BuildMatrix(&translate, 0, 0, 0, block->m_xform.m_x, block->m_xform.m_y, block->m_xform.m_z);
		MultiplyMatrix(&translate, matrix, matrix);
		BuildMatrix(&translate, 0, 0, 0, block->m_centerX, block->m_centerY, block->m_centerZ);
		MultiplyMatrix(&translate, matrix, matrix);
		parent = block;
		while (parent->m_parent != -1) {
			parent = &g_staticBlocks[parent->m_parent];
			MultiplyMatrix(&parent->m_matrix, matrix, matrix);
		}

		g_pendingXform = g_defaultXform;
		g_currentBlock = g_nextBlock;
		g_nextBlock++;
	}
}

void HandleElseBlock(void)
{
}

// Closes the block BeginBlock opened last.
void EndBlock(struct BwdStream* p_stream)
{
	g_blockDepth--;
	if (g_blockDepth <= -1) {
		Error(0x22, NULL);
	}
	else {
		g_currentBlock = g_blockStack[g_blockDepth];
	}
}

// Sets the transform the next block opens with.
void ApplyBlockXform(Xform p_xform)
{
	g_pendingXform = p_xform;
}

// Transforms a point by the current block's matrix.
void TransformBlockPoint(int32_t* p_point)
{
	if (g_currentBlock != -1) {
		TransformPoint(&g_staticBlocks[g_currentBlock].m_matrix, p_point, p_point + 1, p_point + 2);
	}
}

// Resets a static object cache entry.
void ResetStaticObject(int32_t p_index)
{
	StaticObject* entry;

	entry = &g_staticObjects[p_index];
	entry->m_resource = -1;
	entry->m_block = -1;
	entry->m_parent = -1;
	entry->m_flags = 0;
	entry->m_shapeKind = 0;
	entry->m_replacement = -1;
	entry->m_thing = -1;
	entry->m_shape = NULL;
	entry->m_object = NULL;
	entry->m_xform.m_scaleX = entry->m_xform.m_scaleY = entry->m_xform.m_scaleZ = 1;
	entry->m_xform.m_angleX = entry->m_xform.m_angleY = entry->m_xform.m_angleZ = 0;
	entry->m_xform.m_x = entry->m_xform.m_y = entry->m_xform.m_z = 0;
	entry->m_callback = NULL;
}

void ResetStaticCache(void)
{
	int32_t i;

	for (i = 0; i < 0x402; i++) {
		ResetStaticObject(i);
	}

	g_staticObjectCount = 0;
	g_staticCacheReady = 1;
}

void FirstStaticCache(void)
{
	int32_t i;
	StaticObject* entry;

	for (i = 0; i < g_staticObjectCount; i++) {
		entry = &g_staticObjects[i];
		LoadStaticObject(i, entry->m_block);
	}
}

void AttachTaskToObj(int32_t p_index, TimedCallbackFn p_fn, int32_t p_period, char* p_data)
{
	StaticObject* entry;

	entry = &g_staticObjects[p_index];
	CreateDetachedTask(&entry->m_callback, p_fn, p_period, p_data);
}

// Runs the timed callbacks of the cache entries.
void RunStaticObjectTasks(void)
{
	int32_t i;
	StaticObject* entry;

	for (i = 0; i < g_staticObjectCount; i++) {
		entry = &g_staticObjects[i];
		if (!(entry->m_flags & 0x800) && entry->m_callback) {
			RunTimedCallbacks(&entry->m_callback);
		}
	}
}

// The static object's timed callbacks: removes one, removes them all, or signals them all (-1)
// when the object is loaded again.
void RemoveStaticObjectTask(int32_t p_index, TimedCallback* p_callback)
{
	StaticObject* entry;

	entry = &g_staticObjects[p_index];
	RemoveTask(&entry->m_callback, p_callback);
}

void RemoveStaticObjectTasks(int32_t p_index)
{
	StaticObject* entry;

	entry = &g_staticObjects[p_index];
	RemoveAllTasks(&entry->m_callback);
}

void SignalStaticObjectTasks(int32_t p_index)
{
	StaticObject* entry;

	entry = &g_staticObjects[p_index];
	SignalAllTasks(&entry->m_callback);
}

void SetStaticObjectKind(int32_t p_index, uint32_t p_kind)
{
	StaticObject* entry;

	entry = &g_staticObjects[p_index];
	entry->m_flags &= ~0xf000;
	entry->m_flags |= (p_kind << 12) & 0xf000;
}

// Links cache entry p_index to its game thing and to the entry that replaces it when the thing is
// destroyed (DestroyStaticObject), which stays unloaded (0x800) until then.
void LinkStaticObjectThing(int32_t p_index, int32_t p_replacement, int32_t p_thing)
{
	StaticObject* replacement;
	StaticObject* entry;

	if (p_index < 0 || p_index >= g_staticObjectCount) {
		return;
	}

	entry = &g_staticObjects[p_index];
	entry->m_flags |= 0x400;
	entry->m_shapeKind |= 0x200;
	entry->m_replacement = (int16_t) p_replacement;
	entry->m_thing = (int16_t) p_thing;
	if (p_replacement > -1) {
		replacement = &g_staticObjects[p_replacement];
		replacement->m_flags |= 0x800;
		PropagateStaticObjectStates();
	}
}

// Destroys the game thing of cache entry p_index: marks both (0x200 and 0x800, 4), frees its shape
// and loads the entry that replaces it (m_replacement) where the old object was. Returns the
// replacement's index, or -1.
int32_t DestroyStaticObject(int32_t p_index)
{
	Matrix matrix;
	int32_t replacement;
	struct SceneObject* obj;
	StaticObject* entry;
	Shape* shape;
	GameThing* thing;

	obj = NULL;
	entry = &g_staticObjects[p_index];
	if (entry->m_thing == -1) {
		return -1;
	}

	thing = &g_gameThings[entry->m_thing];
	entry->m_flags |= 0x200;
	entry->m_flags |= 0x800;
	thing->m_flags |= 4;
	thing->m_staticObject = -1;
	RemoveStaticObjectTasks(p_index);
	shape = entry->m_shape;
	if (shape) {
		obj = entry->m_object;
		if (obj) {
			matrix = *GetObjWorldMatrix(obj);
		}

		DestroyObjShape(shape);
		entry->m_shape = NULL;
		entry->m_object = NULL;
	}

	replacement = entry->m_replacement;
	if (replacement != -1) {
		entry = &g_staticObjects[replacement];
		entry->m_flags &= ~0x800;
		if (LoadStaticObject(replacement, entry->m_block)) {
			obj = entry->m_object;
			if (obj) {
				SetObjLocalMatrix(obj, &matrix);
			}

			SignalStaticObjectTasks(replacement);
			if (obj) {
				UpdateObjWorld(obj);
			}
		}
		else {
			entry->m_flags &= 0x800;
			replacement = -1;
		}
	}

	return replacement;
}

// Marks p_thing's cache entry (flag 0x200) and has DestroyStaticObject replace it. When the replacement's
// game thing has flag 0x8000, its object tree is handed to BlowOffObjTree and FreeStaticObjectTree.
void DestroyThingObject(GameThing* p_thing)
{
	int32_t index;
	int32_t current;
	GameThing* thing;
	StaticObject* entry;

	index = -1;
	entry = &g_staticObjects[p_thing->m_staticObject];
	entry->m_flags |= 0x200;
	current = p_thing->m_staticObject;
	if (current != -1) {
		index = DestroyStaticObject(current);
	}

	if (index != -1) {
		thing = &g_gameThings[g_staticObjects[index].m_thing];
		if (thing->m_flags & 0x8000) {
			BlowOffObjTree(g_staticObjects[index].m_object, DestroyObjTreeAndShapes, 0);
			FreeStaticObjectTree(index);
		}
	}

	PropagateStaticObjectStates();
}

// Propagates the parents' states to the cache entries hanging from them: a hidden parent (0x200)
// hides the entry (DestroyStaticObject); a parent flagged 0x800 flags the entry and unloads it
// (UnloadStaticObject), and an entry whose parent no longer is is loaded again (LoadStaticObject), its game
// thing's object marked when the thing has bit 0x8000.
void PropagateStaticObjectStates(void)
{
	GameThing* thing;
	int32_t i;
	int32_t index;
	StaticObject* entry;

	for (i = 0; i < g_staticObjectCount; i++) {
		entry = &g_staticObjects[i];
		if (entry->m_parent > -1) {
			if (g_staticObjects[entry->m_parent].m_flags & 0x200 && !(entry->m_flags & 0x200)) {
				g_staticObjectsChanged = 1;
				DestroyStaticObject(i);
			}

			if (g_staticObjects[entry->m_parent].m_flags & 0x800) {
				if (!(entry->m_flags & 0x800)) {
					g_staticObjectsChanged = 1;
					entry->m_flags |= 0x800;
					UnloadStaticObject(i);
				}
			}
			else if (entry->m_flags & 0x800 && !(entry->m_flags & 0x200)) {
				g_staticObjectsChanged = 1;
				entry->m_flags &= ~0x800;
				if (LoadStaticObject(i, entry->m_block)) {
					index = g_staticObjects[i].m_thing;
					if (index != -1) {
						thing = &g_gameThings[index];
						if (thing->m_flags & 0x8000) {
							BlowOffObjTree(g_staticObjects[i].m_object, DestroyObjTreeAndShapes, 0);
							FreeStaticObjectTree(i);
						}
					}
				}
				else {
					entry->m_flags &= 0x800;
				}
			}
		}
	}
}

// Returns whether every static object in use has a resource (the world loader's check).
int32_t AreStaticObjectsComplete(void)
{
	int32_t i;
	int32_t found;

	found = false;
	for (i = 0; i < g_staticObjectCount && !found; i++) {
		if (g_staticObjects[i].m_resource == -1) {
			found = true;
		}
	}

	if (found) {
		return false;
	}
	else {
		return true;
	}
}

// Loads cache entry p_index's shape and places it: a shape-only entry (-2) in block p_block's
// frame, otherwise on its object. Returns true while the shape is loaded, false if it can't be or
// shouldn't be: a hidden entry (0x800), or a destroyed game thing's (without explosion chunks).
int32_t LoadStaticObject(int32_t p_index, int32_t p_block)
{
	int32_t size;
	int32_t offset;
	int32_t thing;
	uint8_t* data;
	struct SceneObject* parent;
	uint32_t kind;
	StaticObject* entry;
	Matrix matrix;

	size = 0;
	offset = 0;
	entry = &g_staticObjects[p_index];
	if (entry->m_flags & 0x800) {
		return false;
	}

	if (entry->m_resource == -1) {
		return false;
	}

	thing = entry->m_thing;
	if (thing != -1 && (g_gameThings[thing].m_flags & 0x8000) && (!g_explosionChunks || IsDebrisFull())) {
		return false;
	}

	if (entry->m_shape && !(GetShapeState(entry->m_shape) & 0x400)) {
		return true;
	}

	data = LoadCachedResource(g_mw2PrjHandle, entry->m_resource, g_resourceTypeTags[c_resTagPoly], 0);
	if (data) {
		size = GetPrjResourceSize(g_mw2PrjHandle, g_resourceTypeTags[c_resTagPoly], entry->m_resource);
	}
	else {
		return false;
	}

	SetShapeScale(entry->m_xform.m_scaleX, entry->m_xform.m_scaleY, entry->m_xform.m_scaleZ);
	SetShapeFlags(entry->m_flags & 0x1ff);
	parent = NULL;
	if (entry->m_parent > -1) {
		parent = g_staticObjects[entry->m_parent].m_object;
	}

	if (entry->m_thing != -1) {
		g_shapeOwnerSet = 1;
		g_shapeOwnerKind = 0x200;
		g_shapeOwner = entry->m_thing;
	}

	entry->m_shape = LoadShapes(data, &offset, size, parent);
	g_shapeOwnerSet = 0;
	UnlockCachedResource(entry->m_resource, g_resourceTypeTags[c_resTagPoly]);
	if (!entry->m_shape) {
		return false;
	}

	kind = (entry->m_flags & 0xf000) >> 12;
	if (entry->m_parent == -2) {
		MultiplyMatrix(&g_staticBlocks[p_block].m_matrix, &entry->m_matrix, &matrix);
		TransformShape(entry->m_shape, &matrix);
		entry->m_object = NULL;
		if (entry->m_flags & 0x400) {
			SetShapeKind(entry->m_shape, entry->m_shapeKind);
			SetShapeOwner(entry->m_shape, entry->m_thing);
		}
		else {
			SetShapeKind(entry->m_shape, entry->m_shapeKind);
			SetShapeOwner(entry->m_shape, p_index);
		}

		SetShapeCollisionType(entry->m_shape, kind);
		AddSceneShape(entry->m_shape);
	}
	else if (entry->m_object) {
		SetObjShape(entry->m_object, entry->m_shape);
		SetShapeObject(entry->m_shape, entry->m_object);
		AddSceneShape(entry->m_shape);
		if (entry->m_parent == -1) {
			MultiplyMatrix(&g_staticBlocks[p_block].m_matrix, &entry->m_matrix, &matrix);
			SetObjTransform(entry->m_object, &matrix);
			UpdateObj(entry->m_object);
		}
		else {
			SetObjRotation(
				entry->m_object,
				entry->m_xform.m_angleX,
				entry->m_xform.m_angleY,
				entry->m_xform.m_angleZ,
				0
			);
			SetObjPosition(entry->m_object, entry->m_xform.m_x, entry->m_xform.m_y, entry->m_xform.m_z);
			UpdateObj(entry->m_object);
		}

		if (entry->m_flags & 0x400) {
			SetObjTreeKind(entry->m_object, entry->m_shapeKind);
			SetObjTreeOwner(entry->m_object, entry->m_thing);
		}
		else {
			SetShapeKind(entry->m_shape, entry->m_shapeKind);
			SetShapeOwner(entry->m_shape, p_index);
		}

		SetObjTreeCollisionType(entry->m_object, kind);
	}
	else {
		return false;
	}

	if (kind == 5) {
		BuildShapeQuadtree(entry->m_shape);
	}

	return true;
}

// Frees a cache entry's shape.
void UnloadStaticObject(int32_t p_index)
{
	StaticObject* entry;

	entry = &g_staticObjects[p_index];
	if (entry->m_shape) {
		DestroyObjShape(entry->m_shape);
		entry->m_shape = NULL;
	}
}

struct SceneObject* GetStaticSceneObject(int32_t p_index)
{
	struct SceneObject* result;

	result = NULL;
	if (p_index < g_staticObjectCount && p_index >= 0) {
		result = g_staticObjects[p_index].m_object;
	}

	return result;
}

Shape* GetStaticObjectShape(int32_t p_index)
{
	Shape* result;

	result = NULL;
	if (p_index < g_staticObjectCount && p_index >= 0) {
		result = g_staticObjects[p_index].m_shape;
	}

	return result;
}

// Returns a cache entry's position: its shape's, or its object's. A bad index (the test lets
// g_staticObjectCount itself through) or an empty entry gives 0, 0, 0.
void GetStaticObjectPosition(int32_t p_index, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	struct SceneObject* obj;
	StaticObject* entry;
	Shape* shape;

	if (p_index > g_staticObjectCount || p_index < 0) {
		*p_x = *p_y = *p_z = 0;
	}
	else {
		entry = &g_staticObjects[p_index];
		shape = entry->m_shape;
		if (shape) {
			*p_x = shape->m_centerX;
			*p_y = shape->m_centerY;
			*p_z = shape->m_centerZ;
		}
		else {
			obj = entry->m_object;
			if (obj) {
				GetObjPosition(obj, p_x, p_y, p_z);
			}
			else {
				*p_x = *p_y = *p_z = 0;
			}
		}
	}
}

// Shows the static blocks' boxes (the "unitbox" model scaled to each), or frees them when they are
// shown. Returns whether any is shown.
int32_t ToggleBlockBoxes(void)
{
	int32_t result;
	int32_t size;
	ResourceRef* ref;
	int32_t dz;
	int32_t dy;
	int32_t dx;
	int32_t scaleZ;
	int32_t scaleY;
	int32_t scaleX;
	int32_t offset;
	Shape* shape;
	StaticBlock* block;
	int32_t i;
	ResourceRef local;
	Matrix matrix;
	uint8_t* data;

	result = false;
	if (!g_blockBoxesShown) {
		g_blockBoxesShown = true;
		ref = &local;
		ref->m_id = -1;
		strncpy(ref->m_name, "unitbox", 12);
		ref->m_name[12] = '\0';
		data = LoadResourceByRef(
			ref,
			g_resourceTypeTags[c_resTagPoly],
			g_resourceTypeExtensions[c_resExtWtb],
			1,
			&size,
			NULL
		);
		if (data) {
			for (i = 0; i < g_nextBlock; i++) {
				block = &g_staticBlocks[i];
				if (block->m_maxX == block->m_minX) {
					continue;
				}

				dx = block->m_maxX - block->m_minX;
				dy = block->m_maxY - block->m_minY;
				dz = block->m_maxZ - block->m_minZ;
				dx = abs(dx);
				dy = abs(dy);
				dz = abs(dz);
				scaleX = (dx + 9) / 10;
				scaleY = (dy + 9) / 10;
				scaleZ = (dz + 9) / 10;
				SetShapeScale(scaleX, scaleY, scaleZ);
				SetShapeFlags(0);
				offset = 0;
				g_blockBoxes[i] = LoadShapes(data, &offset, size, NULL);
				shape = g_blockBoxes[i];
				if (shape) {
					result = true;
					SetShapeCollisionType(shape, 4);
					BuildMatrix(&matrix, 0, 0, 0, block->m_centerX, block->m_centerY, block->m_centerZ);
					MultiplyMatrix(&block->m_matrix, &matrix, &matrix);
					TransformShape(shape, &matrix);
					AddSceneShape(shape);
				}
			}

			if (ref->m_id == -1) {
				MechHeapFree(g_primaryHeap, data);
			}
			else {
				UnlockCachedResource(ref->m_id, g_resourceTypeTags[c_resTagPoly]);
			}
		}
		else {
			MechLogErrorf("symlog.txt: Couldn't load ID=%s Type=%s\n", ref->m_name, g_resourceTypeTags[c_resTagPoly]);
		}
	}
	else {
		g_blockBoxesShown = false;
		result = true;
		for (i = 0; i < g_nextBlock; i++) {
			if (g_blockBoxes[i]) {
				DestroyObjShape(g_blockBoxes[i]);
				g_blockBoxes[i] = NULL;
			}
		}
	}

	return result;
}

// Loads the "unitbox" model into every box of the tree p_root.
void ShowQuadtreeBoxes(QuadtreeNode* p_root)
{
	int32_t size;
	ResourceRef* ref;
	ResourceRef local;
	uint8_t* data;

	ref = &local;
	ref->m_id = -1;
	strncpy(ref->m_name, "unitbox", 12);
	ref->m_name[12] = '\0';
	data =
		LoadResourceByRef(ref, g_resourceTypeTags[c_resTagPoly], g_resourceTypeExtensions[c_resExtWtb], 1, &size, NULL);
	if (data) {
		LoadQuadtreeBoxes(p_root, data, size);
		if (ref->m_id == -1) {
			MechHeapFree(g_primaryHeap, data);
		}
		else {
			UnlockCachedResource(ref->m_id, g_resourceTypeTags[c_resTagPoly]);
		}
	}
	else {
		MechLogErrorf("symlog.txt: Couldn't load ID=%s Type=%s\n", ref->m_name, g_resourceTypeTags[c_resTagPoly]);
	}
}

// Loads model p_data scaled to the box of p_node at its center, then to its children's.
void LoadQuadtreeBoxes(QuadtreeNode* p_node, uint8_t* p_data, int32_t p_size)
{
	int32_t dz;
	int32_t dy;
	int32_t dx;
	int32_t scaleZ;
	int32_t scaleY;
	int32_t scaleX;
	int32_t offset;
	int32_t x;
	Shape* shape;
	int32_t z;
	int32_t i;
	int32_t y;
	Matrix matrix;

	if (!p_node) {
		return;
	}

	dx = p_node->m_maxX - p_node->m_minX;
	dy = p_node->m_maxY - p_node->m_minY;
	dz = p_node->m_maxZ - p_node->m_minZ;
	dx = abs(dx);
	dy = abs(dy);
	dz = abs(dz);
	scaleX = (dx + 9) / 10;
	scaleY = (dy * 2 + 18) / 10;
	scaleZ = (dz + 9) / 10;
	SetShapeScale(scaleX, scaleY, scaleZ);
	SetShapeFlags(4);
	offset = 0;
	shape = LoadShapes(p_data, &offset, p_size, NULL);
	if (shape) {
		SetShapeCollisionType(shape, 4);
		x = (p_node->m_maxX + p_node->m_minX) >> 1;
		y = (p_node->m_minY + p_node->m_maxY) >> 1;
		z = (p_node->m_maxZ + p_node->m_minZ) >> 1;
		BuildMatrix(&matrix, 0, 0, 0, x, y, z);
		TransformShape(shape, &matrix);
		AddSceneShape(shape);
	}

	for (i = 0; i < 4; i++) {
		LoadQuadtreeBoxes(p_node->m_children[i], p_data, p_size);
	}
}

// Frees cache entry p_index and, first, every entry hanging from it, zeroing its game thing.
// Returns whether p_index is an entry.
int32_t FreeStaticObjectTree(uint32_t p_index)
{
	int32_t result;
	uint32_t thing;
	int32_t i;
	StaticObject* entry;

	result = false;
	for (i = 0; i < g_staticObjectCount; i++) {
		if (g_staticObjects[i].m_parent == p_index) {
			FreeStaticObjectTree(i);
		}
	}

	if (p_index < 0x402) {
		entry = &g_staticObjects[p_index];
		thing = entry->m_thing;
		if (thing < 0xfe) {
			ZeroGameThing(thing);
		}

		ResetStaticObject(p_index);
		result = true;
	}

	return result;
}

// Frees a scene object tree, and the shapes on it when the object has one.
void DestroyObjTreeAndShapes(struct SceneObject* p_obj)
{
	ShapeCallback callback;
	Shape* shape;

	callback = NULL;
	if (!p_obj) {
		return;
	}

	shape = GetObjShape(p_obj);
	if (shape) {
		callback = DestroyObjShape;
	}

	DestroyObjTree(p_obj, callback);
}

int32_t GetExplosionChunks(int32_t p_arg)
{
	return g_explosionChunks;
}

void SetExplosionChunks(int32_t p_arg, int32_t p_explosionChunks)
{
	g_explosionChunks = p_explosionChunks;
	g_mw2SndCfgData->m_explosionChunks = p_explosionChunks;
}
