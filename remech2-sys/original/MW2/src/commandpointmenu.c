/* The menu of command point 3 (menu 8), the lance mate the third command key commands. A
   data-only object: its data follows bandpoly.c's. */
#include "commandpointmenu.h"

#include "commandmenu.h"
#include "menu.h"
#include "menupage.h"
#include "targeting.h"
#include "types.h"

#include <stddef.h>

PANE g_commandPoint3MenuTarget = {NULL, 0x3852, 0x4ccd, 0x10000, 0x999a};

PANE g_commandPoint3MenuBackgroundTarget = {NULL, 0x3852, 0x4ccd, 0x10000, 0x999a};

MenuDefinition g_commandPoint3Menu = {
	&g_commandPoint3MenuTarget,
	10,
	g_commandPoint3MenuPageStack,
	0,
	-1,
	NULL,
	&g_commandPoint3MenuBackgroundTarget,
	-1,
	NULL,
	225,
	219,
	1,
	NULL,
	14,
	14,
	8,
	{0, 0},
	{0, 0},
	{0x51f, 0},
	{0x51f, 0},
	{0x6666, 0},
	&g_commandPoint3Page
};

MenuPage* g_commandPoint3MenuPageStack[8];
