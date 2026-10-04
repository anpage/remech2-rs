#include "mainmenu.h"

#include "audiosample.h"
#include "audiosubsystem.h"
#include "buttonmenu.h"
#include "campaignmission.h"
#include "decomp.h"
#include "font.h"
#include "mainmenubutton.h"
#include "mechvariant.h"
#include "menudata.h"
#include "menuscreen.h"
#include "messages.h"
#include "mousestate.h"
#include "mss.h"
#include "options.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"

#include <stddef.h>

void* operator new(size_t);

// GLOBAL: MW2SHELL 0x1006ae74
ButtonMenu* g_mainMenu = NULL;

// The main menu's music, started once the intro sound (g_mainMenuIntro) is over, and fading in.
// GLOBAL: MW2SHELL 0x1006ae78
AudioSample* g_mainMenuMusic = NULL;

// Database item 104, played when the menu opens.
// GLOBAL: MW2SHELL 0x1006ae7c
AudioSample* g_mainMenuIntro = NULL;

// GLOBAL: MW2SHELL 0x1006ae80
MechS32 g_mainMenuMusicStarted = 0;

// GLOBAL: MW2SHELL 0x1006ae84
MechChar g_mainMenuLogoVideo[] = "amwlogo1";

// The original 0x10049c60 is the CRT operator new, already annotated in library_msvc.h.
void* AllocateAllowNew(MechS32 p_size)
{
	return ::operator new(p_size);
}

void MainMenuCallback(TMPackDataBase*, MechS32*, MechU8*, MechChar**, MechS32);

// FUNCTION: MW2SHELL 0x1003dc10
void DrawMainMenu(TMPackDataBase* p_database, MechS32*)
{
	void* audioData = NULL;
	MechS32 audioSize;

	SelectStar(1, 0, 0, 0, 100);
	p_database->GetDBItem(104, &audioData, &audioSize);
	g_mainMenuIntro = new AudioSample(g_audioSubsystem, audioData, audioSize);

	g_videoDriver->LoadBackground(p_database, 1);
	g_mainMenu = new ButtonMenu(g_videoDriver, g_defaultFont, 0, g_mainMenuButtons, 3);

	PlayVideoInFreeSlot(g_mainMenuLogoVideo, 0x6f, 0x21, 10, 0);
	g_mainMenuIntro->SetVolume(0x78);
	g_mainMenuIntro->Start();
	RegisterScreenFunction(MainMenuCallback);
}

// The main menu's frame: the trials of grievance, the two clan halls and EXIT.
// Not 100%: the stack slots of data, button and size are permuted.
// FUNCTION: MW2SHELL 0x1003dd89
void MainMenuCallback(TMPackDataBase* p_database, MechS32* p_campaign, MechU8*, MechChar**, MechS32 p_msg)
{
	void* data = NULL;
	MechS32 button;
	MechS32 size;

	AIL_serve();

	// The original skips the frame's work with a goto, like StarConfigCallback.
	if (p_msg != c_msgScreenFrame) {
		goto done;
	}

	if (!g_mainMenuMusicStarted && !g_mainMenuIntro->IsPlaying()) {
		p_database->GetDBItem(0x4a, &data, &size);
		g_mainMenuMusic = new AudioSample(g_audioSubsystem, data, size);
		g_mainMenuMusic->EnableLoop();
		g_mainMenuMusic->Start();
		g_mainMenuMusic->SetFade(500, 1000, 0, 0x1e);
		g_mainMenuMusicStarted = 1;
	}
	else if (g_mainMenuMusicStarted) {
		g_mainMenuMusic->DoFade();
	}

	button = g_mainMenu->HitTest(g_mouseState->m_x, g_mouseState->m_y);
	switch (button) {
	case 0:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		p_msg = c_msgTrials;
		*p_campaign = 2;
		break;
	case 1:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		p_msg = c_msgLandingVideo;
		*p_campaign = 0;
		break;
	case 2:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		p_msg = c_msgLandingVideo;
		*p_campaign = 1;
		break;
	case 3:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		if (!ShowDialog("Embrace cowardice?#Yes|No", 1)) {
			p_msg = c_msgQuit;
		}
		break;
	default:
		break;
	}

done:
	if (p_msg != c_msgScreenFrame) {
		CloseAllVideos();
		delete g_mainMenu;
		g_mainMenu = NULL;
		delete g_mainMenuMusic;
		g_mainMenuMusic = NULL;
		delete g_mainMenuIntro;
		g_mainMenuIntro = NULL;
		g_mainMenuMusicStarted = 0;
		MechPostMessage(p_msg, c_msgMainMenu, 0);
		UnregisterScreenFunction(MainMenuCallback);
	}
}
