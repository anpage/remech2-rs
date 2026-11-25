#include "resource.h"

#include "ai.h"
#include "bwd.h"
#include "bwdobjectrecord.h"
#include "bwdrecord.h"
#include "bwdstreamkey.h"
#include "classtable.h"
#include "collision.h"
#include "config.h"
#include "decomp.h"
#include "error.h"
#include "files.h"
#include "geocache.h"
#include "includerecord.h"
#include "includerecord2.h"
#include "loadres.h"
#include "missionsetup.h"
#include "missiontable.h"
#include "mw2prj.h"
#include "object.h"
#include "objectanim.h"
#include "players.h"
#include "prjfile.h"
#include "quadtree.h"
#include "scenariotable.h"
#include "shape.h"
#include "shapegeom.h"
#include "shapelists.h"
#include "simmain.h"
#include "team.h"
#include "transform.h"
#include "types.h"
#include "weapons.h"
#include "wtbshapes.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// A bitmap record: its size and palette-sized data.
typedef struct BitmapRecord {
	BwdRecord m_header; // 0x00
	int32_t m_width;    // 0x08
	int32_t m_height;   // 0x0c
	int32_t m_data[1];  // 0x10 — up to the record's end
} BitmapRecord;

// A path record: the path's name and its waypoints.
typedef struct PathRecord {
	BwdRecord m_header;    // 0x00
	char m_name[0x40];     // 0x08
	PathPoint m_points[1]; // 0x48 — up to the record's end
} PathRecord;

// A formation record's slot: its offset from the leader and its heading.
typedef struct FormationSlot {
	int32_t m_x;       // 0x00
	int32_t m_z;       // 0x04
	int32_t m_heading; // 0x08
} FormationSlot;

// A formation record: the formation's name and its slots.
typedef struct FormationRecord {
	BwdRecord m_header;       // 0x00
	char m_name[0x10];        // 0x08
	FormationSlot m_slots[1]; // 0x18 — up to the record's end
} FormationRecord;

// A star record's entry: a team's two values and its formation's name.
typedef struct StarEntry {
	int32_t m_affiliation; // 0x00
	int32_t m_side;        // 0x04
	char m_name[0x10];     // 0x08
} StarEntry;

typedef struct StarTable {
	BwdRecord m_header;   // 0x00
	StarEntry m_stars[1]; // 0x08 — up to the record's end
} StarTable;

// A formation-name record: the sixteen teams' formations.
typedef struct FormationNames {
	BwdRecord m_header;     // 0x00
	char m_names[16][0x11]; // 0x08
} FormationNames;

ScenarioTable* g_scenarios = NULL;

// The next name of g_scenarios ExecuteInclude substitutes.
int32_t g_nextScenario = 0;

// The number of entries in g_thingRecordIndices, and the next one NextThingRecordObject returns.
int32_t g_thingRecordCount = 0;

int32_t g_nextThingRecord = 0;

int32_t g_mangleBase = 0;

// The mangle base of the world stream's next mangle_on section (BwdExecuteStream).
int32_t g_nextMangleBase = 0;

// The formation LoadStarTable gives the player's team, or NULL to use the record's.
char* g_playerTeamFormation = NULL;

// The formation LoadStarTable gives the other teams, or NULL to use the record's.
char* g_otherTeamFormation = NULL;

// The player the world stream (BwdExecuteStream) created last, for ReelMotionTask.
struct Player* g_lastPlayer = NULL;

// The task kinds of a world stream's task record (BwdExecuteStream): 0 spins a star's object, 1
// cycles its face colors, 2 moves it around a circle, 3 moves a thing's object through an
// animation, 4 loops a sound on a star's object, 5 moves it along a path.
TimedCallbackFn g_taskFns[6] = {SpinTask, ColorCycleTask, OrbitTask, ReelMotionTask, AmbientSoundTask, PathTask};

MissionTable* g_missionTables[16];

// The number of entries of each of g_missionTables.
int32_t g_missionTableCounts[16];

int32_t g_thingRecordIndices[0x96];

// The number of names in g_scenarios.
int32_t g_scenarioCount;

// Copies a bitmap record's size to p_width and p_height and its data to p_data, each if not NULL.
void LoadMapBitmap(BwdRecord* p_record, int32_t* p_width, int32_t* p_height, int32_t* p_data)
{
	BitmapRecord* record;
	int32_t count;
	int32_t i;

	record = (BitmapRecord*) p_record;
	if (p_width) {
		*p_width = record->m_width;
	}

	if (p_height) {
		*p_height = record->m_height;
	}

	if (p_data) {
		count = (record->m_header.m_size - 0x10) >> 2;
		for (i = 0; i < count; i++) {
			p_data[i] = record->m_data[i];
		}
	}
}

