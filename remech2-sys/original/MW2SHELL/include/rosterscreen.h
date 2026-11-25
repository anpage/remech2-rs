#ifndef ROSTERSCREEN_H
#define ROSTERSCREEN_H

#include "audiosample.h"
#include "buttonmenu.h"
#include "pilotrecord.h"
#include "screenfield.h"
#include "tmpackdatabase.h"
#include "types.h"

// The functions and globals of rosterscreen.cpp that other units use.
extern PilotRecord* g_rosterPilots[10];
extern ScreenField g_pilotRecordFields[8];
extern ScreenField g_missionListFields[19];
extern AudioSample* g_rosterSound;
extern int32_t g_missionListShown;
extern ButtonMenu* g_rosterMenu;
extern int32_t g_rosterCampaign;
extern char g_rosterFieldText[0x100];

void ShowPilotCallsigns();
void HidePilotCallsigns();
void ClearPilot(PilotRecord* p_pilot);
void SetActivePilot(PilotRecord* p_pilot);
void DrawPilotRoster(TMPackDataBase* p_database, int32_t p_campaign, uint8_t* p_pilotChosen, char**);
// The pilot roster's frame, implemented on the Rust side (src/shell/screens/roster.rs)
extern "C" void PilotRosterCallback(
	TMPackDataBase* p_database,
	int32_t* p_campaign,
	uint8_t* p_pilotChosen,
	char** p_scenario,
	int32_t p_msg
);

#endif // ROSTERSCREEN_H
