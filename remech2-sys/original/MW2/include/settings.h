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
	extern char g_gameCtrlItem[10];
	extern char g_systemsStatusTitle[15];
	extern char g_lightAmplificationItem[20];
	extern char g_imageEnhancementItem[18];
	extern char g_hudItem[4];
	extern char g_autoThermOverrideItem[20];
	extern char g_autoEjectItem[11];
	extern MenuControl g_infraredControl;
	extern MenuControl g_enhancedVisionControl;
	extern MenuControl g_hudControl;
	extern MenuControl g_overrideShutdownControl;
	extern MenuControl g_autoEjectControl;
	extern MenuPage g_systemsStatusPage;
	extern PANE g_systemsMenuTarget;
	extern PANE g_systemsMenuBackgroundTarget;

	int32_t GetSystemSetting(int32_t p_id);
	void SetSystemSetting(int32_t p_id, int32_t p_value);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H
