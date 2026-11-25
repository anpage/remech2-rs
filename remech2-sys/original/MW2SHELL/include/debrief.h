#ifndef DEBRIEF_H
#define DEBRIEF_H

#include "archivereader.h"
#include "buttonmenu.h"
#include "collection.h"
#include "mission.h"
#include "page.h"
#include "pilotrecord.h"
#include "tmpackdatabase.h"
#include "types.h"

// The functions and globals of debrief.cpp that other units use.
extern ButtonMenu* g_debriefMenu;
extern Page* g_debriefPage;
extern Collection* g_debriefPages;
extern ArchiveReader* g_aftermathReader;
extern PilotRecord g_pilotBeforeMission;
extern MissionResult g_missionResults;
extern char g_objectiveStatus[0x80];
extern char g_debriefText[0x1000];
extern MissionResultObjective* g_sortedObjectives[48];
extern char g_objectiveLine[0x400];
extern char g_careerHonor[0x200];
extern char g_objectiveDescription[0x80];
extern char g_honorPoints[0x200];
extern char g_objectiveType[0x80];
extern char g_skillName[0x200];
extern char g_careerHonorLine[0x200];
extern char g_honorLine[0x200];
extern char g_objectiveTime[0x80];

void DrawMissionDebrief(TMPackDataBase* p_database, int32_t p_campaign, char** p_scenario);
// The debriefing screen's frame, implemented on the Rust side (src/shell/screens/debrief.rs)
extern "C" void MissionDebriefCallback(
	TMPackDataBase* p_database,
	int32_t* p_campaign,
	uint8_t* p_pilotChosen,
	char** p_scenario,
	int32_t p_msg
);
// The last mission's report with the debug menu's outcome override applied.
// Implemented on the Rust side (src/shell/screens/debug.rs)
extern "C" void ReadMissionReport(MissionReport* p_report);

#endif // DEBRIEF_H
