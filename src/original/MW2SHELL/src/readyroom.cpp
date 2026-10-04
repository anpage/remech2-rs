#include "readyroom.h"

#include "audiosample.h"
#include "audiosubsystem.h"
#include "buttonmenu.h"
#include "campaignmission.h"
#include "decomp.h"
#include "font.h"
#include "mainmenubutton.h"
#include "mechbay.h"
#include "mechchassis.h"
#include "mechvariant.h"
#include "menudata.h"
#include "menuscreen.h"
#include "missionui.h"
#include "mousestate.h"
#include "options.h"
#include "pilotrecord.h"
#include "pilotroster.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

// The ready room screen.

// The video playing before the screen moves on, -1 for none, and the message it moves on with.
// GLOBAL: MW2SHELL 0x1006a588
MechS32 g_readyRoomExitVideo = -1;

// GLOBAL: MW2SHELL 0x1006a58c
MechS32 g_readyRoomExitMessage = c_msgScreenFrame;

// Database item 0x64, played with the mech bay's video.
// GLOBAL: MW2SHELL 0x1006a590
AudioSample* g_readyRoomSound = NULL;

// Set once the ready room's quick tips have been shown.
// GLOBAL: MW2SHELL 0x1006a594
MechS32 g_readyRoomTipsShown = 0;

// GLOBAL: MW2SHELL 0x100904a0
ButtonMenu* g_readyRoomMenu;

// DrawReadyRoom's p_wParam: the quick tips show when it comes from the clan hall (c_msgClanHall).
// GLOBAL: MW2SHELL 0x100904a4
WPARAM g_readyRoomMessage;

// Play the second faction grid animation only when video slot zero is idle.
// FUNCTION: MW2SHELL 0x10039de0
void PlayReadyRoomGrid(MechS32 p_campaign)
{
	if (!IsVideoPlaying(0)) {
		switch (p_campaign) {
		case 0:
			PlayVideo(0, "awogrid2", 0x12f, 0x149, 0x48, 0);
			break;
		case 1:
			PlayVideo(0, "ajfgrid2", 0x115, 0x155, 0x48, 0);
		default:
			break;
		}
	}
}

void ReadyRoomCallback(TMPackDataBase* p_database, MechS32* p_campaign, MechU8*, MechChar** p_scenario, MechS32 p_msg);

// Opens the ready room. Coming from the clan hall (c_msgClanHall) or a mission (c_msgDebrief) sets
// up the pilot's next mission first; only the pilot FREEBIRTHTOAD gets the mission buttons.
// FUNCTION: MW2SHELL 0x10039e72
void DrawReadyRoom(TMPackDataBase* p_database, MechS32 p_campaign, char** p_scenario, WPARAM p_wParam)
{
	if (p_wParam == c_msgClanHall || p_wParam == c_msgDebrief) {
		SelectStar(0, 0, 3, 1, 100);
		*p_scenario = g_campaignMissions[p_campaign][g_currentPilot->m_mission].m_scenario;
		ShellApplyMissionUiInfo(*p_scenario, 1, 0);
	}

	g_readyRoomMessage = p_wParam;
	SelectStar(1, 0, 0, 0, 100);
	SelectStar(0, -1, -1, -1, -1);
	SetStarMech(0, NULL, NULL);
	g_videoDriver->LoadBackground(p_database, g_readyRoomScreens[p_campaign].m_picture);

	if (!strcmp(g_currentPilot->m_callsign, "FREEBIRTHTOAD")) {
		g_readyRoomMenu = new ButtonMenu(
			g_videoDriver,
			g_defaultFont,
			FALSE,
			g_readyRoomScreens[p_campaign].m_buttons,
			g_readyRoomScreens[p_campaign].m_count
		);
	}
	else {
		g_readyRoomMenu =
			new ButtonMenu(g_videoDriver, g_defaultFont, FALSE, g_readyRoomScreens[p_campaign].m_buttons, 4);
	}

	switch (p_campaign) {
	case 0:
		PlayVideo(0, "awogrid1", 0x12f, 0x149, 0x44, 0);
		g_mouseState->MoveCursorTo(0x1b3, 0x168);
		break;
	case 1:
		PlayVideo(0, "ajfgrid1", 0x115, 0x155, 0x44, 0);
		if (p_wParam == c_msgClanHall) {
			PlayVideo(0x10, "ajfv8trd", 1, 0x6c, 2, 0);
		}
		break;
	}

	RegisterScreenFunction(ReadyRoomCallback);
}

