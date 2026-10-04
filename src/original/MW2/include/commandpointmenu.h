#ifndef COMMANDPOINTMENU_H
#define COMMANDPOINTMENU_H

#include "menu.h"
#include "menupage.h"
#include "types.h"

// The globals of commandpointmenu.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MenuDefinition g_commandPoint3Menu;
	extern MenuPage* g_commandPoint3MenuPageStack[8];

#ifdef __cplusplus
}
#endif

#endif // COMMANDPOINTMENU_H
