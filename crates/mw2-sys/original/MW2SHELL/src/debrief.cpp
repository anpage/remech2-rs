#include "debrief.h"

#include "archivereader.h"
#include "buttonmenu.h"
#include "campaignmission.h"
#include "collection.h"
#include "customstar.h"
#include "decomp.h"
#include "files.h"
#include "font.h"
#include "keyboardinput.h"
#include "mainmenubutton.h"
#include "mechbay.h"
#include "mechchassis.h"
#include "mechvariant.h"
#include "menudata.h"
#include "menuscreen.h"
#include "messages.h"
#include "mousestate.h"
#include "options.h"
#include "page.h"
#include "pilotrecord.h"
#include "pilotroster.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "stringutil.h"
#include "textpages.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(1)
// SIZE 0x50
// The simulator's career record (MW2CAR.CFG): the last mission's statistics. The simulator
// counts a kill in the "direct" members when the player made it, and in the totals for every
// enemy destroyed; the debriefing prints them in that order.
struct CareerRecord {
	undefined m_unk0x00[0x07 - 0x00]; // 0x00 — the simulator's; the debriefing doesn't read it
	MechU16 m_directMechKills;        // 0x07
	undefined m_unk0x09[0x13 - 0x09]; // 0x09 — the simulator's; the debriefing doesn't read it
	MechU16 m_shotsFired;             // 0x13
	MechU16 m_hits;                   // 0x15
	undefined m_unk0x17[0x1e - 0x17]; // 0x17 — the simulator's; the debriefing doesn't read it
	MechU16 m_mechKills;              // 0x1e
	undefined m_unk0x20[0x34 - 0x20]; // 0x20 — the simulator's; the debriefing doesn't read it
	MechU16 m_wingmenLost;            // 0x34
	undefined m_unk0x36[0x44 - 0x36]; // 0x36 — the simulator's; the debriefing doesn't read it
	MechU16 m_directVehicleKills;     // 0x44
	undefined m_unk0x46[0x4a - 0x46]; // 0x46 — the simulator's; the debriefing doesn't read it
	MechU16 m_vehicleKills;           // 0x4a
	undefined m_unk0x4c[0x50 - 0x4c]; // 0x4c — the simulator's; the debriefing doesn't read it
};
#pragma pack()

DECOMP_SIZE_ASSERT(MissionObjective, 0x34)
DECOMP_SIZE_ASSERT(MissionResults, 0x9d4)
DECOMP_SIZE_ASSERT(CareerRecord, 0x50)

// The debriefing screen.
// GLOBAL: MW2SHELL 0x1005b040
ButtonMenu* g_debriefMenu = NULL;

// GLOBAL: MW2SHELL 0x1005b044
Page* g_debriefPage = NULL;

// GLOBAL: MW2SHELL 0x1005b048
Collection* g_debriefPages = NULL;

// The aftermath reader, while it is open.
// GLOBAL: MW2SHELL 0x1005b04c
ArchiveReader* g_aftermathReader = NULL;

// The debriefing's text buffers.
// GLOBAL: MW2SHELL 0x10076860
MechChar g_objectiveStatus[0x80];

// GLOBAL: MW2SHELL 0x100768e0
MechChar g_debriefText[0x1000];

// The mission's objectives, in the order CompareObjectives sorts them.
// GLOBAL: MW2SHELL 0x100778e0
MissionObjective* g_sortedObjectives[48];

// GLOBAL: MW2SHELL 0x100779a0
MechChar g_objectiveLine[0x400];

// GLOBAL: MW2SHELL 0x10077da0
MechChar g_careerHonor[0x200];

// The pilot as the mission found them, restored by a replay.
// GLOBAL: MW2SHELL 0x10077fa0
PilotRecord g_pilotBeforeMission;

// An identity color map (0 maps to 0xff) that BuildDebriefText builds and nothing reads.
// GLOBAL: MW2SHELL 0x10077fe0
undefined g_unk0x10077fe0[0x100];

