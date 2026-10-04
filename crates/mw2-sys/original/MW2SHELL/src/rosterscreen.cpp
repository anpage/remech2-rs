#include "rosterscreen.h"

#include "audiosample.h"
#include "audiosubsystem.h"
#include "buttonmenu.h"
#include "campaignmission.h"
#include "decomp.h"
#include "font.h"
#include "keyboardinput.h"
#include "mainmenubutton.h"
#include "mechbay.h"
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
#include <time.h>

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

// Draws the tab's data, a string, in the text font. Unused.
// FUNCTION: MW2SHELL 0x10014d8a
TextGlyph* DrawRosterText(ScreenField* p_tab)
{
	MechChar* text = (MechChar*) p_tab->m_data;

	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, text, NULL);
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

void PilotRosterCallback(
	TMPackDataBase*,
	MechS32* p_campaign,
	MechU8* p_pilotChosen,
	MechChar** p_scenario,
	MechS32 p_msg
);

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
	srand(clock());
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

// The pilot roster's frame: EXIT, the ten pilot slots (an empty one asks for a callsign),
// ACCEPT, DELETE MECHWARRIOR, LAUNCH OLD MISSION and PILOT INFO.
// Not 100%: the stack slots are permuted, and the mission test loads the tab before the pilot
// (reversing the comparison's operands doesn't flip it).
// FUNCTION: MW2SHELL 0x1001534c
void PilotRosterCallback(
	TMPackDataBase*,
	MechS32* p_campaign,
	MechU8* p_pilotChosen,
	MechChar** p_scenario,
	MechS32 p_msg
)
{
	PilotRecord* pilot;
	MechS32 button;
	ScreenField* tab;

	// The original skips the frame's work with a goto, like StarConfigCallback.
	if (p_msg != c_msgScreenFrame) {
		goto done;
	}

	button = g_rosterMenu->HitTest(g_mouseState->m_x, g_mouseState->m_y);
	if (g_mouseState->GetLeftPressed() == 1) {
		if (g_missionListShown) {
			tab = FindFieldAt(g_missionListFields, g_mouseState->m_x, g_mouseState->m_y);
			if (tab && tab->m_click && g_currentPilot->m_mission > MECH_PTR_TO_S32(tab->m_data)) {
				*p_scenario = g_campaignMissions[*p_campaign][MECH_PTR_TO_S32(tab->m_data)].m_scenario;
				SelectStar(0, 0, 3, 1, 100);
				ShellApplyMissionUiInfo(*p_scenario, 1, 0);
				SelectStar(1, 0, 0, 0, 100);
				SelectStar(0, -1, -1, -1, -1);
				SetStarMech(0, NULL, g_currentPilot->m_callsign);
				p_msg = c_msgLaunchSim;
			}
		}

	again:
		switch (button) {
		case 0:
			g_currentPilot = NULL;
			p_msg = c_msgMainMenu;
			break;
		case 12:
			if (g_currentPilot && ShowDialog("Terminate MechWarrior?#Yes|No", 1) == 1) {
				break;
			}
			ClearPilot(g_currentPilot);
			if (g_newPilotRegistered) {
				g_newPilotRegistered = 0;
			}
			HideFields(g_pilotRecordFields);
			HideFields(g_missionListFields);
			g_missionListShown = 0;
			g_currentPilot = NULL;
			g_rosterMenu->DisableButton(11);
			g_rosterMenu->DisableButton(12);
			g_rosterMenu->DisableButton(13);
			g_rosterMenu->DisableButton(14);
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			pilot = g_rosterPilots[button - 1];
			if (pilot->m_inUse) {
				g_currentPilot = pilot;
				SetActivePilot(pilot);
				if (g_mouseState->GetDoubleClicked()) {
					*p_pilotChosen = 1;
					p_msg = c_msgClanHall;
					break;
				}
				g_rosterMenu->EnableButton(11);
				g_rosterMenu->EnableButton(12);
				if (g_currentPilot->m_mission) {
					g_rosterMenu->EnableButton(13);
				}
				else {
					g_rosterMenu->DisableButton(13);
				}
				g_rosterMenu->DisableButton(14);
				HideFields(g_missionListFields);
				g_missionListShown = 0;
				HideFields(g_pilotRecordFields);
				ShowFields(g_pilotRecordFields);
			}
			else {
				g_rosterMenu->DisableButton(11);
				g_rosterMenu->DisableButton(12);
				g_rosterMenu->DisableButton(13);
				g_rosterMenu->DisableButton(14);
				HideFields(g_missionListFields);
				g_missionListShown = 0;
				HideFields(g_pilotRecordFields);
				g_currentPilot = NULL;
				pilot->m_callsign[0] = '\0';
				EditTextField(g_titleFont, 0x2a, (button - 1) * 35 + 0x5c, pilot->m_callsign, NULL, 14, 300);
				UppercaseString(pilot->m_callsign);
				if (pilot->m_callsign[0]) {
					g_currentPilot = pilot;
					pilot->m_inUse = 1;
					pilot->m_mission = 0;
					pilot->m_rank = 0;
					pilot->m_honor = (MechS32) (rand() / 32767.0 * 1000.0 + 1000.0);
					pilot->m_kills = 0;
					pilot->m_hits = 0;
					pilot->m_shotsFired = 0;
					pilot->m_unk0x24 = 0;
					SetActivePilot(pilot);
					ShowPilotCallsigns();
					g_rosterMenu->EnableButton(11);
					g_rosterMenu->EnableButton(12);
					ShowFields(g_pilotRecordFields);
					g_newPilotRegistered = 1;
				}

				if (g_mouseState->m_leftDown == 1) {
					button = g_rosterMenu->HitTest(g_mouseState->m_x, g_mouseState->m_y);
					goto again;
				}
			}
			break;
		case 11:
			if (g_currentPilot) {
				*p_pilotChosen = 1;
				p_msg = c_msgClanHall;
			}
			break;
		case 13:
			g_rosterMenu->DisableButton(13);
			g_rosterMenu->EnableButton(14);
			HideFields(g_pilotRecordFields);
			ShowFields(g_missionListFields);
			g_missionListShown = 1;
			break;
		case 14:
			if (g_currentPilot->m_mission) {
				g_rosterMenu->EnableButton(13);
			}
			g_rosterMenu->DisableButton(14);
			HideFields(g_missionListFields);
			g_missionListShown = 0;
			ShowFields(g_pilotRecordFields);
			break;
		}
	}

done:
	if (p_msg != c_msgScreenFrame) {
		HideFields(g_missionListFields);
		HideFields(g_pilotRecordFields);
		SavePilotRoster();
		delete g_rosterMenu;
		delete g_rosterSound;
		HidePilotCallsigns();
		MechPostMessage(p_msg, c_msgPilotRoster, 0);
		UnregisterScreenFunction(PilotRosterCallback);
	}
}
