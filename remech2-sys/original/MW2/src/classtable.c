#include "classtable.h"

#include "classentry.h"
#include "collision.h"
#include "eyepoint.h"
#include "fixedmul.h"
#include "loadres.h"
#include "mw2prj.h"
#include "object.h"
#include "players.h"
#include "polydraw.h"
#include "poolsizes.h"
#include "prjfile.h"
#include "shape.h"
#include "shapelists.h"
#include "simmain.h"
#include "staticmem.h"
#include "targeting.h"
#include "types.h"
#include "wtbshapes.h"

// A player's model level as ChoosePlayerDetailLevels chooses it.
typedef struct PlayerDetail {
	int32_t m_distance; // 0x00 — from the eyepoint
	int32_t m_level;    // 0x04 — -2 until chosen by distance
	int32_t m_player;   // 0x08
} PlayerDetail;

int32_t g_classEntryCount = 0;

int32_t g_classTableReady = 0;

ClassEntry g_classTable[0x30c];

// Loads the shapes of p_player's entries for its base level (m_baseLevel) into one pool block.
int32_t LoadBaseLevelShapes(Player* p_player)
{
	int32_t i;
	int32_t count;
	uint8_t* buffer;

	count = 0;
	for (i = 0; i < g_classEntryCount; i++) {
		if (g_classTable[i].m_owner == p_player->m_index) {
			count++;
		}
	}

	buffer = StaticPoolAlloc(GetObjSize() * count & 0xffff, g_staticPoolTags[2]);
	if (buffer == NULL) {
		return 0;
	}

	for (i = 0; i < g_classEntryCount; i++) {
		if (g_classTable[i].m_owner == p_player->m_index) {
			LoadClassEntryShape(i, p_player->m_baseLevel, buffer);
			buffer += GetObjSize();
			ReleaseClassEntryShape(i, p_player->m_baseLevel);
		}
	}

	return 1;
}

// Adds p_unk0x00 as level p_level of a class entry: level 0 starts a new entry, a later level
// goes to entry p_index. The unset levels above it take the same value. Returns the entry's
// index, or -1.
int32_t AddClassEntryLevel(
	int32_t p_unk0x00,
	uint32_t p_unk0x04,
	uint32_t p_unk0x08,
	uint32_t p_unk0x0c,
	int32_t p_unk0x10,
	int32_t p_level,
	int32_t p_index,
	int16_t p_unk0x1c
)
{
	int32_t index;
	ClassEntry* entry;
	int32_t i;

	index = -1;
	if (!g_classTableReady) {
		ResetClassTable();
	}

	if (g_classEntryCount >= 0x30c) {
		return -1;
	}

	if (p_unk0x10 == -1) {
		p_unk0x10 = -2;
	}

	if (p_level == 0) {
		entry = &g_classTable[g_classEntryCount];
		entry->m_owner = -2;
		entry->m_loadedLevel = -1;
		entry->m_parent = p_unk0x10;
		entry->m_x = p_unk0x04;
		entry->m_y = p_unk0x08;
		entry->m_z = p_unk0x0c;
		entry->m_resourceIds[p_level] = p_unk0x00;
		entry->m_shape = NULL;
		entry->m_obj = NULL;
		entry->m_partId = 0;
		entry->m_released = 0;
		index = g_classEntryCount++;
	}
	else {
		index = p_index;
		if (index < 0) {
			return -1;
		}

		entry = &g_classTable[index];
	}

	entry->m_resourceIds[p_level] = p_unk0x00;
	entry->m_kinds[p_level] = p_unk0x1c;
	if (p_level < 5) {
		for (i = p_level + 1; i < 5; i++) {
			if (entry->m_resourceIds[i] == -1) {
				entry->m_resourceIds[i] = p_unk0x00;
				entry->m_kinds[i] = p_unk0x1c;
			}
		}
	}

	return index;
}