// GLOBAL: MW2SHELL 0x100780e0
MissionResults g_missionResults;

// GLOBAL: MW2SHELL 0x10078ab8
MechChar g_objectiveDescription[0x80];

// GLOBAL: MW2SHELL 0x10078b38
MechChar g_honorPoints[0x200];

// GLOBAL: MW2SHELL 0x10078d38
MechChar g_objectiveType[0x80];

// GLOBAL: MW2SHELL 0x10078db8
MechChar g_skillName[0x200];

// GLOBAL: MW2SHELL 0x10078fb8
MechChar g_careerHonorLine[0x200];

// GLOBAL: MW2SHELL 0x100791b8
MechChar g_honorLine[0x200];

// GLOBAL: MW2SHELL 0x100793b8
MechChar g_objectiveTime[0x80];

void MissionDebriefCallback(TMPackDataBase*, MechS32* p_campaign, MechU8*, MechChar** p_scenario, MechS32 p_msg);

// Returns TRUE when one of the options that makes a trial easier is set.
// FUNCTION: MW2SHELL 0x10001000
MechU8 HasEasyOptions()
{
	if (g_difficultyConfig.m_unlimitedAmmo == 1) {
		return TRUE;
	}
	if (g_difficultyConfig.m_invulnerability == 1) {
		return TRUE;
	}
	if (!g_difficultyConfig.m_collisionDamage) {
		return TRUE;
	}

	return FALSE;
}

// The qsort order of the debriefing's objectives: by the time they were reached, the ones never
// reached last.
// Not 100%: the stack slots of a, b, first and second are permuted.
// FUNCTION: MW2SHELL 0x10001056
int CompareObjectives(const void* p_a, const void* p_b)
{
	MissionObjective** a = (MissionObjective**) p_a;
	MissionObjective** b = (MissionObjective**) p_b;
	MissionObjective* first = *a;
	MissionObjective* second = *b;

	if (first->m_time < 0) {
		return 1;
	}
	if (second->m_time < 0) {
		return -1;
	}
	if (second->m_time < first->m_time) {
		return 1;
	}
	if (second->m_time > first->m_time) {
		return -1;
	}

	return 0;
}

