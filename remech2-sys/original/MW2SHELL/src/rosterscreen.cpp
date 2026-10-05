#include "rosterscreen.h"

#include "audiosample.h"
#include "audiosubsystem.h"
#include "buttonmenu.h"
#include "campaignmission.h"
#include "decomp.h"
#include "elapsed.h"
#include "font.h"
#include "keyboardinput.h"
#include "mainmenubutton.h"
#include "mechbay.h"
#include "mechrand.h"
#include "mechvariant.h"
#include "menudata.h"
#include "menuscreen.h"
#include "messages.h"
#include "missionui.h"
#include "mousestate.h"
#include "options.h"
#include "pilotrecord.h"
#include "pilotroster.h"
#include "refreshmode.h"
#include "screenfield.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "stringutil.h"
#include "textglyph.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The pilot roster screen of a clan hall: ten pilot slots, the selected pilot's record and the
// mission list.

// DrawPilotRoster's p_campaign.
// GLOBAL: MW2SHELL 0x1007cc98
MechS32 g_rosterCampaign;

// GLOBAL: MW2SHELL 0x1007cca0
MechChar g_rosterFieldText[0x100];

// The campaign's ten slots of g_pilotRoster.
// GLOBAL: MW2SHELL 0x1007cda8
PilotRecord* g_rosterPilots[10];

// Stack-slot permutation: top and i.
// FUNCTION: MW2SHELL 0x10014b60
void ShowPilotCallsigns()
{
	PilotRecord* pilot;
	MechS32 top;
	MechS32 i;

	top = 0x5c;
	for (i = 0; i < 10; i++, top += 0x23) {
		pilot = g_rosterPilots[i];
		if (pilot->m_inUse == 0) {
			continue;
		}

		if (pilot->m_glyph) {
			delete pilot->m_glyph;
		}
		pilot->m_glyph = g_titleFont->AddText(0x2a, top, pilot->m_callsign, NULL);
	}
}

// FUNCTION: MW2SHELL 0x10014c1e
void HidePilotCallsigns()
{
	PilotRecord* pilot;
	MechS32 i;

	for (i = 0; i < 10; i++) {
		pilot = g_rosterPilots[i];
		if (pilot->m_glyph) {
			delete pilot->m_glyph;
			pilot->m_glyph = NULL;
		}
	}
}

// FUNCTION: MW2SHELL 0x10014caa
void ClearPilot(PilotRecord* p_pilot)
{
	p_pilot->m_inUse = 0;
	strcpy(p_pilot->m_callsign, "");
	if (p_pilot->m_glyph) {
		delete p_pilot->m_glyph;
		p_pilot->m_glyph = NULL;
	}
}

// FUNCTION: MW2SHELL 0x10014d3e
void SetActivePilot(PilotRecord* p_pilot)
{
	MechS32 i;

	for (i = 0; i < 10; i++) {
		g_rosterPilots[i]->m_active = 0;
	}
	p_pilot->m_active = 1;
}

// Draws the tab's data, a string, in the button font.
// FUNCTION: MW2SHELL 0x10014dc4
TextGlyph* DrawRosterLabel(ScreenField* p_tab)
{
	MechChar* text = (MechChar*) p_tab->m_data;

	return g_buttonFont->AddText(p_tab->m_left, p_tab->m_top, text, NULL);
}

// The mission list: the tab's data is the mission index. Missions the pilot hasn't reached stay blank.
// FUNCTION: MW2SHELL 0x10014dfe
TextGlyph* DrawMissionListEntry(ScreenField* p_tab)
{
	if (MECH_PTR_TO_S32(p_tab->m_data) >= g_currentPilot->m_mission) {
		return NULL;
	}

	sprintf(g_rosterFieldText, "~%s", g_campaignMissions[g_rosterCampaign][MECH_PTR_TO_S32(p_tab->m_data)].m_title);
	return g_textFont->AddText(p_tab->m_left + p_tab->m_width / 2, p_tab->m_top, g_rosterFieldText, NULL);
}

// The pilot record callbacks below open with a test of p_tab that does nothing.

// FUNCTION: MW2SHELL 0x10014e83
TextGlyph* DrawPilotCallsign(ScreenField* p_tab)
{
	if (p_tab) {
	}

	sprintf(g_rosterFieldText, "~%s", g_currentPilot->m_callsign);
	return g_titleFont->AddText(p_tab->m_left, p_tab->m_top, g_rosterFieldText, NULL);
}

// FUNCTION: MW2SHELL 0x10014ed7
TextGlyph* DrawPilotRank(ScreenField* p_tab)
{
	if (p_tab) {
	}

	sprintf(g_rosterFieldText, "~%s", g_rankNames[g_currentPilot->m_rank]);
	return g_titleFont->AddText(p_tab->m_left, p_tab->m_top, g_rosterFieldText, NULL);
}

// FUNCTION: MW2SHELL 0x10014f32
TextGlyph* DrawPilotHonor(ScreenField* p_tab)
{
	if (p_tab) {
	}

	sprintf(g_rosterFieldText, "~%d", g_currentPilot->m_honor);
	return g_titleFont->AddText(p_tab->m_left, p_tab->m_top, g_rosterFieldText, NULL);
}

// FUNCTION: MW2SHELL 0x10014f86
TextGlyph* DrawPilotMission(ScreenField* p_tab)
{
	if (p_tab) {
	}

	sprintf(g_rosterFieldText, "~%s", g_campaignMissions[g_rosterCampaign][g_currentPilot->m_mission].m_title);
	return g_titleFont->AddText(p_tab->m_left, p_tab->m_top, g_rosterFieldText, NULL);
}

