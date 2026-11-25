/* The main in-mission menu (menu 4): abort the mission, the monitor brightness, the audio and
   combat settings, flee to Windows. A data-only object: its data follows network.c's. */
#include "mainmenu.h"

#include "audiomenu.h"
#include "brightnessmenu.h"
#include "dorcs.h"
#include "menu.h"
#include "menuchoices.h"
#include "menucontrol.h"
#include "menucontrols.h"
#include "menupage.h"
#include "perf.h"
#include "targeting.h"
#include "types.h"

#include <stddef.h>

MechChar g_mainMenuTitle[] = "MAIN MENU";

MechChar g_abortMissionItem[] = "Abort Mission";

MechChar g_monitorBrightnessItem[] = "Monitor Brightness";

// The original said "Windows" here and in the title
MechChar g_fleeToWindowsItem[] = "Flee to Desktop";

MechChar g_acceptText[] = "Accept (Esc to cancel)";

MechChar g_escToExitText[] = "(Esc to exit)";

MechChar g_fleeToWindowsTitle[] = "FLEE TO DESKTOP";

MechChar g_abortMissionTitle[] = "ABORT MISSION";

MechChar g_monitorBrightnessTitle[] = "MONITOR BRIGHTNESS";

MechChar g_areYouSureText[] = "Are you sure?";

MechChar g_confirmCowardiceText[] = "Confirm your cowardice";

MechChar g_confirmationRequestedText[] = "Confirmation requested";

MechChar g_noText[] = "No";

MechChar g_yesText[] = "Yes";

MechChar g_offText[] = "Off";

MechChar g_onText[] = "On";

MechChar g_lowText[] = "Low";

MechChar g_mediumText[] = "Medium";

MechChar g_highText[] = "High";

MenuChoices g_offOnChoices = {NULL, 2, {g_offText, g_onText}};

MenuChoices g_noYesChoices = {NULL, 2, {g_noText, g_yesText}};

MenuChoices g_lowHighChoices = {NULL, 2, {g_lowText, g_highText}};

MenuChoices g_offLowMediumHighChoices = {NULL, 4, {g_offText, g_lowText, g_mediumText, g_highText}};

MechS32 g_sliderShapes[8] = {67, 0, 64, 0, 70, 0, 82, 0};

MenuControl g_confirmControl = {2, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL};

MenuControl g_brightnessControl = {
	2,
	0,
	g_sliderShapes,
	0,
	NULL,
	GetBrightnessFraction,
	PreviewBrightnessFraction,
	SetBrightnessFraction,
	RestoreBrightness
};

MenuPage g_abortMissionPage = {
	0,
	g_abortMissionTitle,
	0,
	2,
	0,
	NULL,
	{{3, g_confirmationRequestedText, NULL, NULL, NULL}, {1, g_acceptText, AbortMissionAction, &g_confirmControl, NULL}}
};

MenuPage g_brightnessPage = {
	0,
	g_monitorBrightnessTitle,
	0,
	2,
	0,
	NULL,
	{{1, g_monitorBrightnessItem, RunMenuSlider, &g_brightnessControl, NULL}, {2, g_acceptText, NULL, NULL, NULL}}
};

MenuPage g_fleePage = {
	0,
	g_fleeToWindowsTitle,
	0,
	2,
	0,
	NULL,
	{{3, g_confirmCowardiceText, NULL, NULL, NULL}, {1, g_acceptText, FleeToWindowsAction, &g_confirmControl, NULL}}
};

MenuPage g_mainMenuPage = {
	0,
	g_mainMenuTitle,
	0,
	6,
	0,
	NULL,
	{{0, g_abortMissionItem, NULL, NULL, &g_abortMissionPage},
	 {0, g_monitorBrightnessItem, NULL, NULL, &g_brightnessPage},
	 {0, g_audioItem, NULL, NULL, &g_audioPage},
	 {0, g_combatVariablesItem, NULL, NULL, &g_combatVariablesPage},
	 {0, g_fleeToWindowsItem, NULL, NULL, &g_fleePage},
	 {2, g_acceptText, NULL, NULL, NULL}}
};

PANE g_mainMenuTarget = {NULL, 0x4000, 0x3333, 0x10000, 0xcccd};

PANE g_mainMenuBackgroundTarget = {NULL, 0, 0, 0x10000, 0x10000};

MenuDefinition g_mainMenu = {
	&g_mainMenuTarget,
	17,
	g_mainMenuPageStack,
	0,
	0x112,
	NULL,
	&g_mainMenuBackgroundTarget,
	-1,
	NULL,
	225,
	219,
	1,
	NULL,
	250,
	10,
	6,
	{0, 0},
	{0xa3d, 0},
	{0xa3d, 0},
	{0xa3d, 0},
	{0x7852, 0},
	&g_mainMenuPage
};

MenuPage* g_mainMenuPageStack[8];
