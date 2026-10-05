#include "mechvariant.h"

#include "audiosample.h"
#include "buttonmenu.h"
#include "campaignmission.h"
#include "customstar.h"
#include "decomp.h"
#include "font.h"
#include "keyboardinput.h"
#include "mechbay.h"
#include "mechchassis.h"
#include "menudata.h"
#include "menuscreen.h"
#include "messages.h"
#include "mousestate.h"
#include "pilotrecord.h"
#include "projectarchive.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "simhandoff.h"
#include "simhandoffstate.h"
#include "starmech.h"
#include "textglyph.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

// The two stars of a custom battle: the player's and the enemy's.

DECOMP_SIZE_ASSERT(StarMech, 0x24)
DECOMP_SIZE_ASSERT(CustomStar, 0x80)
DECOMP_SIZE_ASSERT(FormationOption, 0x08)
DECOMP_SIZE_ASSERT(FormationSlot, 0x10)
DECOMP_SIZE_ASSERT(FormationPositions, 0x30)
DECOMP_SIZE_ASSERT(MechChassis, 0x18)
DECOMP_SIZE_ASSERT(MenuScreen, 0x10)
DECOMP_SIZE_ASSERT(MainMenuButton, 0x1c)

MechS32 PlayVideo(
	MechS32 p_index,
	const char* p_name,
	undefined4 p_unk0x08,
	undefined4 p_unk0x0c,
	MechU32 p_unk0x10,
	MechU32 p_unk0x14
);

// The formations of the Wolf and Inner Sphere stars (Jade Falcon's differ): six formations of
// three mechs.
// GLOBAL: MW2SHELL 0x1005b4c0
FormationPositions g_formationLayouts[6] = {
	{{{2, 430, 346, 2}, {1, 333, 353, 1}, {0, 204, 359, 0}}},
	{{{2, 295, 330, 0}, {1, 332, 352, 1}, {0, 385, 395, 2}}},
	{{{2, 245, 341, 0}, {0, 343, 355, 1}, {1, 418, 363, 2}}},
	{{{2, 390, 332, 2}, {1, 350, 355, 1}, {0, 277, 374, 0}}},
	{{{0, 390, 332, 1}, {2, 204, 358, 0}, {1, 385, 395, 2}}},
	{{{2, 295, 330, 0}, {1, 430, 346, 2}, {0, 288, 376, 1}}},
};

// GLOBAL: MW2SHELL 0x1005b5e0
FormationPositions g_jadeFalconFormationLayouts[6] = {
	{{{2, 425, 345, 2}, {1, 325, 354, 1}, {0, 181, 362, 0}}},
	{{{2, 297, 329, 0}, {1, 325, 354, 1}, {0, 377, 408, 2}}},
	{{{2, 251, 341, 0}, {0, 330, 350, 1}, {1, 410, 366, 2}}},
	{{{2, 373, 338, 2}, {1, 338, 357, 1}, {0, 258, 385, 0}}},
	{{{0, 373, 338, 1}, {2, 181, 362, 0}, {1, 377, 408, 2}}},
	{{{2, 297, 329, 0}, {1, 425, 345, 2}, {0, 258, 385, 1}}},
};

// GLOBAL: MW2SHELL 0x1005b700
MechChar g_wolfStarVideo[] = "awosc%s";

// GLOBAL: MW2SHELL 0x1005b708
MechChar g_jadeFalconStarVideo[] = "ajfsc%s";

// GLOBAL: MW2SHELL 0x1005b710
MechChar g_trialStarVideo[] = "aiasc%s";

// GLOBAL: MW2SHELL 0x1005b718
CustomStar g_playerStar =
	{0, 0, 3, 3, 100, {{12, "tbr00std", "MechWarrior"}, {0, "drw00std", "Friend 1"}, {1, "frm00std", "Friend 2"}}};