// The mission list's click callback.
// FUNCTION: MW2SHELL 0x10014fee
void ClickMissionListEntry(ScreenField* p_tab)
{
	if (p_tab) {
	}
}

// Negative m_top: rows (times the previous tab's height) and an offset below the previous tab.
#define TAB_BELOW(rows, offset) ((MechS32) (0x80000000 | ((rows) << 4) | (offset)))
#define ROSTER_TAB(x, y, width, draw, click, data) {x, y, width, -1, 0, NULL, NULL, draw, click, (void*) (data), NULL}
#define ROSTER_END {-1, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL}

// The selected pilot's record.
// GLOBAL: MW2SHELL 0x10063c78
ScreenField g_pilotRecordFields[8] = {
	ROSTER_TAB(0x1d4, 0x5c, -1, DrawPilotCallsign, NULL, NULL),
	ROSTER_TAB(0x1d4, 0xd1, -1, DrawRosterLabel, NULL, "~RANK"),
	ROSTER_TAB(0x1d4, TAB_BELOW(1, 3), -1, DrawPilotRank, NULL, NULL),
	ROSTER_TAB(0x1d4, TAB_BELOW(2, 2), -1, DrawRosterLabel, NULL, "~HONOR"),
	ROSTER_TAB(0x1d4, TAB_BELOW(1, 3), -1, DrawPilotHonor, NULL, NULL),
	ROSTER_TAB(0x1d4, TAB_BELOW(2, 2), -1, DrawRosterLabel, NULL, "~MISSION"),
	ROSTER_TAB(0x1d4, TAB_BELOW(1, 3), -1, DrawPilotMission, NULL, NULL),
	ROSTER_END,
};

// The mission list.
// GLOBAL: MW2SHELL 0x10063dd8
ScreenField g_missionListFields[] = {
	ROSTER_TAB(0x1d4, 0x5c, -1, DrawPilotCallsign, NULL, NULL),
	ROSTER_TAB(0x1d4, 0xc8, -1, DrawRosterLabel, NULL, "~Select Mission"),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 3), 100, DrawMissionListEntry, ClickMissionListEntry, 0),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 1),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 2),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 3),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 4),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 5),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 6),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 7),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 8),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 9),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 10),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 11),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 12),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 13),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 14),
	ROSTER_TAB(0x1a2, TAB_BELOW(1, 2), 100, DrawMissionListEntry, ClickMissionListEntry, 15),
	ROSTER_END,
};

#undef TAB_BELOW
#undef ROSTER_TAB
#undef ROSTER_END

// Database item 0x51, started when the roster opens.
// GLOBAL: MW2SHELL 0x1006411c
AudioSample* g_rosterSound = NULL;

// Set while the mission list shows instead of the pilot's record.
// GLOBAL: MW2SHELL 0x10064120
MechS32 g_missionListShown = 0;

// GLOBAL: MW2SHELL 0x1007cda0
ButtonMenu* g_rosterMenu;

// Opens the pilot roster of the campaign's clan hall: the ten pilot slots, the active pilot's
// record, and the menu.
// Not 100%: the stack slots of data, slot, i and size are permuted.
// FUNCTION: MW2SHELL 0x10015008
void DrawPilotRoster(TMPackDataBase* p_database, MechS32 p_campaign, MechU8* p_pilotChosen, char**)
{
	void* data = NULL;
	MechS32 slot;
	MechS32 i;
	MechS32 size;

	g_rosterCampaign = p_campaign;
	// The original seeded with clock(), milliseconds since the program started
	MechSRand(MechMilliseconds());
	g_videoDriver->LoadBackground(p_database, g_rosterScreens[p_campaign].m_picture);
	LoadPilotRoster();
	g_currentPilot = NULL;

	switch (p_campaign) {
	case 0:
		slot = 0;
		break;
	case 1:
		slot = 10;
		break;
	default:
		slot = 0;
		break;
	}

	for (i = 0; i < 10; i++, slot++) {
		g_rosterPilots[i] = &g_pilotRoster[slot];
		if (g_currentPilot) {
			g_pilotRoster[slot].m_active = 0;
		}
		else if (g_pilotRoster[slot].m_active) {
			if (g_pilotRoster[slot].m_inUse == 1) {
				g_currentPilot = &g_pilotRoster[slot];
			}
			else {
				g_pilotRoster[slot].m_active = 0;
			}
		}
	}

	g_rosterMenu = new ButtonMenu(g_videoDriver, g_defaultFont, FALSE, g_rosterScreens[p_campaign].m_buttons, 15);
	ShowPilotCallsigns();
	if (!g_currentPilot) {
		g_rosterMenu->DisableButton(11);
		g_rosterMenu->DisableButton(12);
		g_rosterMenu->DisableButton(13);
		g_rosterMenu->DisableButton(14);
	}
	else {
		ShowFields(g_pilotRecordFields);
		g_rosterMenu->DisableButton(14);
		if (!g_currentPilot->m_mission) {
			g_rosterMenu->DisableButton(13);
		}
	}

	g_mw2Database->GetDBItem(0x51, &data, &size);
	g_rosterSound = new AudioSample(g_audioSubsystem, data, size);
	g_rosterSound->SetVolume(0x1e);
	g_rosterSound->Start();
	*p_pilotChosen = 0;
	RegisterScreenFunction(PilotRosterCallback);
	g_videoDriver->DrawShell();
	g_videoDriver->ExpandRectBySize(0, 0, 640, 480);
}

// PilotRosterCallback, the pilot roster's frame, is implemented on the Rust side
// (src/shell/screens/roster.rs).
