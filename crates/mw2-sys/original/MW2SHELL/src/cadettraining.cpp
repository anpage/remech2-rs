#include "cadettraining.h"

#include "audiosample.h"
#include "audiosubsystem.h"
#include "buttonmenu.h"
#include "decomp.h"
#include "mainmenubutton.h"
#include "menudata.h"
#include "menuscreen.h"
#include "messages.h"
#include "missionui.h"
#include "mousestate.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdlib.h>
#include <time.h>
#include <windows.h>

// The trainer's idle video alternates between two takes; a countdown to the next.
// GLOBAL: MW2SHELL 0x1006acc8
MechS32 g_trainerTake = 0;

// GLOBAL: MW2SHELL 0x1006accc
MechS32 g_trainerIdleCountdown = -1;

// Set once the trainer's welcome video is over and the mission buttons are up.
// GLOBAL: MW2SHELL 0x1006acd0
MechS32 g_trainingButtonsShown = 0;

// The trainer's video playing before a training mission starts, -1 for none.
// GLOBAL: MW2SHELL 0x1006acd4
MechS32 g_trainingExitVideo = -1;

// The room's sound, database item 0x4c, looped once the welcome is over.
// GLOBAL: MW2SHELL 0x1006acd8
AudioSample* g_trainingAmbience = NULL;

// Set once the training screen's quick tips have been shown.
// GLOBAL: MW2SHELL 0x1006acdc
MechS32 g_trainingTipsShown = 0;

// GLOBAL: MW2SHELL 0x10090668
ButtonMenu* g_cadetTrainingMenu;

// DrawCadetTraining's p_wParam: the quick tips show when it comes from the clan hall
// (c_msgClanHall).
// GLOBAL: MW2SHELL 0x1009066c
WPARAM g_trainingMessage;

void CadetTrainingCallback(TMPackDataBase*, MechS32*, MechU8*, MechChar**, MechS32);

// Sets up the campaign's training screen: its first button, its background and the videos of
// the trainer.
// FUNCTION: MW2SHELL 0x1003c7e0
void DrawCadetTraining(TMPackDataBase* p_database, MechS32 p_campaign, char**, WPARAM p_wParam)
{
	g_trainingMessage = p_wParam;
	g_cadetTrainingMenu =
		new ButtonMenu(g_videoDriver, g_defaultFont, 0, g_cadetTrainingScreens[p_campaign].m_buttons, 1);
	g_videoDriver->LoadBackground(p_database, g_cadetTrainingScreens[p_campaign].m_picture);

	switch (p_campaign) {
	case 0:
		PlayVideo(0, "awotrnwn", 0x1c5, 0, 0x42, 0);
		PlayVideo(3, "awotrnwa", 0x48, 0xe0, 2, 0);
		break;
	case 1:
		PlayVideo(0, "ajftrnwn", 0x1a0, 0, 0x42, 0);
		PlayVideo(3, "ajftrnwa", 0x48, 0xe0, 2, 0);
		break;
	}

	srand(clock());
	RegisterScreenFunction(CadetTrainingCallback);
}

// The training screen's frame: once the trainer's welcome is over, the mission buttons and the
// room's sound; EXIT, or a training mission after the trainer's video.
// Not 100%: the stack slots of the locals are permuted.
// FUNCTION: MW2SHELL 0x1003c966
void CadetTrainingCallback(
	TMPackDataBase* p_database,
	MechS32* p_campaign,
	MechU8*,
	MechChar** p_scenario,
	MechS32 p_msg
)
{
	void* data = NULL;
	MechS32 i;
	MechS32 button;
	MechS32 size;

	// The original skips the frame's work with a goto, like StarConfigCallback.
	if (p_msg != c_msgScreenFrame) {
		goto done;
	}

	if (g_quickTips && !g_trainingTipsShown && g_trainingMessage == c_msgClanHall && g_trainingButtonsShown) {
		DialogBoxParam(g_module, MAKEINTRESOURCE(0x68), g_gameWindow, (DLGPROC) OkDialogProc, 0);
		g_trainingTipsShown = 1;
	}

	if (!g_trainingButtonsShown && !IsVideoPlaying(0)) {
		g_trainingButtonsShown = 1;
		for (i = 1; i < g_cadetTrainingScreens[*p_campaign].m_count; i++) {
			g_cadetTrainingMenu->AddButton(g_cadetTrainingScreens[*p_campaign].m_buttons[i], i, FALSE);
		}

		p_database->GetDBItem(0x4c, &data, &size);
		g_trainingAmbience = new AudioSample(g_audioSubsystem, data, size);
		g_trainingAmbience->SetVolume(0x32);
		g_trainingAmbience->EnableLoop();
		g_trainingAmbience->Start();
	}

	if (!IsVideoPlaying(3)) {
		if (!g_trainerIdleCountdown) {
			switch (*p_campaign) {
			case 0:
				PlayVideo(3, g_trainerTake ? "awotrnwa" : "awotrnwb", 0x48, 0xe0, 2, 0);
				g_trainerTake = 1 - g_trainerTake;
				break;
			case 1:
				PlayVideo(3, g_trainerTake ? "ajftrnwa" : "ajftrnwb", 0x48, 0xe0, 2, 0);
				g_trainerTake = 1 - g_trainerTake;
				break;
			}
		}

		if (--g_trainerIdleCountdown < 0) {
			g_trainerIdleCountdown = 20000;
		}
	}

	if (g_trainingExitVideo == -1) {
		button = g_cadetTrainingMenu->HitTest(g_mouseState->m_x, g_mouseState->m_y);
		switch (button) {
		case 0:
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			p_msg = c_msgClanHall;
			break;
		default:
			if (button == -1 || g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			*p_scenario = g_trainingScenarios[*p_campaign][button - 1];
			ShellApplyMissionUiInfo(*p_scenario, 0, 0);
			delete g_trainingAmbience;
			g_trainingAmbience = NULL;
			switch (*p_campaign) {
			case 0:
				g_trainingExitVideo = PlayVideo(0x10, "awotrndr", 0x230, 0xa8, 2, 0);
				break;
			case 1:
				g_trainingExitVideo = PlayVideo(0x10, "ajftrndr", 0x21c, 0xa8, 2, 0);
				break;
			}
			break;
		}
	}
	else if (!IsVideoPlaying(g_trainingExitVideo)) {
		g_trainingExitVideo = -1;
		p_msg = c_msgLaunchSim;
	}

done:
	if (p_msg != c_msgScreenFrame) {
		CloseAllVideos();
		delete g_cadetTrainingMenu;
		delete g_trainingAmbience;
		g_trainingAmbience = NULL;
		g_trainingTipsShown = 0;
		g_trainerTake = 0;
		g_trainerIdleCountdown = -1;
		g_trainingButtonsShown = 0;
		MechPostMessage(p_msg, c_msgCadetTraining, 0);
		UnregisterScreenFunction(CadetTrainingCallback);
	}
}