// Replaces the scenario table with a copy of p_table.
int32_t LoadScenarioTable(ScenarioTable* p_table)
{
	int32_t result;

	result = FALSE;
	if (g_scenarios) {
		MechHeapFree(g_primaryHeap, g_scenarios);
	}

	g_scenarios = NULL;
	g_scenarioCount = 0;
	g_nextScenario = 0;
	g_scenarios = MemAlloc(p_table->m_header.m_size);
	if (g_scenarios) {
		memcpy(g_scenarios, p_table, p_table->m_header.m_size);
		g_scenarioCount = (g_scenarios->m_header.m_size - 8) / 0xc;
		result = TRUE;
	}
	else {
		Error(0x3f, NULL);
	}

	return result;
}

// Replaces the mission table in p_table's slot with a copy of it.
int32_t LoadMissionTable(MissionTable* p_table)
{
	int32_t result;
	int32_t slot;

	result = FALSE;
	slot = p_table->m_star;
	if (g_missionTables[slot]) {
		MechHeapFree(g_primaryHeap, g_missionTables[slot]);
	}

	g_missionTables[slot] = NULL;
	g_missionTableCounts[slot] = 0;
	g_missionTables[slot] = MemAlloc(p_table->m_header.m_size);
	if (!g_missionTables[slot]) {
		Error(0x40, NULL);
	}
	else {
		result = TRUE;
		memcpy(g_missionTables[slot], p_table, p_table->m_header.m_size);
		g_missionTableCounts[slot] =
			(g_missionTables[slot]->m_header.m_size - sizeof(BwdRecord)) / sizeof(MissionEntry);
		SetUpStarMission(g_missionTables[slot]);
	}

	return result;
}

// Adds a path to g_paths, or returns FALSE if the table or the path is full.
int32_t LoadPathTable(PathRecord* p_record)
{
	int32_t count;
	int32_t result;
	int32_t index;
	int32_t i;
	PathRecord* record;

	result = FALSE;
	record = p_record;
	count = (record->m_header.m_size - 0x48) / 0x1c;
	if (g_pathCount >= 0x40 || count > 0x40) {
		return result;
	}

	index = g_pathCount++;
	g_paths[index].m_count = count;
	strncpy(g_paths[index].m_name, record->m_name, 0x40);
	for (i = 0; i < count; i++) {
		g_paths[index].m_points[i].m_x = record->m_points[i].m_x;
		g_paths[index].m_points[i].m_y = record->m_points[i].m_y;
		g_paths[index].m_points[i].m_z = record->m_points[i].m_z;
		g_paths[index].m_points[i].m_pitch = record->m_points[i].m_pitch << 16;
		g_paths[index].m_points[i].m_heading = record->m_points[i].m_heading << 16;
		g_paths[index].m_points[i].m_roll = record->m_points[i].m_roll << 16;
		g_paths[index].m_points[i].m_duration = record->m_points[i].m_duration;
	}

	return TRUE;
}

// Adds a formation to g_formationTemplates, or returns FALSE if the table or the formation is
// full.
int32_t LoadFormationTable(FormationRecord* p_record)
{
	int32_t count;
	int32_t result;
	int32_t index;
	int32_t i;
	FormationRecord* record;

	result = FALSE;
	record = p_record;
	count = (record->m_header.m_size - 0x18) / 0xc;
	if (g_formationTemplateCount >= 0x20 || count > 8) {
		return result;
	}

	index = g_formationTemplateCount++;
	strncpy(g_formationTemplates[index].m_name, record->m_name, 0x10);
	for (i = 0; i < count; i++) {
		g_formationTemplates[index].m_x[i] = record->m_slots[i].m_x;
		g_formationTemplates[index].m_z[i] = record->m_slots[i].m_z;
		g_formationTemplates[index].m_heading[i] = record->m_slots[i].m_heading;
	}

	return TRUE;
}