// GLOBAL: MW2SHELL 0x1005b798
CustomStar g_enemyStar =
	{0, 0, 3, 3, 100, {{12, "tbr00std", "Enemy 1"}, {0, "drw00std", "Enemy 2"}, {1, "frm00std", "Enemy 3"}}};

// GLOBAL: MW2SHELL 0x1005b818
CustomStar* g_selectedStar = &g_playerStar;

// GLOBAL: MW2SHELL 0x1005b820
FormationOption g_formationOptions[6] = {
	{"~Echelon Left", "echelonl"},
	{"~Echelon Right", "echelonr"},
	{"~Line Abreast", "lineabreast"},
	{"~Line Astern", "lineastern"},
	{"~V-Form", "vform"},
	{"~Wedge", "wedge"},
};

// Where the name labels go, by position: left edges and tops, for the Wolf and Inner Sphere
// screens and for the Jade Falcon one.
// GLOBAL: MW2SHELL 0x1005b850
MechS32 g_starLabelLefts[4] = {81, 313, 508, 0};

// GLOBAL: MW2SHELL 0x1005b860
MechS32 g_starLabelTops[4] = {166, 133, 189, 0};

// GLOBAL: MW2SHELL 0x1005b870
MechS32 g_jadeFalconLabelLefts[4] = {47, 280, 495, 0};

// GLOBAL: MW2SHELL 0x1005b880
MechS32 g_jadeFalconLabelTops[3] = {172, 127, 217};

// The formation, mission, star size, tonnage limit and star mass lines.
// GLOBAL: MW2SHELL 0x1005b88c
TextGlyph* g_formationLine = NULL;

// GLOBAL: MW2SHELL 0x1005b890
TextGlyph* g_missionLine = NULL;

// GLOBAL: MW2SHELL 0x1005b894
TextGlyph* g_starSizeLine = NULL;

// GLOBAL: MW2SHELL 0x1005b898
TextGlyph* g_tonnageLine = NULL;

// GLOBAL: MW2SHELL 0x1005b89c
TextGlyph* g_starMassLine = NULL;

// Per position: the name, type and mass glyphs.
// GLOBAL: MW2SHELL 0x10079438
TextGlyph* g_positionGlyphs[3][3];

// The mass of each mech of the star.
// GLOBAL: MW2SHELL 0x10079460
MechS32 g_mechMasses[3];

// GLOBAL: MW2SHELL 0x1007946c
FormationPositions* g_formationLayout;

// GLOBAL: MW2SHELL 0x10079470
AudioSample* g_mechLabSound;

// GLOBAL: MW2SHELL 0x10079478
MechChar g_starInfoText[0x100];

// GLOBAL: MW2SHELL 0x10079578
MechS32* g_labelTops;

// GLOBAL: MW2SHELL 0x1007957c
MechS32* g_labelLefts;

// GLOBAL: MW2SHELL 0x10079580
AudioSample* g_starSound;

// GLOBAL: MW2SHELL 0x10079584
ButtonMenu* g_starMenu;

// GLOBAL: MW2SHELL 0x10079588
MechChar g_fitTextBuffer[0x100];

// GLOBAL: MW2SHELL 0x10079688
MechChar* g_starVideoFormat;

// Saves both stars and which one is selected in the mw2prm.cfg record, for RestoreStars to
// restore.
// FUNCTION: MW2SHELL 0x10002d30
void SaveStars()
{
	if (g_selectedStar == &g_playerStar) {
		g_simHandoff.m_playerStarSelected = TRUE;
	}
	else {
		g_simHandoff.m_playerStarSelected = FALSE;
	}

	g_simHandoff.m_playerStar = g_playerStar;
	g_simHandoff.m_enemyStar = g_enemyStar;
}

// FUNCTION: MW2SHELL 0x10002d8d
void RestoreStars()
{
	if (g_simHandoff.m_playerStarSelected) {
		g_selectedStar = &g_playerStar;
	}
	else {
		g_selectedStar = &g_enemyStar;
	}

	g_playerStar = g_simHandoff.m_playerStar;
	g_enemyStar = g_simHandoff.m_enemyStar;
}