void ResetClassTable(void)
{
	ClassEntry* entry;
	int32_t i;
	int32_t level;

	for (i = 0; i < 0x30c; i++) {
		entry = &g_classTable[i];
		entry->m_owner = -1;
		entry->m_loadedLevel = -1;
		entry->m_parent = -1;
		entry->m_shape = NULL;
		entry->m_obj = NULL;
		entry->m_partId = 0;
		entry->m_released = 0;
		entry->m_x = entry->m_y = entry->m_z = 0;
		for (level = 0; level < 5; level++) {
			entry->m_resourceIds[level] = -1;
			entry->m_kinds[level] = 0;
		}
	}

	g_classEntryCount = 0;
	g_classTableReady = 1;
}

// Gives the entries added since the last call (owner -2) to p_player.
void ClaimNewClassEntries(Player* p_player)
{
	int32_t i;
	ClassEntry* entry;

	for (i = 0; i < g_classEntryCount; i++) {
		entry = &g_classTable[i];
		if (entry->m_owner == -2) {
			entry->m_owner = p_player->m_index;
			if (entry->m_parent == -1) {
				entry->m_parent = -2;
			}
		}
	}
}

// Loads the level-p_level shapes of p_owner's entries, or, for a player with flag 2, keeps the
// shape already loaded. Returns 0 when a shape doesn't load.
int32_t LoadClassLevel(int32_t p_owner, int32_t p_level)
{
	int32_t i;
	ClassEntry* entry;
	int32_t result;

	result = 1;
	if (p_level == -1) {
		return 1;
	}

	g_shapeOwnerSet = 1;
	g_shapeOwnerKind = 0x100;
	g_shapeOwner = p_owner;
	for (i = 0; i < g_classEntryCount; i++) {
		entry = &g_classTable[i];
		if (entry->m_owner == p_owner) {
			if ((g_players[entry->m_owner]->m_flags & 2) && !entry->m_partId) {
				HideShape(entry->m_shape);
			}
			else if (!LoadClassEntryShape(i, p_level, NULL)) {
				result = 0;
			}

			if (!entry->m_partId) {
				DisableShapeCollision(entry->m_shape);
			}
		}
	}

	g_shapeOwnerSet = 0;
	return result;
}

// Releases the level-p_level shapes of p_owner's entries.
void ReleaseClassLevel(int32_t p_owner, int32_t p_level)
{
	int32_t i;

	for (i = 0; i < g_classEntryCount; i++) {
		if (g_classTable[i].m_owner == p_owner) {
			ReleaseClassEntryShape(i, p_level);
		}
	}
}