// Appends the honor breakdown of the mission to p_text and returns the honor it earns.
// Not 100%: the stack slots of the locals are permuted.
// FUNCTION: MW2SHELL 0x100010ed
MechS32 AppendHonorBreakdown(
	DifficultyConfig* p_difficulty,
	CareerRecord* p_career,
	MissionResults* p_results,
	MechChar* p_text
)
{
	MechS32 honor = 0;
	MissionObjective* objective = NULL;
	MechS32 points = 5000;
	MechS32 i;
	MechS32 width;
	MechS32 secondary;
	MechS32 tertiary;
	CustomStar* star;
	MechS32 tons;
	MechDouble hit;
	MechS32 hitBonus;
	MechDouble multiplier;
	MechS32 bonus;

	for (i = 0; i < p_results->m_objectiveCount; i++) {
		objective = &p_results->m_objectives[i];
		if (objective->m_type == 1 && objective->m_status == 0) {
			points = 0;
		}
	}

	sprintf(g_honorPoints, "%d", points);
	width = g_bodyFont->GetTextWidth(g_honorPoints);
	sprintf(g_honorLine, "\\nMission Completion:\\g%03d\\b%03d%s\\n", 350, width, g_honorPoints);
	strcat(p_text, g_honorLine);
	honor += points;

	secondary = 0;
	tertiary = 0;
	for (i = 0; i < p_results->m_objectiveCount; i++) {
		objective = &p_results->m_objectives[i];
		switch (objective->m_type) {
		case 2:
			if (objective->m_status == 1) {
				secondary++;
			}
			break;
		case 4:
			if (objective->m_status == 1) {
				tertiary++;
			}
			break;
		default:
			break;
		}
	}

	if (secondary > 0) {
		points = secondary * 1500;
		honor += points;
		sprintf(g_honorPoints, "%d", points);
		width = g_bodyFont->GetTextWidth(g_honorPoints);
		sprintf(
			g_honorLine,
			"Secondary Objective Completed:\\t\\t%d\\t(x%d)\\g%03d\\b%03d%s\\n",
			1500,
			secondary,
			350,
			width,
			g_honorPoints
		);
		strcat(p_text, g_honorLine);
	}

	if (tertiary > 0) {
		points = tertiary * 500;
		honor += points;
		sprintf(g_honorPoints, "%d", points);
		width = g_bodyFont->GetTextWidth(g_honorPoints);
		sprintf(
			g_honorLine,
			"Tertiary Objective Completed:\\t\\t%d\\t(x%d)\\g%03d\\b%03d%s\\n",
			500,
			tertiary,
			350,
			width,
			g_honorPoints
		);
		strcat(p_text, g_honorLine);
	}

	if (p_career->m_wingmenLost > 0 && p_results->m_outcome == 2) {
		points = p_career->m_wingmenLost * -4000;
		honor += points;
		sprintf(g_honorPoints, "%d", points);
		width = g_bodyFont->GetTextWidth(g_honorPoints);
		sprintf(
			g_honorLine,
			"Wingman Deaths:\\t\\t\\t\\t%d\\t(x%d)\\g%03d\\b%03d%s\\n",
			-4000,
			p_career->m_wingmenLost,
			350,
			width,
			g_honorPoints
		);
		strcat(p_text, g_honorLine);
	}

	sprintf(g_honorLine, "\\t\\t\\t\\t\\t\\tdirect\\ttotal\\n");
	strcat(p_text, g_honorLine);

	points = p_career->m_mechKills * 250;
	g_currentPilot->m_kills += p_career->m_mechKills;
	sprintf(g_honorPoints, "%d", points);
	width = g_bodyFont->GetTextWidth(g_honorPoints);
	honor += points;
	sprintf(
		g_honorLine,
		"Enemy Mechs Destroyed:\\t\\t\\t%d\\t%d\\g%03d\\b%03d%s\\n",
		p_career->m_directMechKills,
		p_career->m_mechKills,
		350,
		width,
		g_honorPoints
	);
	strcat(p_text, g_honorLine);

	points = p_career->m_vehicleKills * 125;
	g_currentPilot->m_kills += p_career->m_directVehicleKills;
	sprintf(g_honorPoints, "%d", points);
	width = g_bodyFont->GetTextWidth(g_honorPoints);
	honor += points;
	sprintf(
		g_honorLine,
		"Enemy Vehicles Destoyed:\\t\\t\\t%d\\t%d\\g%03d\\b%03d%s\\n",
		p_career->m_directVehicleKills,
		p_career->m_vehicleKills,
		350,
		width,
		g_honorPoints
	);
	strcat(p_text, g_honorLine);

	star = GetStar(0);
	tons = star->m_size * star->m_tonnage;
	for (i = 0; i < star->m_count; i++) {
		tons -= g_mechChassis[star->m_mechs[i].m_chassis].m_tonnage;
	}

	if (tons > 0) {
		points = tons * 25;
		sprintf(g_honorPoints, "%d", points);
		width = g_bodyFont->GetTextWidth(g_honorPoints);
		honor += points;
		sprintf(
			g_honorLine,
			"Star Underweight Bonus:\\t\\t\\t%d\\t(x%d tons)\\g%03d\\b%03d%s\\n",
			25,
			tons,
			350,
			width,
			g_honorPoints
		);
		strcat(p_text, g_honorLine);
	}

	if (p_career->m_shotsFired == 0) {
		hit = 0.0;
	}
	else {
		hit = (MechDouble) (MechU32) p_career->m_hits / (MechDouble) (MechU32) p_career->m_shotsFired;
	}

	if (hit >= 0.7) {
		hitBonus = 750;
	}
	else {
		hitBonus = 0;
	}

	if (hit <= 1.0) {
		sprintf(g_honorPoints, "%d", hitBonus);
		width = g_bodyFont->GetTextWidth(g_honorPoints);
		sprintf(
			g_honorLine,
			"Hit Percentage:\\t\\t\\t\\t%3.1f\\g%03d\\b%03d%s\\n",
			hit * 100.0,
			350,
			width,
			g_honorPoints
		);
		strcat(p_text, g_honorLine);
		honor += hitBonus;
	}

	g_currentPilot->m_hits += p_career->m_hits;
	g_currentPilot->m_shotsFired += p_career->m_shotsFired;

	switch (p_difficulty->m_enemySkill) {
	case 0:
		multiplier = 0.8;
		strcpy(g_skillName, "EASY");
		break;
	case 1:
		multiplier = 1.0;
		strcpy(g_skillName, "MEDIUM");
		break;
	case 2:
		multiplier = 1.3;
		strcpy(g_skillName, "HARD");
		break;
	}

	if (!p_difficulty->m_heatTracking) {
		strcpy(g_honorLine, "\\n\\cThe Keshik deems it Dishonorable to Alter Heat Tracking\\n");
		strcat(p_text, g_honorLine);
		honor = 0;
	}
	else if (p_results->m_outcome == 2) {
		sprintf(g_honorPoints, "%d", honor);
		width = g_bodyFont->GetTextWidth(g_honorPoints);
		sprintf(g_honorLine, "\\nMission Honor:\\g%03d\\b%03d%s\\n", 350, width, g_honorPoints);
		strcat(p_text, g_honorLine);

		bonus = (MechS32) (honor * multiplier) - honor;
		honor += bonus;
		sprintf(g_honorPoints, "%d", honor);
		width = g_bodyFont->GetTextWidth(g_honorPoints);
		sprintf(
			g_honorLine,
			"Difficulty Multiplier:\\t(%s = %1.1f)\\g%03d\\b%03d%s\\n",
			g_skillName,
			multiplier,
			350,
			width,
			g_honorPoints
		);
		strcat(p_text, g_honorLine);
	}

	if (HasEasyOptions() == 1) {
		strcpy(g_honorLine, "\\n\\cNo Career Advancement with Altered Reality Enabled\\n");
		strcat(p_text, g_honorLine);
		honor = 0;
	}

	if (p_results->m_outcome == 3) {
		strcpy(g_honorLine, "\\n\\cMission Failed:  NO HONOR ACQUIRED\\n");
		strcat(p_text, g_honorLine);
		honor = 0;
	}

	return honor;
}