// Sets a mech of the selected star: its pilot name and its variant file. A three-letter variant
// name is the mech's standard variant. Returns 0 when the mech is too heavy for the star.
// FUNCTION: MW2SHELL 0x10002de7
MechS32 SetStarMech(MechS32 p_index, MechChar* p_variant, MechChar* p_name)
{
	MechS32 type;

	if (p_index < 0) {
		p_index = g_selectedStar->m_selected;
	}
	else {
		g_selectedStar->m_selected = p_index;
	}

	if (p_name) {
		strcpy(g_selectedStar->m_mechs[p_index].m_pilot, p_name);
	}

	if (p_variant) {
		for (type = 0; g_mechChassis[type].m_prefix; type++) {
			if (!strncasecmp(p_variant, g_mechChassis[type].m_prefix, 3)) {
				break;
			}
		}

		if (!g_mechChassis[type].m_prefix) {
			if (p_index && g_selectedStar->m_count == p_index + 1) {
				g_selectedStar->m_count--;
				return -1;
			}
			else if (g_selectedStar->m_count <= p_index) {
				return 1;
			}
			else {
				return 0;
			}
		}

		if (g_mechChassis[type].m_tonnage > g_selectedStar->m_tonnage) {
			return 0;
		}

		strncpy(g_selectedStar->m_mechs[p_index].m_variant, p_variant, 8);
		g_selectedStar->m_mechs[p_index].m_variant[8] = '\0';
		if (strlen(p_variant) == 3) {
			strcat(g_selectedStar->m_mechs[p_index].m_variant, "00std");
		}
		g_selectedStar->m_mechs[p_index].m_chassis = type;
	}

	if (g_selectedStar->m_count == p_index && g_selectedStar->m_size > g_selectedStar->m_count) {
		g_selectedStar->m_count++;
	}

	return -1;
}

// FUNCTION: MW2SHELL 0x10003013
MechChar* GetStarMechVariant(MechS32 p_index)
{
	if (p_index < 0) {
		p_index = g_selectedStar->m_selected;
	}

	if (p_index < g_selectedStar->m_size && p_index < g_selectedStar->m_count) {
		return g_selectedStar->m_mechs[p_index].m_variant;
	}
	else {
		return "";
	}
}

// FUNCTION: MW2SHELL 0x1000307c
MechS32 GetStarMechChassis(MechS32 p_index)
{
	if (p_index < 0) {
		p_index = g_selectedStar->m_selected;
	}

	if (p_index < g_selectedStar->m_size && p_index < g_selectedStar->m_count) {
		return g_selectedStar->m_mechs[p_index].m_chassis;
	}
	else {
		return -1;
	}
}

// Returns a star's formation: the selected star's for a negative p_star, else the player's (0) or
// the enemy's.
// FUNCTION: MW2SHELL 0x100030e5
MechS32 GetStarFormation(MechS32 p_star)
{
	if (p_star < 0) {
		return g_selectedStar->m_formation;
	}
	else if (p_star) {
		return g_enemyStar.m_formation;
	}
	else {
		return g_playerStar.m_formation;
	}
}

// FUNCTION: MW2SHELL 0x1000312e
CustomStar* GetStar(MechS32 p_star)
{
	if (p_star < 0) {
		return g_selectedStar;
	}
	else if (p_star) {
		return &g_enemyStar;
	}
	else {
		return &g_playerStar;
	}
}

