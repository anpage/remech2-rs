#ifndef MAINMENU_H
#define MAINMENU_H

#include "menu.h"
#include "menuchoices.h"
#include "menupage.h"
#include "types.h"

// The globals of mainmenu.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechChar g_acceptText[];
	extern MechChar g_escToExitText[];
	extern MenuChoices g_offOnChoices;
	extern MenuChoices g_lowHighChoices;
	extern MechS32 g_sliderShapes[8];
	extern MenuDefinition g_mainMenu;
	extern MenuPage* g_mainMenuPageStack[8];

#ifdef __cplusplus
}
#endif

#endif // MAINMENU_H
