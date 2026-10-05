#ifndef COMMANDMENU_H
#define COMMANDMENU_H

#include "menu.h"
#include "menuchoices.h"
#include "menucontrol.h"
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
	extern PANE g_commandPoint2MenuTarget;
	extern PANE g_commandPoint2MenuBackgroundTarget;
	extern MechChar g_commandComputerTitle[17];
	extern MechChar g_commandAllItem[12];
	extern MechChar g_changeFormationItem[17];
	extern MechChar g_noStarMatesText[14];
	extern MechChar g_currentFormationLabel[14];
	extern MechChar g_commandPoint2Item[16];
	extern MechChar g_commandPoint3Item[16];
	extern MechChar g_commandPoint4Item[16];
	extern MechChar g_commandPoint5Item[16];
	extern MechChar g_statusLabel[8];
	extern MechChar g_notAvailableText[14];
	extern MechChar g_commandAllTitle[12];
	extern MechChar g_changeFormationTitle[17];
	extern MechChar g_commandPoint2Title[16];
	extern MechChar g_commandPoint3Title[16];
	extern MechChar g_commandPoint4Title[16];
	extern MechChar g_commandPoint5Title[16];
	extern MechChar g_echelonLeftText[13];
	extern MechChar g_echelonRightText[14];
	extern MechChar g_lineAbreastText[13];
	extern MechChar g_lineAsternText[12];
	extern MechChar g_vFormText[7];
	extern MechChar g_wedgeText[6];
	extern MechChar g_noFormationText[13];
	extern MechChar g_orderAttackText[7];
	extern MechChar g_orderDefendText[7];
	extern MechChar g_orderJoinFormationText[15];
	extern MechChar g_orderChangeFormationText[17];
	extern MechChar g_orderDisengageText[10];
	extern MechChar g_orderEngageAtWillText[15];
	extern MechChar g_orderShutdownText[9];
	extern MechChar g_orderNoneText[7];
	extern MechChar g_attackMyTargetItem[17];
	extern MechChar g_defendMyTargetItem[17];
	extern MechChar g_joinFormationItem[15];
	extern MechChar g_disengageItem[10];
	extern MechChar g_engageAtWillItem[15];
	extern MechChar g_shutdownItem[9];
	extern MechChar g_aiStateNoneText[6];
	extern MechChar g_aiStateIdleText[6];
	extern MechChar g_aiStateAvoidText[10];
	extern MechChar g_aiStateTargetText[11];
	extern MechChar g_aiStateAttackText[10];
	extern MechChar g_aiStateFleeText[13];
	extern MechChar g_aiStateFollowText[14];
	extern MechChar g_aiStateReconText[11];
	extern MechChar g_aiStatePatrolText[11];
	extern MechChar g_aiStateGoDirectText[10];
	extern MechChar g_aiState9Text[13];
	extern MechChar g_aiStateRestText[8];
	extern MechChar g_aiStateShutdownText[10];
	extern MechChar g_aiStateDeadText[11];
	extern MenuChoices g_formationChoices;
	extern MenuChoices g_orderChoices;
	extern MenuChoices g_aiStateChoices;
	extern MenuChoices g_noChoices;
	extern MenuControl g_formationControl;
	extern MenuControl g_commandAllStatusControl;
	extern MenuControl g_commandPoint2StatusControl;
	extern MenuControl g_commandPoint3StatusControl;
	extern MenuControl g_commandPoint4StatusControl;
	extern MenuControl g_commandPoint5StatusControl;
	extern MenuControl g_echelonLeftControl;
	extern MenuControl g_echelonRightControl;
	extern MenuControl g_lineAbreastControl;
	extern MenuControl g_lineAsternControl;
	extern MenuControl g_vFormControl;
	extern MenuControl g_wedgeControl;
	extern MenuControl g_attackControl;
	extern MenuControl g_engageAtWillControl;
	extern MenuControl g_joinFormationControl;
	extern MenuControl g_defendControl;
	extern MenuControl g_disengageControl;
	extern MenuControl g_shutdownControl;
	extern MenuPage g_changeFormationPage;
	extern MenuPage g_commandAllPage;
	extern MenuPage g_commandPoint4Page;
	extern MenuPage g_commandPoint5Page;
	extern MenuPage g_commandComputerPage;
	extern PANE g_commandMenuTarget;
	extern PANE g_commandMenuBackgroundTarget;

#ifdef __cplusplus
}
#endif

#endif // COMMANDMENU_H
