#ifndef MISSIONUI_H
#define MISSIONUI_H

#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

class AudioSample;
class ButtonMenu;
class TextGlyph;

// A mech's name on the briefing screen: its glyph and the mech type it shows.
struct MechNameTag {
	TextGlyph* m_glyph;
	int32_t m_type; // an index into g_mechChassis, negative for none
};

// The functions and globals of missionui.cpp that other units use.
extern char* g_briefingVideos[12][2];
extern char* g_briefingScenarios[12];
extern char* g_clanVideos[6];
extern MechNameTag g_playerMechTags[3];
extern MechNameTag g_enemyMechTags[3];
extern int32_t g_briefingChassisCount;
extern int32_t g_briefingVideo;
extern TextGlyph* g_enemyFormationName;
extern TextGlyph* g_playerFormationName;
extern ButtonMenu* g_missionBriefingMenu;
extern AudioSample* g_launchSound;
extern AudioSample* g_trialSound;
extern uint8_t g_briefingTextColors[0x100];
extern int32_t g_briefingClan;
extern TextGlyph* g_briefingLines[3];
extern int32_t g_playerFormation;
extern int32_t g_briefingRival;
extern size_t g_briefingMessage;
extern int32_t g_enemyFormation;
extern char g_briefingLine[0x100];
extern int32_t g_briefingMission;
void ShellApplyMissionUiInfo(char* p_scenario, int32_t p_stars, int32_t p_video);
int32_t GetChassisCount();
void DrawMissionBriefing(TMPackDataBase* p_database, char** p_scenario, size_t p_wParam);

#endif // MISSIONUI_H
