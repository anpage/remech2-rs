#ifndef COMMANDMENU_H
#define COMMANDMENU_H

#include "menu.h"
#include "menupage.h"
#include "types.h"

// The globals of commandmenu.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MenuDefinition g_commandPoint2Menu;
	extern MenuPage g_commandPoint2Page;
	extern MenuPage g_commandPoint3Page;
	extern MenuDefinition g_commandMenu;
	extern MenuPage* g_commandMenuPageStack[8];
	extern MenuPage* g_commandPoint2MenuPageStack[8];

#ifdef __cplusplus
}
#endif

#endif // COMMANDMENU_H