// Copies p_src to p_dst with every run of spaces and control characters turned into one space.
// FUNCTION: MW2SHELL 0x10001c28
void CollapseWhitespace(MechChar* p_dst, MechChar* p_src)
{
	MechS32 space = FALSE;

	while (*p_src) {
		if (*p_src <= ' ') {
			if (!space) {
				*p_dst++ = ' ';
				space = TRUE;
			}
		}
		else {
			space = FALSE;
			*p_dst++ = *p_src;
		}
		p_src++;
	}
	*p_dst = '\0';
}

// Builds the debriefing's text in p_text: the objectives, sorted by time, and the honor
// breakdown, whose honor a completed mission adds to the pilot's.
// Not 100%: the stack slots of i, seconds, minutes, honor and width are permuted.
// FUNCTION: MW2SHELL 0x10001ca0
void BuildDebriefText(
	CareerRecord* p_career,
	MissionResults* p_results,
	MechChar* p_text,
	DifficultyConfig* p_difficulty
)
{
	MechS32 i;
	MechS32 time;
	MechS32 seconds;
	MechS32 minutes;
	MechS32 honor;
	MechS32 width;

	strcpy(p_text, "");

	g_unk0x10077fe0[0] = 0xff;
	for (i = 1; i < 0x100; i++) {
		g_unk0x10077fe0[i] = i;
	}

	for (i = 0; i < p_results->m_objectiveCount; i++) {
		g_sortedObjectives[i] = &p_results->m_objectives[i];
	}
	qsort(g_sortedObjectives, p_results->m_objectiveCount, sizeof(MissionObjective*), CompareObjectives);

	strcat(p_text, "Time\\g050Type\\g170Objective\\g370Status\\n\\n");
	for (i = 0; i < p_results->m_objectiveCount; i++) {
		switch (g_sortedObjectives[i]->m_type) {
		case 0:
			strcpy(g_objectiveType, "Default Objective");
			break;
		case 1:
			strcpy(g_objectiveType, "Primary Objective");
			break;
		case 2:
			strcpy(g_objectiveType, "Secondary Objective");
			break;
		case 4:
			strcpy(g_objectiveType, "Tertiary Objective");
			break;
		case 8:
			strcpy(g_objectiveType, "Return Objective");
			break;
		default:
			strcpy(g_objectiveType, "Unknown");
			break;
		}

		switch (g_sortedObjectives[i]->m_status) {
		case 1:
			strcpy(g_objectiveStatus, "Successful");
			break;
		case 0:
			strcpy(g_objectiveStatus, "Failed");
			break;
		default:
			strcpy(g_objectiveStatus, "Unknown");
			break;
		}

		CollapseWhitespace(g_objectiveDescription, g_sortedObjectives[i]->m_description);

		if (g_sortedObjectives[i]->m_time < 0) {
			strcpy(g_objectiveTime, "DNF");
		}
		else {
			time = g_sortedObjectives[i]->m_time;
			seconds = time % 60;
			time /= 60;
			minutes = time % 60;
			time /= 60;
			time %= 24;
			sprintf(g_objectiveTime, "%02d:%02d", minutes, seconds);
		}

		sprintf(
			g_objectiveLine,
			"%s\\g050%s\\g170%s\\g370%s\\n",
			g_objectiveTime,
			g_objectiveType,
			g_objectiveDescription,
			g_objectiveStatus
		);
		strcat(p_text, g_objectiveLine);
	}

	honor = AppendHonorBreakdown(p_difficulty, p_career, p_results, p_text);
	if (p_results->m_outcome == 2) {
		g_currentPilot->m_honor += honor;
	}

	sprintf(g_careerHonor, "%d", g_currentPilot->m_honor);
	width = g_bodyFont->GetTextWidth(g_careerHonor);
	sprintf(g_careerHonorLine, "\\nCareer Honor:\\g%03d\\b%03d%s\\n", 350, width, g_careerHonor);
	strcat(p_text, g_careerHonorLine);
}

