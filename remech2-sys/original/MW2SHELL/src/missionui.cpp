#include "missionui.h"

#include "audiosample.h"
#include "buttonmenu.h"
#include "customstar.h"
#include "decomp.h"
#include "font.h"
#include "formation.h"
#include "mainmenubutton.h"
#include "mechbay.h"
#include "mechchassis.h"
#include "mechvariant.h"
#include "menudata.h"
#include "messages.h"
#include "mousestate.h"
#include "projectarchive.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "simhandoff.h"
#include "simhandoffstate.h"
#include "textglyph.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <string.h>

// The mission briefing screen: the mission's stars and their mechs, read from the mission's
// BWD file, and its briefing text and videos.

// SIZE 0x20
// A star in a mission's BWD file (node type 0x46). Its mechs' variant names follow, one per
// mech of the star's size.
struct BwdStar {
	MechS32 m_difficulty;         // 0x00 — the enemy star's, see g_enemyStarDifficulty
	MechS32 m_tonnage;            // 0x04
	MechS32 m_count;              // 0x08
	MechS32 m_size;               // 0x0c
	MechChar m_variants[1][0x10]; // 0x10
};

DECOMP_SIZE_ASSERT(MechNameTag, 0x08)
DECOMP_SIZE_ASSERT(Formation, 0x08)

void MissionBriefingCallback(TMPackDataBase*, MechS32*, MechU8*, MechChar**, MechS32);

// The briefing videos of the missions, each with its looping continuation.
// GLOBAL: MW2SHELL 0x1006a1c0
MechChar* g_briefingVideos[12][2] = {
	{"aplan01", "aplan01c"},
	{"aplan02", "aplan02c"},
	{"aplan03", "aplan03c"},
	{"aplan04", "aplan04c"},
	{"aplan05", "aplan05c"},
	{"aplan06", "aplan06c"},
	{"aplan07", "aplan07c"},
	{"aplan08", "aplan08c"},
	{"aplan09", "aplan09c"},
	{"aplan10", "aplan10c"},
	{"aplan11", "aplan11c"},
	{"aplan12", "aplan12c"},
};

// The missions of the Trials of Grievance, in order; the list ends at the first NULL.
// GLOBAL: MW2SHELL 0x1006a220
MechChar* g_briefingScenarios[12] = {
	"jackscn1",
	"chedscn1",
	"edamscn1",
	"provscn1",
	"goudscn1",
	"colbscn1",
	"goatscn1",
	"whizscn1",
	"ricoscn1",
	"swisscn1",
};

// The clans' videos, by the index into g_clanNames.
// GLOBAL: MW2SHELL 0x1006a250
MechChar* g_clanVideos[6] = {"wiawolf", "wiajf", "wiaghost", "wiasmoke", "wianova", "wiasteel"};

// The names of the mechs of the player's star...
// GLOBAL: MW2SHELL 0x1006a268
MechNameTag g_playerMechTags[3] = {0};

// ...and of the enemy's.
// GLOBAL: MW2SHELL 0x1006a280
MechNameTag g_enemyMechTags[3] = {0};

// GLOBAL: MW2SHELL 0x1006a298
MechS32 g_briefingChassisCount = 0xf;

// The briefing video, an index into g_briefingVideos.
// GLOBAL: MW2SHELL 0x1006a29c
MechS32 g_briefingVideo = 0;

// The formation names of the enemy's star...
// GLOBAL: MW2SHELL 0x1006a2a0
TextGlyph* g_enemyFormationName = NULL;

// ...and of the player's.
// GLOBAL: MW2SHELL 0x1006a2a4
TextGlyph* g_playerFormationName = NULL;

// GLOBAL: MW2SHELL 0x1006a2a8
ButtonMenu* g_missionBriefingMenu = NULL;

// Played on LAUNCH.
// GLOBAL: MW2SHELL 0x1006a2b0
AudioSample* g_launchSound = NULL;

// Only for a trial (WM_USER + 0xe).
// GLOBAL: MW2SHELL 0x1006a2b4
AudioSample* g_trialSound = NULL;

// The text colors: each color maps to itself, but 0 is transparent and 1 is drawn in 0x22.
// GLOBAL: MW2SHELL 0x10090058
MechU8 g_briefingTextColors[0x100];

// The player's clan and the rival clan, indices into g_clanNames and g_clanVideos. They are
// never the same.
// GLOBAL: MW2SHELL 0x10090170
MechS32 g_briefingClan;

