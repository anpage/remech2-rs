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

// GLOBAL: MW2 0x100a1a90
MechChar g_mainMenuTitle[] = "MAIN MENU";

// GLOBAL: MW2 0x100a1aa0
MechChar g_abortMissionItem[] = "Abort Mission";

// GLOBAL: MW2 0x100a1ab0
MechChar g_monitorBrightnessItem[] = "Monitor Brightness";

// GLOBAL: MW2 0x100a1ac8
MechChar g_fleeToWindowsItem[] = "Flee to Windows";

// GLOBAL: MW2 0x100a1ad8
MechChar g_acceptText[] = "Accept (Esc to cancel)";

// GLOBAL: MW2 0x100a1af0
MechChar g_escToExitText[] = "(Esc to exit)";

// GLOBAL: MW2 0x100a1b00
MechChar g_fleeToWindowsTitle[] = "FLEE TO WINDOWS";

// GLOBAL: MW2 0x100a1b10
MechChar g_abortMissionTitle[] = "ABORT MISSION";

// GLOBAL: MW2 0x100a1b20
MechChar g_monitorBrightnessTitle[] = "MONITOR BRIGHTNESS";

// GLOBAL: MW2 0x100a1b38
MechChar g_areYouSureText[] = "Are you sure?";

// GLOBAL: MW2 0x100a1b48
MechChar g_confirmCowardiceText[] = "Confirm your cowardice";

// GLOBAL: MW2 0x100a1b60
MechChar g_confirmationRequestedText[] = "Confirmation requested";

// GLOBAL: MW2 0x100a1b78
MechChar g_noText[] = "No";

// GLOBAL: MW2 0x100a1b7c
MechChar g_yesText[] = "Yes";

// GLOBAL: MW2 0x100a1b80
MechChar g_offText[] = "Off";

// GLOBAL: MW2 0x100a1b84
MechChar g_onText[] = "On";

// GLOBAL: MW2 0x100a1b88
MechChar g_lowText[] = "Low";

// GLOBAL: MW2 0x100a1b90
MechChar g_mediumText[] = "Medium";

// GLOBAL: MW2 0x100a1b98
MechChar g_highText[] = "High";

// GLOBAL: MW2 0x100a1ba0
MenuChoices g_offOnChoices = {NULL, 2, {g_offText, g_onText}};

// GLOBAL: MW2 0x100a1be8
MenuChoices g_noYesChoices = {NULL, 2, {g_noText, g_yesText}};

// GLOBAL: MW2 0x100a1c30
MenuChoices g_lowHighChoices = {NULL, 2, {g_lowText, g_highText}};

// GLOBAL: MW2 0x100a1c78
MenuChoices g_offLowMediumHighChoices = {NULL, 4, {g_offText, g_lowText, g_mediumText, g_highText}};

// GLOBAL: MW2 0x100a1cc0
MechS32 g_sliderShapes[8] = {67, 0, 64, 0, 70, 0, 82, 0};

// GLOBAL: MW2 0x100a1ce0
MenuControl g_confirmControl = {2, 0, NULL, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100a1d08
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

// GLOBAL: MW2 0x100a1d30
MenuPage g_abortMissionPage = {
	0,
	g_abortMissionTitle,
	0,
	2,
	0,
	NULL,
	{{3, g_confirmationRequestedText, NULL, NULL, NULL}, {1, g_acceptText, AbortMissionAction, &g_confirmControl, NULL}}
};

// GLOBAL: MW2 0x100a1e88
MenuPage g_brightnessPage = {
	0,
	g_monitorBrightnessTitle,
	0,
	2,
	0,
	NULL,
	{{1, g_monitorBrightnessItem, RunMenuSlider, &g_brightnessControl, NULL}, {2, g_acceptText, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100a1fe0
MenuPage g_fleePage = {
	0,
	g_fleeToWindowsTitle,
	0,
	2,
	0,
	NULL,
	{{3, g_confirmCowardiceText, NULL, NULL, NULL}, {1, g_acceptText, FleeToWindowsAction, &g_confirmControl, NULL}}
};

// GLOBAL: MW2 0x100a2138
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

// GLOBAL: MW2 0x100a2290
PANE g_mainMenuTarget = {NULL, 0x4000, 0x3333, 0x10000, 0xcccd};

// GLOBAL: MW2 0x100a22a8
PANE g_mainMenuBackgroundTarget = {NULL, 0, 0, 0x10000, 0x10000};

// GLOBAL: MW2 0x100a22c0
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

// GLOBAL: MW2 0x10177080
MenuPage* g_mainMenuPageStack[8];