// Selects a star (negative arguments leave the setting alone) and sets its formation, its size,
// its mech count and its tonnage limit.
// FUNCTION: MW2SHELL 0x10003175
void SelectStar(MechS32 p_star, MechS32 p_formation, MechS32 p_size, MechS32 p_count, MechS32 p_tonnage)
{
	if (p_star >= 0) {
		if (p_star == 0) {
			g_selectedStar = &g_playerStar;
		}
		else {
			g_selectedStar = &g_enemyStar;
		}
	}

	if (p_formation >= 0) {
		g_selectedStar->m_formation = p_formation;
	}

	if (p_size >= 0) {
		g_selectedStar->m_size = p_size < 3 ? p_size : 3;
	}

	if (p_count >= 0) {
		g_selectedStar->m_count = p_count < 3 ? p_count : 3;
	}

	if (p_tonnage >= 0) {
		g_selectedStar->m_tonnage = p_tonnage;
	}
}

// Adds the formations to the simulator's command line and writes the stars' .bwd files.
// FUNCTION: MW2SHELL 0x10003221
void WriteStarFiles()
{
	strcat(g_simHandoff.m_cmdLine, " -of=");
	strcat(g_simHandoff.m_cmdLine, g_formationOptions[g_playerStar.m_formation].m_option);
	if (g_enemyStar.m_count) {
		strcat(g_simHandoff.m_cmdLine, " -oe=");
		strcat(g_simHandoff.m_cmdLine, g_formationOptions[g_enemyStar.m_formation].m_option);
	}

	PrjWriteStarTemplates(g_playerStar.m_count, g_playerStar.m_mechs, g_enemyStar.m_count, g_enemyStar.m_mechs);
}

#define STAR_LAYOUT(formation, i) g_formationLayout[formation].m_posts[i]

// Returns the mech whose video is under a point, or -1.
// Stack-slot permutation: mech, left, right, i, top and bottom.
// FUNCTION: MW2SHELL 0x10003320
MechS32 FindMechAt(MechS32 p_x, MechS32 p_y)
{
	MechS32 mech;
	MechS32 left;
	MechS32 right;
	MechS32 i;
	MechS32 top;
	MechS32 bottom;

	for (i = 2; i >= 0; i--) {
		left = STAR_LAYOUT(g_selectedStar->m_formation, i).m_left - 0x32;
		right = STAR_LAYOUT(g_selectedStar->m_formation, i).m_left + 0x32;
		top = STAR_LAYOUT(g_selectedStar->m_formation, i).m_top - 100;
		bottom = STAR_LAYOUT(g_selectedStar->m_formation, i).m_top;
		if (left <= p_x && right >= p_x && top <= p_y && bottom >= p_y) {
			mech = STAR_LAYOUT(g_selectedStar->m_formation, i).m_mech;
			if (g_selectedStar->m_mechs[mech].m_chassis >= 0) {
				return mech;
			}
		}
	}

	return -1;
}

// Returns as much of p_text as fits in p_width pixels of p_font.
// Stack-slot permutation: out and width. The original compares p_width against width; the
// declaration order doesn't flip it.
// FUNCTION: MW2SHELL 0x1000345a
MechChar* FitText(MechChar* p_text, Font* p_font, MechS32 p_width)
{
	MechChar* out = g_fitTextBuffer;
	MechS32 width = 0;

	if (p_text == NULL) {
		return p_text;
	}

	while (*p_text) {
		width += p_font->GetCharacterWidth(*p_text);
		if (width > p_width) {
			break;
		}
		*out++ = *p_text++;
	}
	*out = '\0';

	return g_fitTextBuffer;
}

