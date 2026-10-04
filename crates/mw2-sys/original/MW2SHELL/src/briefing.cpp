#include "briefing.h"

#include "archivereader.h"
#include "buttonmenu.h"
#include "collection.h"
#include "decomp.h"
#include "font.h"
#include "keyboardinput.h"
#include "mainmenubutton.h"
#include "menudata.h"
#include "menuscreen.h"
#include "messages.h"
#include "mousestate.h"
#include "page.h"
#include "pilotrecord.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "stringutil.h"
#include "textpages.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "videodriver.h"

#include <string.h>
#include <windows.h>

// The mission briefing screen.

// The briefing's p_quote for LoadTextPages: empty, so the quote escapes in a briefing's text
// expand to nothing. That is its only use, and it gives no name.
// GLOBAL: MW2SHELL 0x10071cd8
MechChar g_unk0x10071cd8[0x04] = "";

// GLOBAL: MW2SHELL 0x10071cdc
ButtonMenu* g_briefingMenu = NULL;

// GLOBAL: MW2SHELL 0x10071ce0
Page* g_briefingPage = NULL;

// GLOBAL: MW2SHELL 0x10071ce4
Collection* g_briefingPages = NULL;

// The situation reader, while it is open.
// GLOBAL: MW2SHELL 0x10071ce8
ArchiveReader* g_situationReader = NULL;

void BriefingCallback(TMPackDataBase*, MechS32* p_campaign, MechU8*, MechChar**, MechS32 p_msg);

// Opens the briefing screen: lays the scenario's briefing text (its first four letters, padded
// with '_', plus "BRF1") out on pages under the menu. A campaign's last mission briefs from
// KTWOBRF1/KTJFBRF1 once the pilot's rank is high enough.
// Not 100%: the stack slots of left, top, width, height and i are permuted.
// FUNCTION: MW2SHELL 0x10046200
void DrawBriefing(TMPackDataBase* p_database, char* p_scenario, MechS32 p_campaign)
{
	MechS32 left;
	MechS32 top;
	MechS32 width;
	MechS32 height;
	MechS32 i;
	MechChar name[0x80];

	CreateCollection(&g_briefingPages, 10, NULL, 4, NULL);
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

	if (g_currentPilot->m_mission == 15 && g_currentPilot->m_rank >= 6) {
		switch (p_campaign) {
		case 0:
			strcpy(name, "KTWOBRF1");
			break;
		case 1:
			strcpy(name, "KTJFBRF1");
			break;
		default:
			break;
		}
	}
	else {
		for (i = 0; i < 4 && p_scenario[i] && p_scenario[i] != '.'; i++) {
			name[i] = p_scenario[i];
		}
		while (i < 4) {
			name[i] = '_';
			i++;
		}
		name[i] = '\0';
		strcat(name, "BRF1");
	}

	UppercaseString(name);
	LoadTextPages(g_briefingPages, left, top, width, height, name, g_bodyFont, g_unk0x10071cd8);

	g_briefingPage = (Page*) CollectionGet(g_briefingPages, 0);
	if (!g_briefingPage) {
		MechPostMessage(c_msgLaunchSim, c_msgBriefing, 0);
		return;
	}

	g_videoDriver->LoadBackground(p_database, g_briefingScreens[p_campaign].m_picture);
	CollectionRemove(g_briefingPages, g_briefingPage, FALSE);
	g_keyboardInput->FlushKeys();

	if (strcmp(g_currentPilot->m_callsign, "FERRARI")) {
		g_briefingScreens[p_campaign].m_count = 3;
	}
	g_briefingMenu = new ButtonMenu(
		g_videoDriver,
		g_bodyFont,
		FALSE,
		g_briefingScreens[p_campaign].m_buttons,
		g_briefingScreens[p_campaign].m_count
	);

	if (!g_briefingPages->m_count) {
		g_briefingMenu->DisableButton(1);
	}
	g_briefingPage->Restart();
	RegisterScreenFunction(BriefingCallback);
}

// The briefing screen's frame: ABORT, SITUATION (the text in a reader), LAUNCH and SKIP.
// FUNCTION: MW2SHELL 0x10046653
void BriefingCallback(TMPackDataBase*, MechS32* p_campaign, MechU8*, MechChar**, MechS32 p_msg)
{
	MechS32 result;
	MechS32 button;

	// The original skips the frame's work with a goto, like StarConfigCallback.
	if (p_msg != c_msgScreenFrame) {
		goto done;
	}

	if (!g_situationReader) {
		g_briefingPage->TypeStep();
		button = g_briefingMenu->HitTest(g_mouseState->m_x, g_mouseState->m_y);
		switch (button) {
		case 2:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			p_msg = c_msgLaunchSim;
			break;
		case 0:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			p_msg = c_msgReadyRoom;
			break;
		case 3:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			p_msg = c_msgDebrief;
			break;
		case 1:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			g_briefingPage->Hide();
			delete g_briefingMenu;
			g_situationReader = new ArchiveReader(
				"",
				g_archiveFont,
				-1,
				FALSE,
				NULL,
				g_briefingPages,
				g_situationScreens[*p_campaign].m_buttons,
				g_situationScreens[*p_campaign].m_count
			);
			break;
		default:
			break;
		}
	}
	else {
		result = g_situationReader->Run();
		if (result == c_msgQuit) {
			p_msg = c_msgQuit;
		}
		if (result == c_msgMainMenu) {
			p_msg = c_msgMainMenu;
		}
		if (result != c_msgArchive) {
			delete g_situationReader;
			g_situationReader = NULL;
			g_briefingMenu = new ButtonMenu(
				g_videoDriver,
				g_defaultFont,
				FALSE,
				g_briefingScreens[*p_campaign].m_buttons,
				g_briefingScreens[*p_campaign].m_count
			);
			g_briefingPage->Restart();
		}
	}

done:
	if (p_msg != c_msgScreenFrame) {
		if (g_situationReader) {
			delete g_situationReader;
		}
		g_situationReader = NULL;
		delete g_briefingPage;
		delete g_briefingMenu;
		MechPostMessage(p_msg, c_msgBriefing, 0);
		UnregisterScreenFunction(BriefingCallback);
	}
}
