#include "settings.h"

#include "environment.h"
#include "gamekeys.h"
#include "hud.h"
#include "mainmenu.h"
#include "mechdamage.h"
#include "menu.h"
#include "menucontrol.h"
#include "menucontrols.h"
#include "menupage.h"
#include "polydraw.h"
#include "rendersettings.h"
#include "speech.h"
#include "targeting.h"
#include "types.h"

char g_gameCtrlItem[] = "Game Ctrl";

char g_systemsStatusTitle[] = "SYSTEMS STATUS";

char g_lightAmplificationItem[] = "Light Amplification";

char g_imageEnhancementItem[] = "Image Emhancement";

char g_hudItem[] = "HUD";

char g_autoThermOverrideItem[] = "Auto Therm Override";

char g_autoEjectItem[] = "Auto Eject";

MenuControl g_infraredControl = {3, 0, &g_offOnChoices, 166, NULL, GetSystemSetting, NULL, SetSystemSetting, NULL};

MenuControl g_enhancedVisionControl =
	{3, 0, &g_offOnChoices, 167, NULL, GetSystemSetting, NULL, SetSystemSetting, NULL};

MenuControl g_hudControl = {3, 0, &g_offOnChoices, 19, NULL, GetSystemSetting, NULL, SetSystemSetting, NULL};

MenuControl g_overrideShutdownControl =
	{3, 0, &g_offOnChoices, 64, NULL, GetSystemSetting, NULL, SetSystemSetting, NULL};

MenuControl g_autoEjectControl = {3, 0, &g_offOnChoices, 60, NULL, GetSystemSetting, NULL, SetSystemSetting, NULL};

MenuPage g_systemsStatusPage = {
	0,
	g_systemsStatusTitle,
	0,
	5,
	0,
	NULL,
	{{4, g_lightAmplificationItem, RunMenuChoice, &g_infraredControl, NULL},
	 {4, g_imageEnhancementItem, RunMenuChoice, &g_enhancedVisionControl, NULL},
	 {4, g_hudItem, RunMenuChoice, &g_hudControl, NULL},
	 {4, g_autoEjectItem, RunMenuChoice, &g_autoEjectControl, NULL},
	 {6, g_escToExitText, NULL, NULL, NULL}}
};

PANE g_systemsMenuTarget = {NULL, 0x8ccd, 0x547b, 0x10000, 0x9eb8};

PANE g_systemsMenuBackgroundTarget = {NULL, 0x8ccd, 0x547b, 0x10000, 0x9eb8};

MenuDefinition g_systemsMenu = {
	&g_systemsMenuTarget,
	2,
	g_systemsMenuPageStack,
	0,
	-1,
	NULL,
	&g_systemsMenuBackgroundTarget,
	-1,
	NULL,
	225,
	219,
	1,
	NULL,
	14,
	14,
	6,
	{0, 0},
	{0, 0},
	{0x51f, 0},
	{0x51f, 0},
	{0xcccd, 0},
	&g_systemsStatusPage
};

MenuPage* g_systemsMenuPageStack[8];

// The systems status menu's settings, by the game key that toggles each (GAMEKEY.MAP): 0x13
// TOGGLE_HUD, 0x3c TOGGLE_AUTOEJECT, 0x40 OVERRIDE_SHUTDOWN, 0xa6 INFRARED (light amplification)
// and 0xa7 ENHANCED_VISION (image enhancement: outlined polygons).

// Returns the setting p_id, or 0.
int32_t GetSystemSetting(int32_t p_id)
{
	int32_t value;

	switch (p_id) {
	case 0xa6:
		value = IsInfraredOn();
		break;
	case 0xa7:
		if (g_renderSettings.m_wireframe == 1) {
			value = 1;
		}
		else {
			value = 0;
		}
		break;
	case 0x13:
		value = g_showHud;
		break;
	case 0x40:
		value = g_overrideShutdown;
		break;
	case 0x3c:
		value = g_autoEject;
		break;
	default:
		value = 0;
		break;
	}

	return value;
}

// Changes the setting p_id (see GetSystemSetting) to p_value.
void SetSystemSetting(int32_t p_id, int32_t p_value)
{
	switch (p_id) {
	case 0xa6:
		SetInfrared(p_value);
		break;
	case 0xa7:
		if (p_value) {
			g_renderSettings.m_wireframe = 1;
			PlayCockpitSound(0x1b, 1);
		}
		else {
			g_renderSettings.m_wireframe = 0;
		}
		break;
	case 0x13:
		g_showHud = p_value;
		break;
	case 0x40:
		g_overrideShutdown = p_value;
		break;
	case 0x3c:
		g_autoEject = p_value;
		break;
	default:
		break;
	}
}