// A click on a pilot name edits it.
// Stack-slot permutation: i, mech, label, left and top. The original loads left and top ahead of
// p_x and p_y in the bounds test; the declaration order doesn't flip it.
// FUNCTION: MW2SHELL 0x100034de
void EditPilotName(MechS32 p_x, MechS32 p_y, MechS32 p_campaign)
{
	MechS32 i;
	MechS32 mech;
	MechS32 label;
	MechS32 left;
	MechS32 top;

	for (i = 0; i < 3; i++) {
		mech = STAR_LAYOUT(g_selectedStar->m_formation, i).m_mech;
		if (mech > g_selectedStar->m_count) {
			continue;
		}
		if (mech == 0 && p_campaign != 2) {
			continue;
		}

		label = STAR_LAYOUT(g_selectedStar->m_formation, i).m_label;
		left = g_labelLefts[label];
		top = g_labelTops[label];
		if (left <= p_x && left + 100 > p_x && top <= p_y && top + 10 > p_y) {
			if (g_positionGlyphs[i][0]) {
				delete g_positionGlyphs[i][0];
			}

			EditTextField(g_defaultFont, left, top, g_selectedStar->m_mechs[mech].m_pilot, NULL, 0xf, 100);
			g_positionGlyphs[i][0] = g_textFont->AddOverlayText(left, top, g_selectedStar->m_mechs[mech].m_pilot, NULL);
			return;
		}
	}
}

// Shows the mech at a formation position: its video, pilot name, type and mass, or hides the
// position when the star has fewer mechs.
// Stack-slot permutation: label, type and mech.
// FUNCTION: MW2SHELL 0x10003690
void ShowFormationMech(MechS32 p_formation, MechS32 p_position, ButtonMenu* p_menu)
{
	MechS32 label;
	MechS32 type;
	MechS32 left;
	MechChar name[0x10];
	MechS32 top;
	MechS32 mech;

	if (g_positionGlyphs[p_position][0]) {
		delete g_positionGlyphs[p_position][0];
	}
	if (g_positionGlyphs[p_position][1]) {
		delete g_positionGlyphs[p_position][1];
	}
	if (g_positionGlyphs[p_position][2]) {
		delete g_positionGlyphs[p_position][2];
	}
	g_positionGlyphs[p_position][0] = NULL;
	g_positionGlyphs[p_position][1] = NULL;
	g_positionGlyphs[p_position][2] = NULL;

	label = STAR_LAYOUT(g_selectedStar->m_formation, p_position).m_label;
	mech = STAR_LAYOUT(p_formation, p_position).m_mech;
	p_menu->DisableButton(label + 6);
	if (mech >= g_selectedStar->m_count) {
		SetVideoFlags(p_position + 10, 0x40000000, 0x40000000);
		SetVideoFlags(label + 1, 0x20, 0x20);
		return;
	}

	ShowVideo(label + 1);
	p_menu->EnableButton(label + 6);

	type = g_selectedStar->m_mechs[mech].m_chassis;
	left = STAR_LAYOUT(g_selectedStar->m_formation, p_position).m_left;
	top = STAR_LAYOUT(g_selectedStar->m_formation, p_position).m_top;
	sprintf(name, g_starVideoFormat, g_mechChassis[type].m_code);
	PlayVideo(p_position + 10, name, left, top, 0x88, 0);

	left = g_labelLefts[label];
	top = g_labelTops[label];
	strcpy(g_selectedStar->m_mechs[mech].m_pilot, FitText(g_selectedStar->m_mechs[mech].m_pilot, g_textFont, 0x5c));
	g_positionGlyphs[p_position][0] =
		g_textFont->AddOverlayText(left, top, g_selectedStar->m_mechs[mech].m_pilot, NULL);
	g_positionGlyphs[p_position][1] = g_textFont->AddOverlayText(left, top + 0xc, g_mechChassis[type].m_name, NULL);

	g_mechMasses[mech] = g_mechChassis[type].m_tonnage;
	sprintf(name, "%d.00 T", g_mechChassis[type].m_tonnage);
	g_positionGlyphs[p_position][2] = g_textFont->AddOverlayText(left, top + 0x18, name, NULL);
}