// The three lines of briefing text.
// GLOBAL: MW2SHELL 0x10090160
TextGlyph* g_briefingLines[3];

// The formations of the player's star and the enemy's, indices into g_formations.
// GLOBAL: MW2SHELL 0x1009016c
MechS32 g_playerFormation;

// GLOBAL: MW2SHELL 0x10090158
MechS32 g_briefingRival;

// DrawMissionBriefing's p_wParam: WM_USER + 0xe for a trial.
// GLOBAL: MW2SHELL 0x10090174
size_t g_briefingMessage;

// GLOBAL: MW2SHELL 0x10090178
MechS32 g_enemyFormation;

// The briefing text line being built.
// GLOBAL: MW2SHELL 0x10090180
MechChar g_briefingLine[0x100];

// The mission, an index into g_briefingScenarios.
// GLOBAL: MW2SHELL 0x10090280
MechS32 g_briefingMission;

// Sets up a star from a BWD star node, and registers its mechs' variants.
// FUNCTION: MW2SHELL 0x10037c60
void LoadStar(MechS32 p_star, BwdStar* p_data)
{
	MechS32 i;

	SelectStar(p_star, 0, p_data->m_size, p_data->m_count, p_data->m_tonnage);
	for (i = 0; i < p_data->m_size; i++) {
		SetStarMech(i, p_data->m_variants[i], NULL);
	}
	SelectStar(p_star, 0, p_data->m_size, p_data->m_count, p_data->m_tonnage);
}

// Reads the mission's BWD file (the scenario's first four letters + "brf2"): its briefing
// name, optionally its two stars (p_stars) and its briefing video and text (p_video).
// Not 100%: the stack slots of name, file, title, star and the loop locals are permuted.
// FUNCTION: MW2SHELL 0x10037cf7
void ShellApplyMissionUiInfo(MechChar* p_scenario, MechS32 p_stars, MechS32 p_video)
{
	MechS32* briefing;
	MechChar name[16];
	MechS32* star;
	MechS32* file;
	MechS32 i;
	MechS32 pos;
	MechS32 j;
	MechS32* title;

	strcpy(name, p_scenario);
	name[4] = '\0';
	strcat(name, "brf2");

	file = (MechS32*) g_projectArchive->GetResourceByName(name, 0xe, "BWD");
	if (!file) {
		return;
	}

	title = g_projectArchive->FindNextBwdNode(file, 0x47, NULL);
	if (title) {
		strcpy(g_missionName, (MechChar*) (title + 2));
	}

	if (p_stars) {
		star = g_projectArchive->FindNextBwdNode(file, 0x46, NULL);
		if (star) {
			LoadStar(0, (BwdStar*) (star + 2));
		}

		star = g_projectArchive->FindNextBwdNode(file, 0x46, star);
		if (star) {
			LoadStar(1, (BwdStar*) (star + 2));
			g_enemyStarDifficulty = star[2];
		}
	}

	SelectStar(0, -1, -1, -1, -1);
	SetStarMech(0, NULL, NULL);

	if (p_video) {
		briefing = g_projectArchive->FindNextBwdNode(file, 0x45, NULL);
		if (briefing) {
			g_briefingVideo = briefing[2] - 1;
			if (g_briefingVideo < 0 || g_briefingVideo >= 12) {
				g_briefingVideo = 0;
			}

			PlayVideo(0, g_briefingVideos[g_briefingVideo][0], 0x19e, 10, 0x42, 0);

			pos = 0;
			for (i = 0; i < 3; i++, pos++) {
				for (j = 0; pos < briefing[1] - 0xc && ((MechChar*) briefing)[pos + 0xc] >= ' '; pos++, j++) {
					g_briefingLine[j] = ((MechChar*) briefing)[pos + 0xc];
				}
				g_briefingLine[j] = '\0';

				if (g_briefingLines[i]) {
					delete g_briefingLines[i];
				}
				g_briefingLines[i] = g_textFont->AddText(0xef, i * 12 + 0x45, g_briefingLine, g_briefingTextColors);
			}
		}
	}

	g_projectArchive->ReleaseResourceByName(name, 0xe, "BWD");
}

