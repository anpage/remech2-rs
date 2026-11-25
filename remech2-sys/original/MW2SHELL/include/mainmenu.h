#ifndef MAINMENU_H
#define MAINMENU_H

#include "audiosample.h"
#include "buttonmenu.h"
#include "tmpackdatabase.h"
#include "types.h"

// The functions and globals of mainmenu.cpp that other units use.
extern char g_mainMenuLogoVideo[9];
void* AllocateAllowNew(int32_t p_size);
extern ButtonMenu* g_mainMenu;
extern AudioSample* g_mainMenuMusic;
extern AudioSample* g_mainMenuIntro;
extern int32_t g_mainMenuMusicStarted;

void DrawMainMenu(TMPackDataBase* p_database, int32_t*);
// The main menu's frame, implemented on the Rust side (src/shell/screens/main_menu.rs)
extern "C" void MainMenuCallback(
	TMPackDataBase* p_database,
	int32_t* p_campaign,
	uint8_t* p_pilotChosen,
	char** p_scenario,
	int32_t p_msg
);

#endif // MAINMENU_H