// Runs p_fn on the stream an include record names. A name of "^" with id -1 or -2 takes the
// scenario table's next name.
int32_t ExecuteInclude(IncludeRecord* p_record, BwdStreamFn p_fn)
{
	BwdStream streamData;
	BwdStreamKey keyData;
	IncludeRecord* record;
	int32_t ok;
	BwdStreamKey* key;
	BwdStream* stream;
	int32_t result;

	record = p_record;
	key = &keyData;
	ok = TRUE;
	result = FALSE;
	key->m_id = record->m_id;
	strncpy(key->m_name, record->m_name, 0xc);
	key->m_name[0xc] = '\0';
	if ((key->m_id == -2 || key->m_id == -1) && key->m_name[0] == '^') {
		if (!g_scenarios) {
			Error(0x39, NULL);
			ok = FALSE;
		}
		else if (g_nextScenario < g_scenarioCount) {
			strncpy(key->m_name, g_scenarios->m_names[g_nextScenario], 0xc);
			g_nextScenario++;
			key->m_name[0xc] = '\0';
		}
		else {
			Error(0x3b, NULL);
			ok = FALSE;
		}
	}

	if (ok) {
		stream = OpenBwdStream(key, &streamData);
		if (stream) {
			result = TRUE;
			result &= p_fn(stream);
			UnloadResource(stream);
		}
		else {
			Error(0x34, NULL);
		}
	}

	return result;
}

// Sets up the teams from a star record: each team's values, and its formation.
void LoadStarTable(StarTable* p_table)
{
	int32_t count;
	StarTable* table;
	int32_t i;

	table = p_table;
	count = (table->m_header.m_size - 8) / sizeof(StarEntry);
	for (i = 0; i < count; i++) {
		g_teams[i].m_affiliation = table->m_stars[i].m_affiliation;
		g_teams[i].m_side = table->m_stars[i].m_side;
		g_starSides[table->m_stars[i].m_affiliation] = table->m_stars[i].m_side;
		if (g_localStar + 1 == i && g_playerTeamFormation) {
			SetTeamFormationByName(i, g_playerTeamFormation);
		}
		else if (g_localStar + 1 != i && g_otherTeamFormation) {
			SetTeamFormationByName(i, g_otherTeamFormation);
		}
		else {
			SetTeamFormationByName(i, table->m_stars[i].m_name);
		}
	}
}

// Gives the sixteen teams the formations a record names.
void SetTeamFormations(FormationNames* p_record)
{
	FormationNames* record;
	int32_t i;

	record = p_record;
	for (i = 0; i < 16; i++) {
		SetTeamFormationByName(i, record->m_names[i]);
	}
}

// Runs p_fn on the stream a record names.
int32_t RunIncludedStream(IncludeRecord2* p_record, BwdStreamFn p_fn)
{
	BwdStream streamData;
	BwdStreamKey keyData;
	int32_t id;
	IncludeRecord2* record;
	BwdStreamKey* key;
	BwdStream* stream;
	int32_t result;

	record = p_record;
	key = &keyData;
	result = FALSE;
	id = record->m_id;
	strncpy(key->m_name, record->m_name, 0xc);
	key->m_name[0xc] = '\0';
	key->m_id = id;
	stream = OpenBwdStream(key, &streamData);
	if (stream) {
		result = TRUE;
		result = p_fn(stream);
		UnloadResource(stream);
	}
	else {
		Error(0x35, NULL);
	}

	return result;
}

// Frees the mission tables' blocks.
void FreeMissionTables(void)
{
	int32_t i;

	if (g_scenarios) {
		MechHeapFree(g_primaryHeap, g_scenarios);
	}

	g_scenarios = NULL;
	for (i = 0; i < 16; i++) {
		if (g_missionTables[i]) {
			MechHeapFree(g_primaryHeap, g_missionTables[i]);
		}

		g_missionTables[i] = NULL;
	}
}

// Scales the vector (p_a, p_b, p_c) to integers whose absolute values add up to 2^29. A vector
// that small is left alone.
void ScaleNormal(float p_a, float p_b, float p_c, undefined4* p_x, undefined4* p_y, undefined4* p_z)
{
	float scale;

	scale = fabs(p_a);
	scale += fabs(p_b);
	if ((scale += fabs(p_c)) > 0.0001) {
		scale /= 536870912.0;
		*p_x = p_a / scale;
		*p_y = p_b / scale;
		*p_z = p_c / scale;
	}
}

// Scales the plane (p_a, p_b, p_c, p_d) like ScaleNormal.
void ScalePlane(
	float p_a,
	float p_b,
	float p_c,
	float p_d,
	undefined4* p_x,
	undefined4* p_y,
	undefined4* p_z,
	undefined4* p_w
)
{
	float scale;

	scale = fabs(p_a);
	scale += fabs(p_b);
	scale += fabs(p_c);
	if ((scale += fabs(p_d)) > 0.0001) {
		scale /= 536870912.0;
		*p_x = p_a / scale;
		*p_y = p_b / scale;
		*p_z = p_c / scale;
		*p_w = p_d / scale;
	}
}