// The ready room's frame: CLAN HALL, MECH LAB, STAR CONFIG, MISSION BRIEFING, and for
// FREEBIRTHTOAD the missions. The mech lab and briefing play a video before moving on.
// Not 100%: the stack slots of the locals are permuted.
// FUNCTION: MW2SHELL 0x1003a151
void ReadyRoomCallback(TMPackDataBase* p_database, MechS32* p_campaign, MechU8*, MechChar** p_scenario, MechS32 p_msg)
{
	void* data = NULL;
	MechChar name[16];
	MechS32 type;
	MechS32 button;
	MechS32 size;

	// The original skips the frame's work with a goto, like StarConfigCallback.
	if (p_msg != c_msgScreenFrame) {
		goto done;
	}

	if (g_quickTips && !g_readyRoomTipsShown && g_readyRoomMessage == c_msgClanHall && !IsVideoPlaying(0x10)) {
		DialogBoxParam(g_module, MAKEINTRESOURCE(0x70), g_gameWindow, (DLGPROC) OkDialogProc, 0);
		g_readyRoomTipsShown = 1;
	}

	if (g_readyRoomExitVideo == -1) {
		PlayReadyRoomGrid(*p_campaign);
		button = g_readyRoomMenu->HitTest(g_mouseState->m_x, g_mouseState->m_y);
		switch (button) {
		case -1:
			break;
		case 1:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			if (g_campaignMissions[g_currentPilot->m_clan][g_currentPilot->m_mission].m_trial == 1) {
				ShowDialog("Trial Protocol: X0769-Q|Keshik to determine appropriate|'Mech for trial.#Ok", 0);
				break;
			}
			switch (*p_campaign) {
			case 0:
				SetVideoFlags(0, 0x40000000, 0x40000000);
				type = GetStarMechChassis(-1);
				if (type < 0) {
					type = 7;
				}
				sprintf(name, "awo%stbl", g_mechChassis[type].m_code);
				g_readyRoomExitVideo = PlayVideo(0, name, 0x131, 0xb9, 6, 0);
				break;
			case 1:
				SetVideoFlags(0, 0x40000000, 0x40000000);
				type = GetStarMechChassis(-1);
				if (type < 0) {
					type = 7;
				}
				sprintf(name, "ajf%stbl", g_mechChassis[type].m_code);
				g_readyRoomExitVideo = PlayVideo(0, name, 0x114, 0xa4, 6, 0);
				break;
			}
			p_database->GetDBItem(0x64, &data, &size);
			g_readyRoomSound = new AudioSample(g_audioSubsystem, data, size);
			g_readyRoomSound->SetVolume(0x32);
			g_readyRoomSound->Start();
			g_readyRoomExitMessage = c_msgMechBay;
			break;
		case 2:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			if (g_campaignMissions[g_currentPilot->m_clan][g_currentPilot->m_mission].m_trial == 1) {
				ShowDialog("Your 'Mech has been|selected for you.|Prepare for Trial!#Ok", 0);
				break;
			}
			p_msg = c_msgStarConfig;
			break;
		case 0:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			switch (*p_campaign) {
			case 0:
				p_msg = c_msgClanHall;
				break;
			case 1:
				g_readyRoomExitVideo = 0x10;
				break;
			}
			break;
		case 3:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
		briefing:
			switch (*p_campaign) {
			case 0:
				g_readyRoomExitVideo = PlayVideoInFreeSlot("awobrief", 0x69, 0x64, 6, 0);
				break;
			case 1:
				g_readyRoomExitVideo = PlayVideoInFreeSlot("ajfbrief", 0x6b, 0x69, 6, 0);
				break;
			}
			g_readyRoomExitMessage = c_msgBriefing;
			break;
		default:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			g_currentPilot->m_mission = button - 4;
			SavePilotRoster();
			*p_scenario = g_campaignMissions[*p_campaign][button - 4].m_scenario;
			SelectStar(0, 0, 3, 1, 100);
			ShellApplyMissionUiInfo(*p_scenario, 1, 0);
			goto briefing;
		}
	}
	else if (!IsVideoPlaying(g_readyRoomExitVideo)) {
		if (g_readyRoomExitMessage == c_msgScreenFrame) {
			g_readyRoomExitVideo = PlayVideo(0x10, "ajfv8tru", 1, 0x6c, 2, 0);
			g_readyRoomExitMessage = c_msgClanHall;
		}
		else {
			p_msg = g_readyRoomExitMessage;
			g_readyRoomExitVideo = -1;
			g_readyRoomExitMessage = c_msgScreenFrame;
		}
	}

done:
	if (p_msg != c_msgScreenFrame) {
		CloseAllVideos();
		delete g_readyRoomMenu;
		if (g_readyRoomSound) {
			delete g_readyRoomSound;
		}
		g_readyRoomSound = NULL;
		g_readyRoomTipsShown = 0;
		PostMessage(g_gameWindow, p_msg, c_msgReadyRoom, 0);
		UnregisterScreenFunction(ReadyRoomCallback);
	}
}