// Shows the name of mech type p_type at (p_left, p_top), replacing the tag's previous glyph.
// Not 100%: the stack slot of text is permuted with the delete temporaries.
// FUNCTION: MW2SHELL 0x10037feb
void ShowMechName(MechNameTag* p_tag, MechS32 p_type, MechS32 p_left, MechS32 p_top)
{
	MechChar* text;

	p_tag->m_type = p_type;
	if (p_tag->m_glyph) {
		delete p_tag->m_glyph;
	}

	if (p_type >= 0) {
		text = g_mechChassis[p_tag->m_type].m_name;
	}
	else {
		text = "[none]";
	}

	p_tag->m_glyph = g_textFont->AddText(p_left, p_top, text, g_briefingTextColors);
}

// Shows the formation names of both stars.
// FUNCTION: MW2SHELL 0x10038093
void ShowFormationNames()
{
	MechPoint* pos;

	g_playerFormation = GetStarFormation(0);
	if (g_playerFormationName) {
		delete g_playerFormationName;
	}
	pos = &g_missionBriefingButtons[6].m_textPos;
	g_playerFormationName =
		g_textFont->AddText(pos->x, pos->y, g_formations[g_playerFormation].m_name, g_briefingTextColors);

	g_enemyFormation = GetStarFormation(1);
	if (g_enemyFormationName) {
		delete g_enemyFormationName;
	}
	pos = &g_missionBriefingButtons[17].m_textPos;
	g_enemyFormationName =
		g_textFont->AddText(pos->x, pos->y, g_formations[g_enemyFormation].m_name, g_briefingTextColors);
}

// Picks a value by the pilot of the player's first mech: 0x12 for Enzo, 0x11 for Hobbes, 0x10
// for Calvin and 0xf for anyone else.
// FUNCTION: MW2SHELL 0x100381c2
MechS32 GetChassisCount()
{
	CustomStar* star;

	star = GetStar(0);
	if (!strcmp(star->m_mechs[0].m_pilot, "Enzo")) {
		return 0x12;
	}
	if (!strcmp(star->m_mechs[0].m_pilot, "Hobbes")) {
		return 0x11;
	}
	if (!strcmp(star->m_mechs[0].m_pilot, "Calvin")) {
		return 0x10;
	}

	return 0xf;
}

// Sets up the mission briefing screen. p_wParam is WM_USER + 0xe for a trial.
// Not 100%: the stack slots of pos, audioData, i and audioSize are permuted.
// FUNCTION: MW2SHELL 0x100382e6
void DrawMissionBriefing(TMPackDataBase* p_database, MechChar** p_scenario, size_t p_wParam)
{
	MechPoint* pos;
	void* audioData = NULL;
	MechS32 i;
	MechS32 audioSize;

	g_briefingMessage = p_wParam;
	g_briefingChassisCount = GetChassisCount();

	for (i = 1; i < 0x100; i++) {
		g_briefingTextColors[i] = i;
	}
	g_briefingTextColors[0] = 0xff;
	g_briefingTextColors[1] = 0x22;

	*p_scenario = "pinkscn1";
	p_database->GetDBItem(78, &audioData, &audioSize);
	g_launchSound = new AudioSample(g_audioSubsystem, audioData, audioSize);
	if (p_wParam == c_msgMainMenu) {
		p_database->GetDBItem(82, &audioData, &audioSize);
		g_trialSound = new AudioSample(g_audioSubsystem, audioData, audioSize);
	}

	g_missionBriefingMenu = new ButtonMenu(g_videoDriver, g_defaultFont, 0, g_missionBriefingButtons, 0x19);
	for (i = 0; i < 3; i++) {
		g_briefingLines[i] = NULL;
	}

	if (p_wParam == c_msgMainMenu) {
		g_simHandoff.m_briefingMission = 0;
	}
	g_briefingMission = g_simHandoff.m_briefingMission;
	g_briefingVideo = 0;
	*p_scenario = g_briefingScenarios[g_briefingMission];
	ShellApplyMissionUiInfo(g_briefingScenarios[g_briefingMission], p_wParam == c_msgMainMenu, 1);

	g_briefingClan = 0;
	PlayVideo(1, g_clanVideos[g_briefingClan], 0xd, 0xcd, 6, 0);
	g_briefingRival = 1;
	PlayVideo(2, g_clanVideos[g_briefingRival], 0x1e3, 0x149, 6, 0);

	for (i = 0; i < 3; i++) {
		g_playerMechTags[i].m_glyph = NULL;
		pos = &g_missionBriefingButtons[i + 3].m_textPos;
		SelectStar(0, -1, -1, -1, -1);
		ShowMechName(&g_playerMechTags[i], GetStarMechChassis(i), pos->x, pos->y);

		g_enemyMechTags[i].m_glyph = NULL;
		pos = &g_missionBriefingButtons[i + 0xe].m_textPos;
		SelectStar(1, -1, -1, -1, -1);
		ShowMechName(&g_enemyMechTags[i], GetStarMechChassis(i), pos->x, pos->y);
	}
	SelectStar(0, -1, -1, -1, -1);

	g_enemyFormationName = NULL;
	g_playerFormationName = NULL;
	ShowFormationNames();

	if (g_trialSound) {
		g_trialSound->SetVolume(0x1e);
		g_trialSound->Start();
	}

	PlayVideo(0x10, "wialanch", 0xd1, 0x173, 0x24, 0);
	RegisterScreenFunction(MissionBriefingCallback);
	g_videoDriver->LoadBackground(p_database, 9);
	UpdateVideos();
	g_videoDriver->DrawShell();
}

