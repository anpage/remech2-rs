#ifndef SETTINGS_H
#define SETTINGS_H

#include "menu.h"
#include "menucontrol.h"
#include "menupage.h"
#include "types.h"

// The functions of settings.c.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MenuDefinition g_systemsMenu;
	extern MenuPage* g_systemsMenuPageStack[8];
	extern MechChar g_gameCtrlItem[10];
	extern MechChar g_systemsStatusTitle[15];
	extern MechChar g_lightAmplificationItem[20];
	extern MechChar g_imageEnhancementItem[18];
	extern MechChar g_hudItem[4];
	extern MechChar g_autoThermOverrideItem[20];
	extern MechChar g_autoEjectItem[11];
	extern MenuControl g_infraredControl;
	extern MenuControl g_enhancedVisionControl;
	extern MenuControl g_hudControl;
	extern MenuControl g_overrideShutdownControl;
	extern MenuControl g_autoEjectControl;
	extern MenuPage g_systemsStatusPage;
	extern PANE g_systemsMenuTarget;
	extern PANE g_systemsMenuBackgroundTarget;

	MechS32 GetSystemSetting(MechS32 p_id);
	void SetSystemSetting(MechS32 p_id, MechS32 p_value);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H