// Reads the simulator's mission results.
// ReadMissionResults on the Rust side (src/shell/screens/debug.rs) wraps it, for the debug menu's
// outcome override.
// FUNCTION: MW2SHELL 0x100021a6
void ReadMissionResultsC(void* p_results)
{
	FILE* file = NULL;

	file = MechFopen("MW2MSN.CFG", "rb");
	if (file == NULL) {
		return;
	}

	fread(p_results, 0x9d4, 1, file);
	fclose(file);
}

// Lays out the debriefing's text on pages, under the scenario's debriefing project (its first four
// letters and DBFS after a completed mission, DBFF otherwise; a pilot of the highest rank gets
// the clan's own).
// FUNCTION: MW2SHELL 0x10002207
void LayoutDebriefPages(
	MissionResults* p_results,
	MechChar* p_name,
	ButtonMenu*,
	MechS32 p_left,
	MechS32 p_top,
	MechS32 p_width,
	MechS32 p_height,
	Collection* p_pages,
	MechChar* p_scenario,
	MechChar* p_text,
	MechS32 p_campaign
)
{
	MechS32 i;

	for (i = 0; i < 4 && p_scenario[i] && p_scenario[i] != '.'; i++) {
		p_name[i] = p_scenario[i];
	}
	while (i < 4) {
		p_name[i] = '_';
		i++;
	}
	p_name[i] = '\0';

	switch (p_results->m_outcome) {
	case 2:
		if (g_currentPilot->m_rank >= 8) {
			switch (p_campaign) {
			case 0:
				strcpy(p_name, "KTWO");
				break;
			case 1:
				strcpy(p_name, "KTJF");
				break;
			}
		}
		strcat(p_name, "DBFS");
		break;
	default:
		strcat(p_name, "DBFF");
		break;
	}

	UppercaseString(p_name);
	LoadTextPages(p_pages, p_left, p_top, p_width, p_height, p_name, g_bodyFont, p_text);
}

