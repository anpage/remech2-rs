#ifndef MENUDATA_H
#define MENUDATA_H

#include "campaignmission.h"
#include "formation.h"
#include "mainmenubutton.h"
#include "menuscreen.h"
#include "types.h"

// The functions and globals of menudata.cpp that other units use.
extern MechS32 g_textTabStops[19];
extern char* g_databaseName;
extern MechChar* g_archiveNames[2];
extern Formation g_formations[6];
extern MainMenuButton g_mainMenuButtons[4];
extern MainMenuButton g_missionBriefingButtons[0x19];
extern CampaignMission* g_campaignMissions[2];
extern MechChar** g_trainingScenarios[2];
extern MenuScreen g_clanHallScreens[3];
extern MenuScreen g_rosterScreens[3];
extern MenuScreen g_archiveScreens[3];
extern MenuScreen g_starConfigScreens[3];
extern MenuScreen g_readyRoomScreens[3];
extern MenuScreen g_debriefScreens[3];
extern MenuScreen g_aftermathScreens[3];
extern MenuScreen g_briefingScreens[3];
extern MenuScreen g_situationScreens[3];
extern MenuScreen g_cadetTrainingScreens[3];
extern MenuScreen g_mechBayScreens[3];

#endif // MENUDATA_H
