#ifndef RESOURCE_H
#define RESOURCE_H

#include "bwd.h"
#include "callbacks.h"
#include "missiontable.h"
#include "types.h"

struct SceneObject;
struct BwdRecord;
struct FormationNames;
struct FormationRecord;
struct PathRecord;
struct StarTable;
struct BwdObjectRecord;
struct IncludeRecord;
struct IncludeRecord2;
struct ScenarioTable;

// The functions and globals of resource.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MissionTable* g_missionTables[16];
	extern int32_t g_missionTableCounts[16];
	extern int32_t g_thingRecordCount;
	extern char* g_playerTeamFormation;
	extern char* g_otherTeamFormation;
	extern struct Player* g_lastPlayer;
	extern int32_t g_nextMangleBase;
	extern TimedCallbackFn g_taskFns[6];
	extern int32_t g_nextThingRecord;
	extern int32_t g_thingRecordIndices[0x96];
	extern struct ScenarioTable* g_scenarios;
	extern int32_t g_nextScenario;
	extern int32_t g_mangleBase;
	extern int32_t g_scenarioCount;

	void LoadMapBitmap(struct BwdRecord* p_record, int32_t* p_width, int32_t* p_height, int32_t* p_data);
	int32_t LoadScenarioTable(struct ScenarioTable* p_table);
	int32_t LoadMissionTable(MissionTable* p_table);
	int32_t LoadPathTable(struct PathRecord* p_record);
	int32_t LoadFormationTable(struct FormationRecord* p_record);
	void LoadStarTable(struct StarTable* p_table);
	void SetTeamFormations(struct FormationNames* p_record);
	int32_t ExecuteInclude(struct IncludeRecord* p_record, BwdStreamFn p_fn);
	int32_t RunIncludedStream(struct IncludeRecord2* p_record, BwdStreamFn p_fn);
	void FreeMissionTables(void);
	int32_t MapResourceId(int32_t p_id);
	void SetMangleBase(int32_t p_base);
	void CreateObjectNode(struct BwdObjectRecord* p_record, int32_t p_static, int32_t p_class, int32_t p_level);
	struct SceneObject* NextThingRecordObject(void);
	int32_t FindFreeGameThing(void);

#ifdef __cplusplus
}
#endif

#endif // RESOURCE_H
