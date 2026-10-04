#ifndef LANCEMENU_H
#define LANCEMENU_H

#include "decomp.h"
#include "menu.h"
#include "menuchoices.h"
#include "menucontrol.h"
#include "menupage.h"
#include "types.h"

// The functions and globals of lancemenu.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_lanceOrders[8];

	MechS32 PrepareCommandComputerPage(MenuDefinition* p_menu, MenuPage* p_page);
	MechS32 PrepareCommandPointPage(MenuDefinition* p_menu, MenuPage* p_page);
	MechS32 GetLanceOrder(MechS32 p_index);
	MechS32 GetFormation(MechS32 p_arg);
	void SelectFormation(MechS32 p_formation, MechS32 p_value);
	MechS32 GetSlotAiState(MechS32 p_index);
	void SetControlSlot(MenuPage* p_page, MenuControl* p_control);
	void InstallGoalSuffix(MenuPage* p_page, MenuControl* p_control);
	void OrderAttack(MechS32 p_index, MechS32 p_value);
	void OrderEngageAtWill(MechS32 p_index, MechS32 p_value);
	void OrderJoinFormation(MechS32 p_index, MechS32 p_value);
	void OrderDefend(MechS32 p_index, MechS32 p_value);
	void OrderDisengage(MechS32 p_index, MechS32 p_value);
	void OrderShutdown(MechS32 p_index, MechS32 p_value);
	MechChar* GetSlotGoalName(
		MenuDefinition* p_menu,
		MenuControl* p_control,
		MechS32 p_index,
		Point p_pos,
		MenuPage* p_page
	);

#ifdef __cplusplus
}
#endif

#endif // LANCEMENU_H
