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

ButtonMenu* g_mainMenu = NULL;

// The main menu's music, started once the intro sound (g_mainMenuIntro) is over, and fading in.
AudioSample* g_mainMenuMusic = NULL;

// Database item 104, played when the menu opens.
AudioSample* g_mainMenuIntro = NULL;

int32_t g_mainMenuMusicStarted = 0;

char g_mainMenuLogoVideo[] = "amwlogo1";

// The original is the CRT operator new.
void* AllocateAllowNew(int32_t p_size)
{
	return ::operator new(p_size);
}

void DrawMainMenu(TMPackDataBase* p_database, int32_t*)
{
	void* audioData = NULL;
	int32_t audioSize;

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

// MainMenuCallback, the main menu's frame, is implemented on the Rust side
// (src/shell/screens/main_menu.rs).