// The normal of the triangle (p_x1, p_y1, p_z1), (p_x2, p_y2, p_z2), (p_x3, p_y3, p_z3), scaled
// by ScaleNormal.
void GetTriangleNormal(
	float p_x1,
	float p_y1,
	float p_z1,
	float p_x2,
	float p_y2,
	float p_z2,
	float p_x3,
	float p_y3,
	float p_z3,
	undefined4* p_x,
	undefined4* p_y,
	undefined4* p_z
)
{
	float a;
	float b;
	float c;
	float ux;
	float uy;
	float uz;
	float vx;
	float vy;
	float vz;

	ux = p_x2 - p_x1;
	uy = p_y2 - p_y1;
	uz = p_z2 - p_z1;
	vx = p_x3 - p_x1;
	vy = p_y3 - p_y1;
	/* The original takes p_y1, not p_z1, from p_z3. */
	a = (vz = p_z3 - p_y1) * uy - vy * uz;
	b = vz * ux - vx * uz;
	c = vy * ux - vx * uy;
	ScaleNormal(a, b, c, p_x, p_y, p_z);
}

// The plane of the triangle, scaled by ScalePlane.
void GetTrianglePlane(
	float p_x1,
	float p_y1,
	float p_z1,
	float p_x2,
	float p_y2,
	float p_z2,
	float p_x3,
	float p_y3,
	float p_z3,
	undefined4* p_x,
	undefined4* p_y,
	undefined4* p_z,
	undefined4* p_w
)
{
	float a;
	float b;
	float c;
	float d;
	float ux;
	float uy;
	float uz;
	float vx;
	float vy;
	float vz;

	ux = p_x2 - p_x1;
	uy = p_y2 - p_y1;
	uz = p_z2 - p_z1;
	vx = p_x3 - p_x1;
	vy = p_y3 - p_y1;
	/* The original takes p_y1, not p_z1, from p_z3. */
	a = (vz = p_z3 - p_y1) * uy - vy * uz;
	b = vz * ux - vx * uz;
	d = -((c = vy * ux - vx * uy) * p_z1 + b * p_y1 + a * p_x1);
	ScalePlane(a, b, c, d, p_x, p_y, p_z, p_w);
}

void GetPointNormalPlane(
	float p_x,
	float p_y,
	float p_z,
	float p_nx,
	float p_ny,
	float p_nz,
	undefined4* p_planeX,
	undefined4* p_planeY,
	undefined4* p_planeZ,
	undefined4* p_planeW
)
{
	float a;
	float b;
	float c;
	float d;

	a = p_nx;
	b = -p_ny;
	c = p_nz;
	d = -(p_x * a + p_y * b + p_z * c);
	ScalePlane(a, b, c, d, p_planeX, p_planeY, p_planeZ, p_planeW);
}

int32_t MapResourceId(int32_t p_id)
{
	return g_mangleBase + p_id;
}

void SetMangleBase(int32_t p_base)
{
	g_mangleBase = p_base;
}

