#ifndef DEBRIEF_H
#define DEBRIEF_H

#include "archivereader.h"
#include "buttonmenu.h"
#include "collection.h"
#include "decomp.h"
#include "page.h"
#include "pilotrecord.h"
#include "tmpackdatabase.h"
#include "types.h"

// SIZE 0x34
// One objective of the mission results.
struct MissionObjective {
	MechS32 m_status;             // 0x00 — 0 failed, 1 successful
	MechS32 m_type;               // 0x04 — 1 primary, 2 secondary, 4 tertiary, 8 return
	undefined4 m_unk0x08;         // 0x08 — the simulator's; the debriefing doesn't read it
	MechS32 m_time;               // 0x0c — in seconds, negative when never reached
	undefined4 m_unk0x10;         // 0x10 — the simulator's; the debriefing doesn't read it
	MechChar m_description[0x20]; // 0x14
};

// SIZE 0x9d4
// The simulator's mission results (MW2MSN.CFG).
struct MissionResults {
	undefined4 m_unk0x00;              // 0x00 — the simulator's; the debriefing doesn't read it
	MechS32 m_objectiveCount;          // 0x04
	undefined4 m_unk0x08;              // 0x08 — the simulator's; the debriefing doesn't read it
	undefined4 m_unk0x0c;              // 0x0c — the simulator's; the debriefing doesn't read it
	MechS32 m_outcome;                 // 0x10 — 2 completed, 3 failed
	MissionObjective m_objectives[48]; // 0x14
};

// The functions and globals of debrief.cpp that other units use.
extern ButtonMenu* g_debriefMenu;
extern Page* g_debriefPage;
extern Collection* g_debriefPages;
extern ArchiveReader* g_aftermathReader;
extern PilotRecord g_pilotBeforeMission;
extern MissionResults g_missionResults;

void DrawMissionDebrief(TMPackDataBase* p_database, MechS32 p_campaign, char** p_scenario);
// The debriefing screen's frame, implemented on the Rust side (src/shell/screens/debrief.rs)
extern "C" void MissionDebriefCallback(
	TMPackDataBase* p_database,
	MechS32* p_campaign,
	MechU8* p_pilotChosen,
	char** p_scenario,
	MechS32 p_msg
);
// Implemented on the Rust side (src/shell/screens/debug.rs), around ReadMissionResultsC
extern "C" void ReadMissionResults(void* p_results);
void ReadMissionResultsC(void* p_results);

#endif // DEBRIEF_H
