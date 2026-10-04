#ifndef MENUCONTROLS_H
#define MENUCONTROLS_H

#include "menu.h"
#include "menucontrol.h"
#include "menupage.h"
#include "point.h"
#include "types.h"

// The functions of menucontrols.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void RunMenuSlider(MenuDefinition* p_menu, MenuControl* p_control, MechS32 p_index, Point p_pos, MenuPage* p_page);
	void RunMenuStatus(MenuDefinition* p_menu, MenuControl* p_control, MechS32 p_index, Point p_pos, MenuPage* p_page);
	void RunMenuChoice(MenuDefinition* p_menu, MenuControl* p_control, MechS32 p_index, Point p_pos, MenuPage* p_page);
	void RunMenuTextBox(MenuDefinition* p_menu, MenuControl* p_control, MechS32 p_index, Point p_pos, MenuPage* p_page);

#ifdef __cplusplus
}
#endif

#endif // MENUCONTROLS_H
