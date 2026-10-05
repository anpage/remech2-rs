#ifndef MAINMENU_H
#define MAINMENU_H

#include "menu.h"
#include "menuchoices.h"
#include "menucontrol.h"
#include "menupage.h"
#include "types.h"

// The globals of mainmenu.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechChar g_acceptText[23];
	extern MechChar g_escToExitText[14];
	extern MenuChoices g_offOnChoices;
	extern MenuChoices g_lowHighChoices;
	extern MechS32 g_sliderShapes[8];
	extern MenuDefinition g_mainMenu;
	extern MenuPage* g_mainMenuPageStack[8];
	extern MechChar g_mainMenuTitle[10];
	extern MechChar g_abortMissionItem[14];
	extern MechChar g_monitorBrightnessItem[19];
	extern MechChar g_fleeToWindowsItem[16];
	extern MechChar g_fleeToWindowsTitle[16];
	extern MechChar g_abortMissionTitle[14];
	extern MechChar g_monitorBrightnessTitle[19];
	extern MechChar g_areYouSureText[14];
	extern MechChar g_confirmCowardiceText[23];
	extern MechChar g_confirmationRequestedText[23];
	extern MechChar g_noText[3];
	extern MechChar g_yesText[4];
	extern MechChar g_offText[4];
	extern MechChar g_onText[3];
	extern MechChar g_lowText[4];
	extern MechChar g_mediumText[7];
	extern MechChar g_highText[5];
	extern MenuChoices g_noYesChoices;
	extern MenuChoices g_offLowMediumHighChoices;
	extern MenuControl g_confirmControl;
	extern MenuControl g_brightnessControl;
	extern MenuPage g_abortMissionPage;
	extern MenuPage g_brightnessPage;
	extern MenuPage g_fleePage;
	extern MenuPage g_mainMenuPage;
	extern PANE g_mainMenuTarget;
	extern PANE g_mainMenuBackgroundTarget;

#ifdef __cplusplus
}
#endif

#endif // MAINMENU_H