// Redraws the formation, mission, star size, tonnage limit and star mass lines.
// Stack-slot permutation: i and mass.
// FUNCTION: MW2SHELL 0x10003a4e
void DrawStarInfo(MechS32 p_campaign)
{
	MechS32 i;
	MechS32 mass;

	if (g_formationLine) {
		delete g_formationLine;
	}
	g_formationLine = g_titleFont->AddText(0x140, 4, g_formationOptions[g_selectedStar->m_formation].m_label, NULL);

	if (g_missionLine) {
		delete g_missionLine;
	}
	if (p_campaign == 2) {
		strcpy(g_starInfoText, "~Mission: Trial of Grievance");
	}
	else {
		sprintf(g_starInfoText, "~Mission: %s", g_campaignMissions[p_campaign][g_currentPilot->m_mission].m_title);
	}
	g_missionLine = g_textFont->AddText(0x140, 0x23, g_starInfoText, NULL);

	if (g_starSizeLine) {
		delete g_starSizeLine;
	}
	sprintf(g_starInfoText, "~Maximum 'Mechs in current Star: %d", g_selectedStar->m_size);
	g_starSizeLine = g_textFont->AddText(0x140, 0x32, g_starInfoText, NULL);

	if (g_tonnageLine) {
		delete g_tonnageLine;
	}
	sprintf(g_starInfoText, "~Keshik Defined Maximum Tonnage (KDMT) per 'Mech: %d.00 T", g_selectedStar->m_tonnage);
	g_tonnageLine = g_textFont->AddText(0x140, 0x41, g_starInfoText, NULL);

	if (g_starMassLine) {
		delete g_starMassLine;
	}
	mass = 0;
	for (i = 0; i < g_selectedStar->m_count; i++) {
		mass += g_mechMasses[i];
	}
	sprintf(g_starInfoText, "~Current Total Mass of the Star: %d.00 T", mass);
	g_starMassLine = g_textFont->AddText(0x140, 0x50, g_starInfoText, NULL);
}

void StarConfigCallback(TMPackDataBase*, MechS32* p_campaign, MechU8*, char**, MechS32 p_msg);

