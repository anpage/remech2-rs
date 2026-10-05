#ifndef MISSIONUI_H
#define MISSIONUI_H

#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

class AudioSample;
class ButtonMenu;
class TextGlyph;

// SIZE 0x08
// A mech's name on the briefing screen: its glyph and the mech type it shows.
struct MechNameTag {
	TextGlyph* m_glyph; // 0x00
	MechS32 m_type;     // 0x04 — an index into g_mechChassis, negative for none
};

// The functions and globals of missionui.cpp that other units use.
extern MechChar* g_briefingVideos[12][2];
extern MechChar* g_briefingScenarios[12];
extern MechChar* g_clanVideos[6];
extern MechNameTag g_playerMechTags[3];
extern MechNameTag g_enemyMechTags[3];
extern MechS32 g_briefingChassisCount;
extern MechS32 g_briefingVideo;
extern TextGlyph* g_enemyFormationName;
extern TextGlyph* g_playerFormationName;
extern ButtonMenu* g_missionBriefingMenu;
extern AudioSample* g_launchSound;
extern AudioSample* g_trialSound;
extern MechU8 g_briefingTextColors[0x100];
extern MechS32 g_briefingClan;
extern TextGlyph* g_briefingLines[3];
extern MechS32 g_playerFormation;
extern MechS32 g_briefingRival;
extern size_t g_briefingMessage;
extern MechS32 g_enemyFormation;
extern MechChar g_briefingLine[0x100];
extern MechS32 g_briefingMission;
void ShellApplyMissionUiInfo(MechChar* p_scenario, MechS32 p_stars, MechS32 p_video);
MechS32 GetChassisCount();
void DrawMissionBriefing(TMPackDataBase* p_database, MechChar** p_scenario, size_t p_wParam);

#endif // MISSIONUI_H