// Returns the rank a trial earns: its successful primary objectives, when a clan pilot has
// completed a trial without the options that make it easier (p_difficulty, g_difficultyConfig).
// Not 100%: the stack slots of count and i are permuted.
// FUNCTION: MW2SHELL 0x100023bf
MechS32 GetTrialRank(MissionResults* p_results, DifficultyConfig* p_difficulty)
{
	MechS32 count;
	MechS32 i;

	if (g_currentPilot->m_clan != 2 &&
		g_campaignMissions[g_currentPilot->m_clan][g_currentPilot->m_mission].m_trial == 1 && !HasEasyOptions() &&
		p_results->m_outcome == 2 && p_difficulty->m_heatTracking == 1) {
		count = 0;
		for (i = 0; i < p_results->m_objectiveCount; i++) {
			if (p_results->m_objectives[i].m_type == 1 && p_results->m_objectives[i].m_status == 1) {
				count++;
			}
		}

		return count;
	}

	return 0;
}

// Opens the debriefing screen: reads the mission's results and statistics, scores them for the
// pilot and lays the text out on pages under the menu.
// Not 100%: the stack slots of file, left, top, width, height, career and name are permuted.
// FUNCTION: MW2SHELL 0x100024a3
void DrawMissionDebrief(TMPackDataBase* p_database, MechS32 p_campaign, char** p_scenario)
{
	FILE* file = NULL;
	MechS32 left;
	MechS32 top;
	MechS32 width;
	MechS32 height;
	CareerRecord career;
	MechChar name[0x10];

	g_videoDriver->LoadBackground(p_database, g_debriefScreens[p_campaign].m_picture);
	switch (p_campaign) {
	case 0:
		left = 0x58;
		top = 0x1e;
		width = 0x1c6;
		height = 430 - top;
		break;
	case 1:
		left = 0x62;
		top = 0x33;
		width = 0x193;
		height = 423 - top;
		break;
	default:
		left = 0x58;
		top = 0x1e;
		width = 0x1c6;
		height = 438 - top;
		break;
	}

	LoadPilotRoster();
	file = MechFopen("MW2CAR.CFG", "rb");
	if (file) {
		fread(&career, 0x50, 1, file);
		fclose(file);
	}
	ReadMissionResults(&g_missionResults);

	CreateCollection(&g_debriefPages, 10, NULL, 4, NULL);
	g_keyboardInput->FlushKeys();
	g_debriefMenu = new ButtonMenu(
		g_videoDriver,
		g_defaultFont,
		FALSE,
		g_debriefScreens[p_campaign].m_buttons,
		g_debriefScreens[p_campaign].m_count
	);

	if (p_campaign != 2) {
		g_pilotBeforeMission = *g_currentPilot;
	}
	BuildDebriefText(&career, &g_missionResults, g_debriefText, &g_difficultyConfig);

	if (p_campaign != 2) {
		g_currentPilot->m_rank += GetTrialRank(&g_missionResults, &g_difficultyConfig);
		if (g_currentPilot->m_rank >= 8) {
			g_currentPilot->m_rank = 8;
		}
		if (g_missionResults.m_outcome == 2 && !HasEasyOptions()) {
			g_currentPilot->m_mission++;
		}
		SavePilotRoster();
		LayoutDebriefPages(
			&g_missionResults,
			name,
			g_debriefMenu,
			left,
			top,
			width,
			height,
			g_debriefPages,
			*p_scenario,
			g_debriefText,
			p_campaign
		);
	}

	g_debriefPage = (Page*) CollectionGet(g_debriefPages, 0);
	if (!g_debriefPage) {
		g_debriefPage = new Page(g_bodyFont, g_videoDriver, NULL, 0, 0, 100, 100);
	}
	else {
		CollectionRemove(g_debriefPages, g_debriefPage, FALSE);
	}

	if (!g_debriefPages->m_count || g_missionResults.m_outcome != 2) {
		g_debriefMenu->DisableButton(1);
	}
	g_debriefPage->Restart();
	RegisterScreenFunction(MissionDebriefCallback);
}