// Loads the level-p_level shape of entry p_index in place of the one it has, and gives it an object:
// under its player's object, or under the object (or shape object) of entry m_parent, made in
// p_buffer if given. Returns whether the entry has its shape.
int32_t LoadClassEntryShape(int32_t p_index, int32_t p_level, void* p_buffer)
{
	int32_t kind;
	int32_t offset;
	uint8_t* data;
	ClassEntry* entry;
	uint32_t flags;
	int32_t size;
	struct SceneObject* parent;
	struct SceneObject* obj;
	int32_t placed;
	Shape* shape;

	size = 0;
	offset = 0;
	flags = 0;
	entry = &g_classTable[p_index];
	if (entry->m_released) {
		return true;
	}

	if (entry->m_shape) {
		if ((entry->m_shape->m_kind & 0xf0) == 0x50) {
			return true;
		}

		flags = GetShapeState(entry->m_shape);
		DestroyObjShape(entry->m_shape);
		entry->m_shape = NULL;
	}

	data = LoadCachedResource(g_mw2PrjHandle, entry->m_resourceIds[p_level], g_resourceTypeTags[c_resTagPoly]);
	if (data) {
		size = GetPrjResourceSize(g_mw2PrjHandle, g_resourceTypeTags[c_resTagPoly], entry->m_resourceIds[p_level]);
	}
	else {
		return false;
	}

	entry->m_loadedLevel = -1;
	SetShapeScale(1, 1, 1);
	SetShapeFlags(0);
	while (!entry->m_shape) {
		entry->m_shape = LoadShapes(data, &offset, size, NULL);
		if (!entry->m_shape && !PurgeOldestCacheEntry()) {
			break;
		}
	}

	UnlockCachedResource(entry->m_resourceIds[p_level], g_resourceTypeTags[c_resTagPoly]);
	if (entry->m_shape) {
		parent = NULL;
		placed = false;
		SetShapeState(entry->m_shape, flags);
		entry->m_loadedLevel = p_level;
		if (!entry->m_obj) {
			if (entry->m_parent == -2) {
				if (!p_buffer) {
					entry->m_obj = CreateObj(g_players[entry->m_owner]->m_obj, 10);
				}
				else {
					entry->m_obj = InitObj(g_players[entry->m_owner]->m_obj, p_buffer);
				}
			}
			else {
				parent = g_classTable[entry->m_parent].m_obj;
				if (!parent) {
					shape = g_classTable[entry->m_parent].m_shape;
					parent = GetShapeObject(shape);
				}

				if (!p_buffer) {
					obj = CreateObj(parent, 10);
				}
				else {
					obj = InitObj(parent, p_buffer);
				}

				if (obj) {
					entry->m_obj = obj;
					placed = true;
				}
			}
		}

		SetObjShape(entry->m_obj, entry->m_shape);
		SetShapeObject(entry->m_shape, entry->m_obj);
		kind = entry->m_kinds[p_level] & 0xf0;
		SetShapeKind(entry->m_shape, kind | 0x100);
		SetShapeOwner(entry->m_shape, entry->m_owner);
		SetShapePartId(entry->m_shape, entry->m_partId);
		AddSceneShape(entry->m_shape);
		if (kind == 0x70) {
			HideShape(entry->m_shape);
			DisableShapeCollision(entry->m_shape);
		}
		else {
			ShowShape(entry->m_shape);
			SetShapeCollisionType(entry->m_shape, 6);
		}

		if (g_players[entry->m_owner]->m_flags & 0x4000) {
			HideObjTree(g_players[entry->m_owner]->m_obj);
			DisableObjTreeCollision(g_players[entry->m_owner]->m_obj);
			g_players[entry->m_owner]->m_flags |= 0x800;
		}

		if (kind == 0xa0) {
			SetObjTreeKind(entry->m_obj, 0x1a0);
		}

		if (placed) {
			SetObjPosition(entry->m_obj, entry->m_x, entry->m_y, entry->m_z);
		}

		UpdateObj(entry->m_obj);
		return true;
	}
	else {
		return false;
	}
}

// Releases the shapes of the first player whose m_detailLevel names a level, and clears it.
void ReleasePendingDetailLevel(void)
{
	int32_t i;
	int32_t done;

	done = false;
	for (i = 0; i < g_playerCount && !done; i++) {
		if (g_players[i]->m_detailLevel != -1) {
			done = true;
			ReleaseClassLevel(i, g_players[i]->m_detailLevel);
			g_players[i]->m_detailLevel = -1;
		}
	}
}

// Releases entry p_index's shape if it is the one loaded for level p_level.
void ReleaseClassEntryShape(int32_t p_index, int32_t p_level)
{
	ClassEntry* entry;

	entry = &g_classTable[p_index];
	if (entry->m_loadedLevel >= 0 && entry->m_loadedLevel == p_level && entry->m_shape) {
		DestroyObjShape(entry->m_shape);
		entry->m_shape = NULL;
		entry->m_loadedLevel = -1;
	}
}

struct SceneObject* GetClassObject(int32_t p_index)
{
	struct SceneObject* obj;

	obj = NULL;
	if (p_index < g_classEntryCount && p_index >= 0) {
		obj = g_classTable[p_index].m_obj;
	}

	return obj;
}

Shape* GetClassShape(int32_t p_index)
{
	Shape* shape;

	shape = NULL;
	if (p_index < g_classEntryCount && p_index >= 0) {
		shape = g_classTable[p_index].m_shape;
	}

	return shape;
}

void SetClassEntryPartId(int32_t p_index, uint16_t p_value)
{
	ClassEntry* entry;

	entry = &g_classTable[p_index];
	entry->m_partId = p_value;
}

