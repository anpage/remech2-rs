#ifndef DEBRIEF_H
#define DEBRIEF_H

#include "archivereader.h"
#include "buttonmenu.h"
#include "collection.h"
#include "decomp.h"
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
extern MechChar g_objectiveStatus[0x80];
extern MechChar g_debriefText[0x1000];
extern MissionResultObjective* g_sortedObjectives[48];
extern MechChar g_objectiveLine[0x400];
extern MechChar g_careerHonor[0x200];
extern undefined g_unk0x10077fe0[0x100];
extern MechChar g_objectiveDescription[0x80];
extern MechChar g_honorPoints[0x200];
extern MechChar g_objectiveType[0x80];
extern MechChar g_skillName[0x200];
extern MechChar g_careerHonorLine[0x200];
extern MechChar g_honorLine[0x200];
extern MechChar g_objectiveTime[0x80];

void DrawMissionDebrief(TMPackDataBase* p_database, MechS32 p_campaign, char** p_scenario);
// The debriefing screen's frame, implemented on the Rust side (src/shell/screens/debrief.rs)
extern "C" void MissionDebriefCallback(
	TMPackDataBase* p_database,
	MechS32* p_campaign,
	MechU8* p_pilotChosen,
	char** p_scenario,
	MechS32 p_msg
);
// The last mission's report with the debug menu's outcome override applied.
// Implemented on the Rust side (src/shell/screens/debug.rs)
extern "C" void ReadMissionReport(MissionReport* p_report);

#endif // DEBRIEF_H