// The debriefing screen's frame: EXIT (on to the next mission once one is completed),
// AFTERMATH (the text in a reader) and REPLAY (restores the pilot).
// FUNCTION: MW2SHELL 0x1000287f
void MissionDebriefCallback(TMPackDataBase*, MechS32* p_campaign, MechU8*, MechChar** p_scenario, MechS32 p_msg)
{
	MechS32 result;
	MechS32 button;

	// The original skips the frame's work with a goto, like StarConfigCallback.
	if (p_msg != c_msgScreenFrame) {
		goto done;
	}

	if (!g_aftermathReader) {
		g_debriefPage->TypeStep();
		button = g_debriefMenu->HitTest(g_mouseState->m_x, g_mouseState->m_y);
		switch (button) {
		case 0:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			p_msg = c_msgReadyRoom;
			if (*p_campaign != 2 && g_missionResults.m_outcome == 2) {
				if (g_currentPilot->m_mission >= 16) {
					p_msg = c_msgEndingVideo;
				}
				else {
					*p_scenario = g_campaignMissions[*p_campaign][g_currentPilot->m_mission].m_scenario;
				}
			}
			break;
		case 1:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			g_debriefPage->Hide();
			delete g_debriefMenu;
			g_aftermathReader = new ArchiveReader(
				"",
				g_archiveFont,
				-1,
				FALSE,
				NULL,
				g_debriefPages,
				g_aftermathScreens[*p_campaign].m_buttons,
				g_aftermathScreens[*p_campaign].m_count
			);
			break;
		case 2:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			if (g_missionResults.m_outcome == 2) {
				if (!ShowDialog("Are you Sure?#Yes|No", 1)) {
					*g_currentPilot = g_pilotBeforeMission;
					SavePilotRoster();
					p_msg = c_msgBriefing;
				}
				else {
					UpdateVideos();
				}
			}
			else {
				p_msg = c_msgBriefing;
			}
			break;
		default:
			break;
		}
	}
	else {
		result = g_aftermathReader->Run();
		if (result == c_msgQuit) {
			p_msg = c_msgQuit;
		}
		if (result == c_msgMainMenu) {
			p_msg = c_msgMainMenu;
		}
		if (result != c_msgArchive) {
			delete g_aftermathReader;
			g_aftermathReader = NULL;
			g_debriefMenu = new ButtonMenu(
				g_videoDriver,
				g_defaultFont,
				FALSE,
				g_debriefScreens[*p_campaign].m_buttons,
				g_debriefScreens[*p_campaign].m_count
			);
			g_debriefPage->Restart();
		}
	}

done:
	if (p_msg != c_msgScreenFrame) {
		delete g_debriefPage;
		delete g_debriefMenu;
		if (g_aftermathReader) {
			delete g_aftermathReader;
		}
		g_aftermathReader = NULL;
		g_videoDriver->ClearGlyphs(TRUE);
		MechPostMessage(p_msg, c_msgDebrief, 0);
		UnregisterScreenFunction(MissionDebriefCallback);
	}
}
