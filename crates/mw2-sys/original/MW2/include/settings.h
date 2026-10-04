#ifndef SETTINGS_H
#define SETTINGS_H

#include "menu.h"
#include "menupage.h"
#include "types.h"

// The functions of settings.c.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MenuDefinition g_systemsMenu;
	extern MenuPage* g_systemsMenuPageStack[8];

	MechS32 GetSystemSetting(MechS32 p_id);
	void SetSystemSetting(MechS32 p_id, MechS32 p_value);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H