// The mission briefing's frame: LAUNCH, EXIT, the next mission, both clans' videos, the mechs
// and formations of both stars (next and previous), and the two stars' accept and config.
// Not 100%: the stack slots of pos, variant, i and button are permuted.
// FUNCTION: MW2SHELL 0x10038744
void MissionBriefingCallback(TMPackDataBase*, MechS32*, MechU8*, MechChar** p_scenario, MechS32 p_msg)
{
	MechPoint* pos;
	MechChar* variant;
	MechS32 i;
	MechS32 button;

	// The original skips the frame's work with a goto, like StarConfigCallback.
	if (p_msg != c_msgScreenFrame) {
		goto done;
	}

	button = g_missionBriefingMenu->HitTest(g_mouseState->m_x, g_mouseState->m_y);
	switch (button) {
	case 0:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		ShowVideo(0x10);
		UpdateVideos();
		g_launchSound->PlayAndWait();
		PrjBuildPlayerStarTemplates(g_briefingClan, g_briefingRival);
		p_msg = c_msgLaunchSim;
		break;
	case 1:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		p_msg = c_msgMainMenu;
		break;
	case 2:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_briefingMission++;
		if (!g_briefingScenarios[g_briefingMission]) {
			g_briefingMission = 0;
		}
		*p_scenario = g_briefingScenarios[g_briefingMission];
		ShellApplyMissionUiInfo(g_briefingScenarios[g_briefingMission], 1, 1);
		for (i = 0; i < 3; i++) {
			pos = &g_missionBriefingButtons[i + 3].m_textPos;
			SelectStar(0, -1, -1, -1, -1);
			ShowMechName(&g_playerMechTags[i], GetStarMechChassis(i), pos->x, pos->y);

			pos = &g_missionBriefingButtons[i + 0xe].m_textPos;
			SelectStar(1, -1, -1, -1, -1);
			ShowMechName(&g_enemyMechTags[i], GetStarMechChassis(i), pos->x, pos->y);
		}
		SelectStar(0, -1, -1, -1, -1);
		ShowFormationNames();
		break;
	case 11:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_briefingClan++;
		if (g_briefingClan >= 6) {
			g_briefingClan = 0;
		}
		if (g_briefingRival == g_briefingClan) {
			g_briefingClan++;
		}
		if (g_briefingClan >= 6) {
			g_briefingClan = 0;
		}
		PlayVideo(1, g_clanVideos[g_briefingClan], 0xd, 0xcd, 6, 0);
		break;
	case 22:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_briefingRival++;
		if (g_briefingRival >= 6) {
			g_briefingRival = 0;
		}
		if (g_briefingRival == g_briefingClan) {
			g_briefingRival++;
		}
		if (g_briefingRival >= 6) {
			g_briefingRival = 0;
		}
		PlayVideo(2, g_clanVideos[g_briefingRival], 0x1e3, 0x149, 6, 0);
		break;
	case 3:
	case 4:
	case 5:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		pos = &g_missionBriefingButtons[button].m_textPos;
		i = button - 3;
		do {
			g_playerMechTags[i].m_type++;
			if (g_playerMechTags[i].m_type >= g_briefingChassisCount) {
				g_playerMechTags[i].m_type = -1;
				variant = "";
			}
			else {
				variant = g_mechChassis[g_playerMechTags[i].m_type].m_prefix;
			}
			SelectStar(0, -1, -1, -1, -1);
		} while (!SetStarMech(i, variant, NULL));
		ShowMechName(&g_playerMechTags[i], GetStarMechChassis(i), pos->x, pos->y);
		break;
	case 7:
	case 8:
	case 9:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		pos = &g_missionBriefingButtons[button - 4].m_textPos;
		i = button - 7;
		do {
			g_playerMechTags[i].m_type--;
			if (g_playerMechTags[i].m_type == -1) {
				variant = "";
			}
			else {
				if (g_playerMechTags[i].m_type == -2) {
					g_playerMechTags[i].m_type = g_briefingChassisCount - 1;
				}
				variant = g_mechChassis[g_playerMechTags[i].m_type].m_prefix;
			}
			SelectStar(0, -1, -1, -1, -1);
		} while (!SetStarMech(i, variant, NULL));
		ShowMechName(&g_playerMechTags[i], GetStarMechChassis(i), pos->x, pos->y);
		break;
	case 14:
	case 15:
	case 16:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		pos = &g_missionBriefingButtons[button].m_textPos;
		i = button - 14;
		do {
			g_enemyMechTags[i].m_type++;
			if (g_enemyMechTags[i].m_type >= g_briefingChassisCount) {
				g_enemyMechTags[i].m_type = -1;
				variant = "";
			}
			else {
				variant = g_mechChassis[g_enemyMechTags[i].m_type].m_prefix;
			}
			SelectStar(1, -1, -1, -1, -1);
		} while (!SetStarMech(i, variant, NULL));
		ShowMechName(&g_enemyMechTags[i], GetStarMechChassis(i), pos->x, pos->y);
		break;
	case 18:
	case 19:
	case 20:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		pos = &g_missionBriefingButtons[button - 4].m_textPos;
		i = button - 18;
		do {
			g_enemyMechTags[i].m_type--;
			if (g_enemyMechTags[i].m_type == -1) {
				variant = "";
			}
			else {
				if (g_enemyMechTags[i].m_type == -2) {
					g_enemyMechTags[i].m_type = g_briefingChassisCount - 1;
				}
				variant = g_mechChassis[g_enemyMechTags[i].m_type].m_prefix;
			}
			SelectStar(1, -1, -1, -1, -1);
		} while (!SetStarMech(i, variant, NULL));
		ShowMechName(&g_enemyMechTags[i], GetStarMechChassis(i), pos->x, pos->y);
		break;
	case 6:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_playerFormation++;
		if (g_playerFormation >= 6) {
			g_playerFormation = 0;
		}
		SelectStar(0, g_playerFormation, -1, -1, -1);
		ShowFormationNames();
		break;
	case 10:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		if (--g_playerFormation < 0) {
			g_playerFormation = 5;
		}
		SelectStar(0, g_playerFormation, -1, -1, -1);
		ShowFormationNames();
		break;
	case 17:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_enemyFormation++;
		if (g_enemyFormation >= 6) {
			g_enemyFormation = 0;
		}
		SelectStar(1, g_enemyFormation, -1, -1, -1);
		ShowFormationNames();
		break;
	case 21:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		if (--g_enemyFormation < 0) {
			g_enemyFormation = 5;
		}
		SelectStar(1, g_enemyFormation, -1, -1, -1);
		ShowFormationNames();
		break;
	case 13:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		p_msg = c_msgStarConfig;
		SelectStar(0, -1, -1, -1, -1);
		break;
	case 24:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		p_msg = c_msgStarConfig;
		SelectStar(1, -1, -1, -1, -1);
		break;
	case 12:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		p_msg = c_msgMechBay;
		SelectStar(0, -1, -1, -1, -1);
		break;
	case 23:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		p_msg = c_msgMechBay;
		SelectStar(1, -1, -1, -1, -1);
		break;
	default:
		break;
	}

	if (!IsVideoPlaying(0)) {
		PlayVideo(0, g_briefingVideos[g_briefingVideo][1], 0x19e, 10, 0x4a, 0);
	}

done:
	if (p_msg != c_msgScreenFrame) {
		CloseAllVideos();
		delete g_missionBriefingMenu;
		delete g_launchSound;
		if (g_trialSound) {
			delete g_trialSound;
			g_trialSound = NULL;
		}
		g_videoDriver->ClearGlyphs(TRUE);
		g_simHandoff.m_briefingMission = g_briefingMission;
		MechPostMessage(p_msg, c_msgTrials, 0);
		UnregisterScreenFunction(MissionBriefingCallback);
	}
}
