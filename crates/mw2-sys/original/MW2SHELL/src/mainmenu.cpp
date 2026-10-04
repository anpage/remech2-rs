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

// MainMenuCallback, the main menu's frame, is implemented on the Rust side
// (src/shell/screens/main_menu.rs).
