#ifndef DORCS_H
#define DORCS_H

#include "menu.h"
#include "menucontrol.h"
#include "menupage.h"
#include "menutextbox.h"
#include "palettecolor.h"
#include "point.h"
#include "recttransition.h"
#include "types.h"

// The functions of dorcs.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_fledToWindows;
	extern void (*g_dorcsPreviousDrawCallback)(void);
	extern MenuDefinition g_dorcsMenu;
	extern MenuPage* g_dorcsMenuPageStack[8];
	extern char g_dorcsPageTitle[26];
	extern char g_dorcsAboutItem[12];
	extern char g_dorcsClarkeName[15];
	extern char g_dorcsDouglasName[19];
	extern char g_dorcsEthertonName[18];
	extern char g_dorcsHusebyName[17];
	extern char g_dorcsKaminsName[11];
	extern char g_dorcsKeatingName[13];
	extern char g_dorcsMilesName[11];
	extern char g_dorcsMortenName[11];
	extern char g_dorcsMortensenName[14];
	extern char g_dorcsPetersonName[14];
	extern char g_dorcsStanfillName[13];
	extern char g_dorcsWhiteName[12];
	extern char g_dorcsZobelName[11];
	extern char g_dorcsAboutText[569];
	extern char g_dorcsClarkeText[669];
	extern char g_dorcsDouglasText[643];
	extern char g_dorcsEthertonText[49];
	extern char g_dorcsHusebyText[286];
	extern char g_dorcsKaminsText[251];
	extern char g_dorcsKeatingText[509];
	extern char g_dorcsMilesText[367];
	extern char g_dorcsMortenText[300];
	extern char g_dorcsMortensenText[383];
	extern char g_dorcsPetersonText[400];
	extern char g_dorcsStanfillText[327];
	extern char g_dorcsWhiteText[49];
	extern char g_dorcsZobelText[85];
	extern char g_dorcsExitItem[16];
	extern PANE g_dorcsTextRect;
	extern MenuTextBox g_dorcsAboutTextBox;
	extern MenuTextBox g_dorcsClarkeTextBox;
	extern MenuTextBox g_dorcsDouglasTextBox;
	extern MenuTextBox g_dorcsEthertonTextBox;
	extern MenuTextBox g_dorcsHusebyTextBox;
	extern MenuTextBox g_dorcsKaminsTextBox;
	extern MenuTextBox g_dorcsKeatingTextBox;
	extern MenuTextBox g_dorcsMilesTextBox;
	extern MenuTextBox g_dorcsMortenTextBox;
	extern MenuTextBox g_dorcsMortensenTextBox;
	extern MenuTextBox g_dorcsPetersonTextBox;
	extern MenuTextBox g_dorcsStanfillTextBox;
	extern MenuTextBox g_dorcsWhiteTextBox;
	extern MenuTextBox g_dorcsZobelTextBox;
	extern MenuControl g_dorcsAboutControl;
	extern MenuControl g_dorcsClarkeControl;
	extern MenuControl g_dorcsDouglasControl;
	extern MenuControl g_dorcsEthertonControl;
	extern MenuControl g_dorcsHusebyControl;
	extern MenuControl g_dorcsKaminsControl;
	extern MenuControl g_dorcsKeatingControl;
	extern MenuControl g_dorcsMilesControl;
	extern MenuControl g_dorcsMortenControl;
	extern MenuControl g_dorcsMortensenControl;
	extern MenuControl g_dorcsPetersonControl;
	extern MenuControl g_dorcsStanfillControl;
	extern MenuControl g_dorcsWhiteControl;
	extern MenuControl g_dorcsZobelControl;
	extern MenuPage g_dorcsAboutPage;
	extern MenuPage g_dorcsClarkePage;
	extern MenuPage g_dorcsDouglasPage;
	extern MenuPage g_dorcsEthertonPage;
	extern MenuPage g_dorcsHusebyPage;
	extern MenuPage g_dorcsKaminsPage;
	extern MenuPage g_dorcsKeatingPage;
	extern MenuPage g_dorcsMilesPage;
	extern MenuPage g_dorcsMortenPage;
	extern MenuPage g_dorcsMortensenPage;
	extern MenuPage g_dorcsPetersonPage;
	extern MenuPage g_dorcsStanfillPage;
	extern MenuPage g_dorcsWhitePage;
	extern MenuPage g_dorcsZobelPage;
	extern MenuPage g_dorcsPage;
	extern PANE g_dorcsMenuTarget;
	extern PANE g_dorcsMenuBackgroundTarget;
	extern PANE g_dorcsPoint;
	extern PANE g_dorcsRectFrom;
	extern PANE g_dorcsRectTo;
	extern PANE g_dorcsRect;
	extern RectTransitionState g_dorcsTransitionState;
	extern RectTransitionDef g_dorcsTransitionDef;
	extern RectTransition g_dorcsTransition;
	extern void* g_dorcsGif;
	extern PaletteColor* g_dorcsPalette;
	extern uint8_t* g_dorcsGifState;
	extern int32_t g_dorcsGifLoaded;
	extern int32_t g_dorcsTime;
	extern int32_t g_dorcsReverse;
	extern int32_t g_dorcsState;
	extern PANE g_dorcsSavedTarget;
	extern PANE g_dorcsGifTarget;
	extern PaletteColor g_dorcsBlack;

	void AbortMissionAction(
		MenuDefinition* p_menu,
		MenuControl* p_control,
		int32_t p_index,
		Point p_pos,
		MenuPage* p_page
	);
	void FleeToWindowsAction(
		MenuDefinition* p_menu,
		MenuControl* p_control,
		int32_t p_index,
		Point p_pos,
		MenuPage* p_page
	);
	void* ReadVfxBin(char* p_name);
	void CloseInGameMenus(void);
	void UpdateDorcs(void);
	void ShowDorcs(void);

#ifdef __cplusplus
}
#endif

#endif // DORCS_H