// Chooses each player's model level (Player::m_detailLevel) by its distance from the eyepoint: the
// nearest player within range gets level 0, the next two level 1, the rest 2 or 3 by distance.
// The local player's own view (GetViewMode == 0) takes level 4, and dead players 0 or 1.
void ChoosePlayerDetailLevels(void)
{
	int32_t scale;
	int32_t third;
	int32_t second;
	int32_t range1;
	int32_t dz;
	int32_t best;
	int32_t pitch;
	int32_t level;
	int32_t range2;
	PlayerDetail* entry;
	int32_t ex;
	int32_t ground;
	int32_t heading;
	int32_t ey;
	int32_t range0;
	int32_t ez;
	int32_t count;
	int32_t length;
	PlayerDetail entries[60];
	int32_t i;
	int32_t dx;
	int32_t nearest;
	Player* player;
	int32_t dy;

	count = 0;
	nearest = -1;
	second = -1;
	third = -1;
	scale = g_eyepoint->m_detailScale;
	range0 = FixedMul16(scale, 0xe10);
	range1 = FixedMul16(scale, 0x2134);
	range2 = FixedMul16(scale, 0x57e4);
	best = range0;
	ex = g_eyepoint->m_x;
	ey = g_eyepoint->m_y;
	ez = g_eyepoint->m_z;
	for (i = 0; i < g_playerCount; i++) {
		entry = &entries[i];
		entry->m_player = i;
		player = g_players[i];
		if (i == g_localPlayerId && !GetViewMode()) {
			entry->m_level = 4;
			entry->m_distance = 0;
		}
		else if (player->m_flags & 2) {
			if (i == g_localPlayerId) {
				entry->m_level = 0;
			}
			else {
				entry->m_level = 1;
			}
		}
		else {
			entry->m_level = -2;
			dx = player->m_position.m_x - ex;
			dy = player->m_position.m_y - ey;
			dz = player->m_position.m_z - ez;
			GetBearingAndRange(dx, dy, dz, &heading, &length, (uint32_t*) &ground, &pitch);
			entry->m_distance = length;
			if (ground < best) {
				best = ground;
				third = second;
				second = nearest;
				nearest = i;
			}
		}
	}

	for (i = 0; i < g_playerCount; i++) {
		entry = &entries[i];
		if (entry->m_level == -2) {
			if (entry->m_player == nearest && entry->m_distance < range0) {
				entry->m_level = 0;
			}
			else if (entry->m_player == second || (entry->m_player == third && entry->m_distance < range1)) {
				count++;
				entry->m_level = 1;
			}
			else if ((second == -1 || third == -1) && entry->m_distance < range1 && count < 3) {
				count++;
				entry->m_level = 1;
			}
			else if (entry->m_distance < range2) {
				entry->m_level = 2;
			}
			else {
				entry->m_level = 3;
			}
		}

		player = g_players[entry->m_player];
		level = player->m_detailLevel;
		if (entry->m_level >= 0 && entry->m_level != level && LoadClassLevel(player->m_index, entry->m_level)) {
			player->m_detailLevel = entry->m_level;
		}
	}
}

// Releases the shape of the entry whose object is p_obj.
void ReleaseObjShape(struct SceneObject* p_obj)
{
	int32_t i;
	ClassEntry* entry;

	for (i = 0; i < g_classEntryCount; i++) {
		entry = &g_classTable[i];
		if (entry->m_obj == p_obj) {
			if (entry->m_shape) {
				DestroyObjShape(entry->m_shape);
			}

			entry->m_shape = NULL;
			entry->m_loadedLevel = -1;
			entry->m_released = 1;
			break;
		}
	}
}

// Forgets the shape of the entry whose object is p_obj without releasing it.
void ForgetObjShape(struct SceneObject* p_obj)
{
	ClassEntry* entry;
	int32_t i;

	for (i = 0; i < g_classEntryCount; i++) {
		entry = &g_classTable[i];
		if (entry->m_obj == p_obj) {
			entry->m_shape = NULL;
			entry->m_loadedLevel = -1;
			entry->m_released = 0;
			break;
		}
	}
}