// Creates the shape of a world stream's object record: a static object of the current block
// (p_static), an entry of the class table (p_class, for level p_level), or else a shape of its own,
// in the world or under its parent's object.
void CreateObjectNode(
	BwdObjectRecord* p_record,
	undefined4 p_unk0x04,
	int32_t p_unk0x08,
	int32_t p_static,
	int32_t p_class,
	int32_t p_level
)
{
	int32_t offset;
	int32_t unk0x34;
	Xform xform;
	int32_t parent;
	int32_t fromResource;
	int32_t parentIndex;
	uint8_t* data;
	int32_t flags;
	int32_t kind;
	char name[13];
	BwdObjectRecord* record;
	int32_t mapped;
	Shape* shape;
	int32_t thing;
	Matrix matrix;
	int32_t resource;
	int32_t first;
	int32_t id;
	int32_t size;
	int32_t handle;
	SceneObject* parentObj;
	SceneObject* obj;
	Shape* classShape;

	record = p_record;
	flags = 0;
	data = NULL;
	size = 0;
	offset = 0;
	first = TRUE;
	fromResource = FALSE;
	resource = record->m_resource;
	id = record->m_id;
	parent = record->m_parent;
	kind = record->m_kind;
	unk0x34 = record->m_shapeKind;
	if (kind < 0 || kind >= 8) {
		kind = 4;
	}

	xform = record->m_xform;
	flags = record->m_flags;
	if (resource != -1) {
		data = LoadCachedResource(g_mw2PrjHandle, resource, g_resourceTypeTags[c_resTagPoly], 0);
		if (data) {
			size = GetPrjResourceSize(g_mw2PrjHandle, g_resourceTypeTags[c_resTagPoly], resource);
			fromResource = TRUE;
		}
		else {
			Error(0x38, NULL);
			return;
		}
	}

	if (fromResource) {
		if (p_static && g_currentBlock != -1) {
			parentIndex = -1;
			if (parent == -2) {
				parentIndex = -2;
			}
			else if (parent != -1) {
				parent = MapResourceId(parent);
				parentIndex = FindStarIdxById(parent);
				if (parentIndex == -1) {
					Error(0x2d, NULL);
				}
			}

			id = MapResourceId(id);
			PlaceStaticObject(id, resource, xform, g_currentBlock, parentIndex, p_unk0x08, flags, kind, unk0x34);
			UnlockCachedResource(resource, g_resourceTypeTags[c_resTagPoly]);
			return;
		}
		else if (p_class && g_thingCapacity > g_thingCount) {
			g_thingIds[g_thingCount] = MapResourceId(id);
			mapped = MapResourceId(id);
			thing = FindThingIdxById(mapped);
			parent = MapResourceId(parent);
			parentIndex = FindThingIdxById(parent);
			g_thingIndices[g_thingCount] =
				AddClassEntryLevel(resource, xform.m_x, xform.m_y, xform.m_z, parentIndex, p_level, thing, unk0x34);
			g_thingCount++;
			UnlockCachedResource(resource, g_resourceTypeTags[c_resTagPoly]);
			return;
		}
	}

	if (!fromResource) {
		strncpy(name, record->m_file, 12);
		name[12] = '\0';
		if (strlen(name) <= 8) {
			strcat(name, ".wtb");
		}

		handle = LoadFile(BuildGamePath(name), &size, (void**) &data, NULL);
		if (handle != -1) {
			MechClose(handle);
		}
		else {
			Error(0x36, NULL);
			return;
		}
	}

	SetShapeScale(xform.m_scaleX, xform.m_scaleY, xform.m_scaleZ);
	SetShapeFlags(flags);
	shape = LoadShapes(data, &offset, size, NULL);
	if (shape) {
		shape->m_kind = unk0x34;
		if (kind >= 0 && kind < 8) {
			SetShapeCollisionType(shape, kind);
		}
		else {
			SetShapeCollisionType(shape, 4);
			Error(0x37, NULL);
		}

		if (first) {
			mapped = MapResourceId(id);
			AddClass(mapped, shape);
		}

		first = FALSE;
		if (parent == -2) {
			BuildMatrix(&matrix, xform.m_angleX, xform.m_angleY, xform.m_angleZ, xform.m_x, xform.m_y, xform.m_z);
			TransformShape(shape, &matrix);
			AddSceneShape(shape);
			if (shape->m_collisionType == 5) {
				BuildShapeQuadtree(shape);
			}
		}
		else {
			parentObj = NULL;
			parent = MapResourceId(parent);
			classShape = FindClassById(parent);
			if (classShape) {
				parentObj = GetShapeObject(classShape);
			}

			obj = CreateObj(parentObj, 10);
			SetObjShape(obj, shape);
			SetShapeObject(shape, obj);
			SetObjRotation(obj, xform.m_angleX, xform.m_angleY, xform.m_angleZ, 0);
			SetObjPosition(obj, xform.m_x, xform.m_y, xform.m_z);
			UpdateObj(obj);
			AddSceneShape(shape);
		}
	}

	if (!fromResource) {
		MechHeapFree(g_primaryHeap, data);
	}
	else {
		FreeCachedResource(resource, g_resourceTypeTags[c_resTagPoly]);
	}
}

// Returns the object of the next entry of g_thingRecordIndices, or NULL after the last.
struct SceneObject* NextThingRecordObject(void)
{
	int32_t id;
	struct SceneObject* obj;

	if (g_nextThingRecord < g_thingRecordCount) {
		id = g_thingRecordIndices[g_nextThingRecord];
		obj = GetClassObject(id);
		g_nextThingRecord++;
	}
	else {
		obj = NULL;
	}

	return obj;
}

// Returns the next free game thing index, or -1 when all 254 are taken.
int32_t FindFreeGameThing(void)
{
	int32_t index;

	index = -1;
	if (g_gameThingCount < 0xfe) {
		index = g_gameThingCount;
		g_gameThingCount++;
	}
	else {
		Error(0x2f, "Too many gamethings", 0);
	}

	return index;
}
