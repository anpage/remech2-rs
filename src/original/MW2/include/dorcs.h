#ifndef DORCS_H
#define DORCS_H

#include "decomp.h"
#include "menu.h"
#include "menucontrol.h"
#include "menupage.h"
#include "point.h"
#include "types.h"

// The functions of dorcs.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_fledToWindows;
	extern void (*g_dorcsPreviousDrawCallback)(void);
	extern MenuDefinition g_dorcsMenu;
	extern MenuPage* g_dorcsMenuPageStack[8];

	void AbortMissionAction(
		MenuDefinition* p_menu,
		MenuControl* p_control,
		MechS32 p_index,
		Point p_pos,
		MenuPage* p_page
	);
	void FleeToWindowsAction(
		MenuDefinition* p_menu,
		MenuControl* p_control,
		MechS32 p_index,
		Point p_pos,
		MenuPage* p_page
	);
	void* ReadVfxBin(MechChar* p_name);
	void CloseInGameMenus(void);
	void UpdateDorcs(void);
	void ShowDorcs(void);

#ifdef __cplusplus
}
#endif

#endif // DORCS_H
