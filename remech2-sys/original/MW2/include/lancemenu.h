#ifndef LANCEMENU_H
#define LANCEMENU_H

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

	extern int32_t g_lanceOrders[8];

	int32_t PrepareCommandComputerPage(MenuDefinition* p_menu, MenuPage* p_page);
	int32_t PrepareCommandPointPage(MenuDefinition* p_menu, MenuPage* p_page);
	int32_t GetLanceOrder(int32_t p_index);
	int32_t GetFormation(int32_t p_arg);
	void SelectFormation(int32_t p_formation, int32_t p_value);
	int32_t GetSlotAiState(int32_t p_index);
	void SetControlSlot(MenuPage* p_page, MenuControl* p_control);
	void InstallGoalSuffix(MenuPage* p_page, MenuControl* p_control);
	void OrderAttack(int32_t p_index, int32_t p_value);
	void OrderEngageAtWill(int32_t p_index, int32_t p_value);
	void OrderJoinFormation(int32_t p_index, int32_t p_value);
	void OrderDefend(int32_t p_index, int32_t p_value);
	void OrderDisengage(int32_t p_index, int32_t p_value);
	void OrderShutdown(int32_t p_index, int32_t p_value);
	char* GetSlotGoalName(
		MenuDefinition* p_menu,
		MenuControl* p_control,
		int32_t p_index,
		Point p_pos,
		MenuPage* p_page
	);

#ifdef __cplusplus
}
#endif

#endif // LANCEMENU_H