// Opens the star screen of a campaign (2: a trial of grievance).
// FUNCTION: MW2SHELL 0x10003d3a
void DrawStarConfig(TMPackDataBase* p_database, MechS32 p_campaign)
{
	void* audioData;
	MechS32 i;
	MechS32 audioSize;

	g_videoDriver->LoadBackground(p_database, g_starConfigScreens[p_campaign].m_picture);
	g_starMenu = new ButtonMenu(g_videoDriver, g_defaultFont, 0, g_starConfigScreens[p_campaign].m_buttons, 9);

	switch (p_campaign) {
	case 0:
		PlayVideo(0, "awogrid", 0x8c, 0x136, 0x4a, 0);
		PlayVideo(1, "awostr1", 0x49, 0x9e, 0x44, 0);
		PlayVideo(2, "awostr2", 0x131, 0x7d, 0x44, 0);
		PlayVideo(3, "awostr3", 500, 0xb5, 0x44, 0);
		PlayVideo(4, "wwobkg", 0xdb, 0x1ad, 2, 0);
		PlayVideo(5, "wwodsgn", 0x197, 0x19a, 0x4a, 0);
		PlayVideo(6, "wwocn", 0x122, 0x1b1, 100, 0);
		PlayVideo(7, "wwocp", 0xf0, 0x1b3, 100, 0);
		PlayVideo(8, "wwovn", 300, 0x1ad, 100, 0);
		PlayVideo(9, "wwovp", 0xdc, 0x1b0, 100, 0);
		g_formationLayout = g_formationLayouts;
		g_starVideoFormat = g_wolfStarVideo;
		g_labelLefts = g_starLabelLefts;
		g_labelTops = g_starLabelTops;
		break;
	case 1:
		PlayVideo(0, "ajfgrid", 0x6c, 0x132, 0x4a, 0);
		PlayVideo(1, "ajfstr1", 0x27, 0xa4, 0x44, 0);
		PlayVideo(2, "ajfstr2", 0x110, 0x77, 0x44, 0);
		PlayVideo(3, "ajfstr3", 0x1e7, 0xd1, 0x44, 0);
		PlayVideo(4, "wjfbkg", 0xd8, 0x1b2, 2, 0);
		PlayVideo(5, "wjfdsgn", 0x171, 0x1a0, 0x48, 0);
		PlayVideo(6, "wjfcn", 0x11e, 0x1b6, 100, 0);
		PlayVideo(7, "wjfcp", 0xf2, 0x1b6, 100, 0);
		PlayVideo(8, "wjfvn", 0x128, 0x1b2, 100, 0);
		PlayVideo(9, "wjfvp", 0xdc, 0x1b5, 100, 0);
		g_formationLayout = g_jadeFalconFormationLayouts;
		g_starVideoFormat = g_jadeFalconStarVideo;
		g_labelLefts = g_jadeFalconLabelLefts;
		g_labelTops = g_jadeFalconLabelTops;
		break;
	case 2:
		PlayVideo(0, "aiagrid", 0x6c, 0x132, 0x4a, 0);
		PlayVideo(1, "aiastr1", 0x49, 0x9e, 0x44, 0);
		PlayVideo(2, "aiastr1", 0x131, 0x7d, 0x44, 0);
		PlayVideo(3, "aiastr1", 500, 0xb5, 0x44, 0);
		PlayVideo(4, "wiabkg2", 0xdb, 0x19e, 4, 0);
		PlayVideo(5, "wiadsgn", 0x19f, 0x1a1, 0x24, 0);
		PlayVideo(6, "wiacn", 0x109, 0x1ae, 0x24, 0);
		PlayVideo(7, "wiacp", 0xee, 0x1b5, 0x24, 0);
		PlayVideo(8, "wiavn", 0x12e, 0x1af, 0x24, 0);
		PlayVideo(9, "wiavp", 0xdf, 0x1b5, 0x24, 0);
		g_formationLayout = g_formationLayouts;
		g_starVideoFormat = g_trialStarVideo;
		g_labelLefts = g_starLabelLefts;
		g_labelTops = g_starLabelTops;
		break;
	}

	for (i = 0; i < 3; i++) {
		g_positionGlyphs[i][0] = NULL;
		g_positionGlyphs[i][1] = NULL;
		g_positionGlyphs[i][2] = NULL;
		ShowFormationMech(g_selectedStar->m_formation, i, g_starMenu);
	}

	g_formationLine = NULL;
	g_missionLine = NULL;
	g_starSizeLine = NULL;
	g_tonnageLine = NULL;
	g_starMassLine = NULL;

	g_mw2Database->GetDBItem(101, &audioData, &audioSize);
	g_starSound = new AudioSample(g_audioSubsystem, audioData, audioSize);
	g_starSound->SetVolume(0x32);
	g_mw2Database->GetDBItem(102, &audioData, &audioSize);
	g_mechLabSound = new AudioSample(g_audioSubsystem, audioData, audioSize);
	g_mechLabSound->SetVolume(0x32);

	DrawStarInfo(p_campaign);
	RegisterScreenFunction(StarConfigCallback);
}

