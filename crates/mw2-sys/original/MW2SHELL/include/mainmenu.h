#ifndef MAINMENU_H
#define MAINMENU_H

#include "audiosample.h"
#include "buttonmenu.h"
#include "tmpackdatabase.h"
#include "types.h"

// The functions and globals of mainmenu.cpp that other units use.
extern MechChar g_mainMenuLogoVideo[9];
void* AllocateAllowNew(MechS32 p_size);
extern ButtonMenu* g_mainMenu;
extern AudioSample* g_mainMenuMusic;
extern AudioSample* g_mainMenuIntro;
extern MechS32 g_mainMenuMusicStarted;

void DrawMainMenu(TMPackDataBase* p_database, MechS32*);
// The main menu's frame, implemented on the Rust side (src/shell/screens/main_menu.rs)
extern "C" void MainMenuCallback(
	TMPackDataBase* p_database,
	MechS32* p_campaign,
	MechU8* p_pilotChosen,
	char** p_scenario,
	MechS32 p_msg
);

#endif // MAINMENU_H