// The star screen's buttons: 0 exits, 1 opens the mech lab, 2 and 3 cycle the formation, 4 and 5
// add and remove a mech, 6 to 8 change the mech at a label position. A double click on a mech
// changes it too.
// Stack-slot permutation: i, button and mech.
// FUNCTION: MW2SHELL 0x100043c2
void StarConfigCallback(TMPackDataBase*, MechS32* p_campaign, MechU8*, char**, MechS32 p_msg)
{
	MechS32 i;
	MechS32 button;
	MechS32 mech;

	// The original skips the frame's work with a goto: the jump it compiles to leaves a stub
	// after the function's end.
	if (p_msg != c_msgScreenFrame) {
		goto done;
	}

	button = g_starMenu->HitTest(g_mouseState->m_x, g_mouseState->m_y);
	if (*p_campaign == 2) {
		SetVideoFlags(5, 0x20, 0x20);
	}
	else {
		SetVideoFlags(5, 1, 1);
	}
	SetVideoFlags(6, 0x20, 0x20);
	SetVideoFlags(7, 0x20, 0x20);
	SetVideoFlags(8, 0x20, 0x20);
	SetVideoFlags(9, 0x20, 0x20);

	switch (button) {
	case 0:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		p_msg = c_msgReadyRoom;
		break;
	case 1:
		if (*p_campaign == 2) {
			ShowVideo(5);
		}
		else {
			SetVideoFlags(5, 1, 0);
		}
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_mechLabSound->Start();
		g_pickStarMech = 0;
		p_msg = c_msgMechBay;
		break;
	case 2:
		if (g_mouseState->m_leftDown == 1) {
			ShowVideo(6);
		}
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_starSound->Start();
		g_selectedStar->m_formation++;
		if (g_selectedStar->m_formation >= 6) {
			g_selectedStar->m_formation = 0;
		}
		for (i = 0; i < 3; i++) {
			ShowFormationMech(g_selectedStar->m_formation, i, g_starMenu);
		}
		DrawStarInfo(*p_campaign);
		break;
	case 3:
		if (g_mouseState->m_leftDown == 1) {
			ShowVideo(7);
		}
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_starSound->Start();
		g_selectedStar->m_formation--;
		if (g_selectedStar->m_formation < 0) {
			g_selectedStar->m_formation = 5;
		}
		for (i = 0; i < 3; i++) {
			ShowFormationMech(g_selectedStar->m_formation, i, g_starMenu);
		}
		DrawStarInfo(*p_campaign);
		break;
	case 4:
		if (g_mouseState->m_leftDown == 1) {
			ShowVideo(8);
		}
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_starSound->Start();
		if (g_selectedStar->m_count < g_selectedStar->m_size) {
			g_selectedStar->m_count++;
		}
		for (i = 0; i < 3; i++) {
			ShowFormationMech(g_selectedStar->m_formation, i, g_starMenu);
		}
		DrawStarInfo(*p_campaign);
		break;
	case 5:
		if (g_mouseState->m_leftDown == 1) {
			ShowVideo(9);
		}
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_starSound->Start();
		if (g_selectedStar->m_count > 1) {
			g_selectedStar->m_count--;
		}
		for (i = 0; i < 3; i++) {
			ShowFormationMech(g_selectedStar->m_formation, i, g_starMenu);
		}
		DrawStarInfo(*p_campaign);
		break;
	case 6:
	case 7:
	case 8:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		g_starSound->Start();
		for (i = 0; i < 3; i++) {
			if (STAR_LAYOUT(g_selectedStar->m_formation, i).m_label == button - 6) {
				g_selectedStar->m_selected = STAR_LAYOUT(g_selectedStar->m_formation, i).m_mech;
				g_pickStarMech = 1;
				p_msg = c_msgMechBay;
			}
		}
		break;
	}

	if (g_mouseState->GetLeftPressed() == 1) {
		EditPilotName(g_mouseState->m_x, g_mouseState->m_y, *p_campaign);
	}

	if (g_mouseState->GetDoubleClicked() && (mech = FindMechAt(g_mouseState->m_x, g_mouseState->m_y)) >= 0) {
		g_selectedStar->m_selected = mech;
		g_pickStarMech = 1;
		p_msg = c_msgMechBay;
	}

done:
	if (p_msg != c_msgScreenFrame) {
		delete g_starMenu;
		delete g_mechLabSound;
		delete g_starSound;
		CloseAllVideos();
		g_videoDriver->ClearGlyphs(TRUE);
		MechPostMessage(p_msg, c_msgStarConfig, 0);
		UnregisterScreenFunction(StarConfigCallback);
	}
}

#undef STAR_LAYOUT
