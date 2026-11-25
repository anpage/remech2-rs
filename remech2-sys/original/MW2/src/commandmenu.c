/* The lance command menus: the command computer (menu 1), command point 2 (menu 7) and the
   pages of each command point, the formation and the orders to all. A data-only object: its data
   follows view.c's. */
#include "commandmenu.h"

#include "lancemenu.h"
#include "mainmenu.h"
#include "menu.h"
#include "menuchoices.h"
#include "menucontrol.h"
#include "menucontrols.h"
#include "menupage.h"
#include "targeting.h"
#include "types.h"

#include <stddef.h>

PANE g_commandPoint2MenuTarget = {NULL, 0x3852, 0x4ccd, 0x10000, 0x999a};

PANE g_commandPoint2MenuBackgroundTarget = {NULL, 0x3852, 0x4ccd, 0x10000, 0x999a};

MenuDefinition g_commandPoint2Menu = {
	&g_commandPoint2MenuTarget,
	10,
	g_commandPoint2MenuPageStack,
	0,
	-1,
	NULL,
	&g_commandPoint2MenuBackgroundTarget,
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
	&g_commandPoint2Page
};

MechChar g_commandComputerTitle[] = "COMMAND COMPUTER";

MechChar g_commandAllItem[] = "Command All";

MechChar g_changeFormationItem[] = "Change Formation";

MechChar g_noStarMatesText[] = "NO STAR MATES";

MechChar g_currentFormationLabel[] = "Current Form:";

MechChar g_commandPoint2Item[] = "Command Point 2";

MechChar g_commandPoint3Item[] = "Command Point 3";

MechChar g_commandPoint4Item[] = "Command Point 4";

MechChar g_commandPoint5Item[] = "Command Point 5";

MechChar g_statusLabel[] = "Status:";

MechChar g_notAvailableText[] = "NOT AVAILABLE";

MechChar g_commandAllTitle[] = "COMMAND ALL";

MechChar g_changeFormationTitle[] = "CHANGE FORMATION";

MechChar g_commandPoint2Title[] = "COMMAND POINT 2";

MechChar g_commandPoint3Title[] = "COMMAND POINT 3";

MechChar g_commandPoint4Title[] = "COMMAND POINT 4";

MechChar g_commandPoint5Title[] = "COMMAND POINT 5";

MechChar g_echelonLeftText[] = "Echelon Left";

MechChar g_echelonRightText[] = "Echelon Right";

MechChar g_lineAbreastText[] = "Line Abreast";

MechChar g_lineAsternText[] = "Line Astern";

MechChar g_vFormText[] = "V Form";

MechChar g_wedgeText[] = "Wedge";

MechChar g_noFormationText[] = "No Formation";

MechChar g_orderAttackText[] = "Attack";

MechChar g_orderDefendText[] = "Defend";

MechChar g_orderJoinFormationText[] = "Join Formation";

MechChar g_orderChangeFormationText[] = "Change Formation";

MechChar g_orderDisengageText[] = "Disengage";

MechChar g_orderEngageAtWillText[] = "Engage at Will";

MechChar g_orderShutdownText[] = "Shutdown";

MechChar g_orderNoneText[] = "No Cmd";

MechChar g_attackMyTargetItem[] = "Attack My Target";

MechChar g_defendMyTargetItem[] = "Defend My Target";

MechChar g_joinFormationItem[] = "Join Formation";

MechChar g_disengageItem[] = "Disengage";

MechChar g_engageAtWillItem[] = "Engage at Will";

MechChar g_shutdownItem[] = "Shutdown";

MechChar g_aiStateNoneText[] = "None ";

MechChar g_aiStateIdleText[] = "Idle ";

MechChar g_aiStateAvoidText[] = "Avoiding ";

MechChar g_aiStateTargetText[] = "Targeting ";

MechChar g_aiStateAttackText[] = "Engaging ";

MechChar g_aiStateFleeText[] = "Disengaging ";

MechChar g_aiStateFollowText[] = "In Formation ";

MechChar g_aiStateReconText[] = "Reconning ";

MechChar g_aiStatePatrolText[] = "Defending ";

MechChar g_aiStateGoDirectText[] = "En Route ";

MechChar g_aiState9Text[] = "Disengaging ";

MechChar g_aiStateRestText[] = "Silent ";

MechChar g_aiStateShutdownText[] = "Shutdown ";

MechChar g_aiStateDeadText[] = "Destroyed ";

MenuChoices g_formationChoices = {
	NULL,
	7,
	{g_echelonLeftText,
	 g_echelonRightText,
	 g_lineAbreastText,
	 g_lineAsternText,
	 g_vFormText,
	 g_wedgeText,
	 g_noFormationText}
};

// The orders, by g_lanceOrders.
MenuChoices g_orderChoices = {
	NULL,
	8,
	{g_orderChangeFormationText,
	 g_orderEngageAtWillText,
	 g_orderAttackText,
	 g_orderJoinFormationText,
	 g_orderDefendText,
	 g_orderDisengageText,
	 g_orderShutdownText,
	 g_orderNoneText}
};

// The AI states, by state plus one (GetSlotAiState), each followed by the player's goal.
MenuChoices g_aiStateChoices = {
	GetSlotGoalName,
	14,
	{g_aiStateNoneText,
	 g_aiStateIdleText,
	 g_aiStateAvoidText,
	 g_aiStateTargetText,
	 g_aiStateAttackText,
	 g_aiStateFleeText,
	 g_aiStateFollowText,
	 g_aiStateReconText,
	 g_aiStatePatrolText,
	 g_aiStateGoDirectText,
	 g_aiState9Text,
	 g_aiStateRestText,
	 g_aiStateShutdownText,
	 g_aiStateDeadText}
};

MenuChoices g_noChoices = {NULL, 0};

MenuControl g_formationControl = {3, 0, &g_formationChoices, 0, NULL, GetFormation, NULL, NULL, NULL};

MenuControl g_commandAllStatusControl = {3, 0, &g_orderChoices, 0, NULL, GetLanceOrder, NULL, NULL, NULL};

MenuControl g_commandPoint2StatusControl =
	{3, 0, &g_aiStateChoices, 1, InstallGoalSuffix, GetSlotAiState, NULL, NULL, NULL};

MenuControl g_commandPoint3StatusControl =
	{3, 0, &g_aiStateChoices, 2, InstallGoalSuffix, GetSlotAiState, NULL, NULL, NULL};

MenuControl g_commandPoint4StatusControl =
	{3, 0, &g_aiStateChoices, 3, InstallGoalSuffix, GetSlotAiState, NULL, NULL, NULL};

MenuControl g_commandPoint5StatusControl =
	{3, 0, &g_aiStateChoices, 4, InstallGoalSuffix, GetSlotAiState, NULL, NULL, NULL};

MenuControl g_echelonLeftControl = {2, 0, &g_noChoices, 0, NULL, NULL, NULL, SelectFormation, NULL};

MenuControl g_echelonRightControl = {2, 0, &g_noChoices, 1, NULL, NULL, NULL, SelectFormation, NULL};

MenuControl g_lineAbreastControl = {2, 0, &g_noChoices, 2, NULL, NULL, NULL, SelectFormation, NULL};

MenuControl g_lineAsternControl = {2, 0, &g_noChoices, 3, NULL, NULL, NULL, SelectFormation, NULL};

MenuControl g_vFormControl = {2, 0, &g_noChoices, 4, NULL, NULL, NULL, SelectFormation, NULL};

MenuControl g_wedgeControl = {2, 0, &g_noChoices, 5, NULL, NULL, NULL, SelectFormation, NULL};

MenuControl g_attackControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderAttack, NULL};

MenuControl g_engageAtWillControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderEngageAtWill, NULL};

MenuControl g_joinFormationControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderJoinFormation, NULL};

MenuControl g_defendControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderDefend, NULL};

MenuControl g_disengageControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderDisengage, NULL};

MenuControl g_shutdownControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderShutdown, NULL};

MenuPage g_changeFormationPage = {
	0,
	g_changeFormationTitle,
	0,
	8,
	0,
	NULL,
	{{3, g_currentFormationLabel, RunMenuStatus, &g_formationControl, NULL},
	 {5, g_echelonLeftText, RunMenuStatus, &g_echelonLeftControl, NULL},
	 {5, g_echelonRightText, RunMenuStatus, &g_echelonRightControl, NULL},
	 {5, g_lineAbreastText, RunMenuStatus, &g_lineAbreastControl, NULL},
	 {5, g_lineAsternText, RunMenuStatus, &g_lineAsternControl, NULL},
	 {5, g_vFormText, RunMenuStatus, &g_vFormControl, NULL},
	 {5, g_wedgeText, RunMenuStatus, &g_wedgeControl, NULL},
	 {6, g_escToExitText, NULL, NULL, NULL}}
};

MenuPage g_commandAllPage = {
	0,
	g_commandAllTitle,
	0,
	7,
	0,
	NULL,
	{{3, g_statusLabel, RunMenuStatus, &g_commandAllStatusControl, NULL},
	 {5, g_attackMyTargetItem, RunMenuStatus, &g_attackControl, NULL},
	 {5, g_defendMyTargetItem, RunMenuStatus, &g_defendControl, NULL},
	 {5, g_joinFormationItem, RunMenuStatus, &g_joinFormationControl, NULL},
	 {5, g_engageAtWillItem, RunMenuStatus, &g_engageAtWillControl, NULL},
	 {5, g_shutdownItem, RunMenuStatus, &g_shutdownControl, NULL},
	 {6, g_escToExitText, NULL, NULL, NULL}}
};

MenuPage g_commandPoint2Page = {
	0,
	g_commandPoint2Title,
	1,
	7,
	0,
	PrepareCommandPointPage,
	{{3, g_statusLabel, RunMenuStatus, &g_commandPoint2StatusControl, NULL},
	 {5, g_attackMyTargetItem, RunMenuStatus, &g_attackControl, NULL},
	 {5, g_defendMyTargetItem, RunMenuStatus, &g_defendControl, NULL},
	 {5, g_joinFormationItem, RunMenuStatus, &g_joinFormationControl, NULL},
	 {5, g_engageAtWillItem, RunMenuStatus, &g_engageAtWillControl, NULL},
	 {5, g_shutdownItem, RunMenuStatus, &g_shutdownControl, NULL},
	 {6, g_escToExitText, NULL, NULL, NULL},
	 {3, g_notAvailableText, NULL, NULL, NULL}}
};

MenuPage g_commandPoint3Page = {
	0,
	g_commandPoint3Title,
	2,
	7,
	0,
	PrepareCommandPointPage,
	{{3, g_statusLabel, RunMenuStatus, &g_commandPoint3StatusControl, NULL},
	 {5, g_attackMyTargetItem, RunMenuStatus, &g_attackControl, NULL},
	 {5, g_defendMyTargetItem, RunMenuStatus, &g_defendControl, NULL},
	 {5, g_joinFormationItem, RunMenuStatus, &g_joinFormationControl, NULL},
	 {5, g_engageAtWillItem, RunMenuStatus, &g_engageAtWillControl, NULL},
	 {5, g_shutdownItem, RunMenuStatus, &g_shutdownControl, NULL},
	 {6, g_escToExitText, NULL, NULL, NULL},
	 {3, g_notAvailableText, NULL, NULL, NULL}}
};

MenuPage g_commandPoint4Page = {
	0,
	g_commandPoint4Title,
	3,
	7,
	0,
	PrepareCommandPointPage,
	{{3, g_statusLabel, RunMenuStatus, &g_commandPoint4StatusControl, NULL},
	 {5, g_attackMyTargetItem, RunMenuStatus, &g_attackControl, NULL},
	 {5, g_defendMyTargetItem, RunMenuStatus, &g_defendControl, NULL},
	 {5, g_joinFormationItem, RunMenuStatus, &g_joinFormationControl, NULL},
	 {5, g_engageAtWillItem, RunMenuStatus, &g_engageAtWillControl, NULL},
	 {5, g_shutdownItem, RunMenuStatus, &g_shutdownControl, NULL},
	 {6, g_escToExitText, NULL, NULL, NULL},
	 {3, g_notAvailableText, NULL, NULL, NULL}}
};

MenuPage g_commandPoint5Page = {
	0,
	g_commandPoint5Title,
	4,
	7,
	0,
	PrepareCommandPointPage,
	{{3, g_statusLabel, RunMenuStatus, &g_commandPoint5StatusControl, NULL},
	 {5, g_attackMyTargetItem, RunMenuStatus, &g_attackControl, NULL},
	 {5, g_defendMyTargetItem, RunMenuStatus, &g_defendControl, NULL},
	 {5, g_joinFormationItem, RunMenuStatus, &g_joinFormationControl, NULL},
	 {5, g_engageAtWillItem, RunMenuStatus, &g_engageAtWillControl, NULL},
	 {5, g_shutdownItem, RunMenuStatus, &g_shutdownControl, NULL},
	 {6, g_escToExitText, NULL, NULL, NULL},
	 {3, g_notAvailableText, NULL, NULL, NULL}}
};

MenuPage g_commandComputerPage = {
	0,
	g_commandComputerTitle,
	0,
	8,
	0,
	PrepareCommandComputerPage,
	{{0, g_changeFormationItem, RunMenuStatus, &g_formationControl, &g_changeFormationPage},
	 {0, g_commandPoint2Item, RunMenuStatus, &g_commandPoint2StatusControl, &g_commandPoint2Page},
	 {0, g_commandPoint3Item, RunMenuStatus, &g_commandPoint3StatusControl, &g_commandPoint3Page},
	 {0, g_commandPoint4Item, RunMenuStatus, &g_commandPoint4StatusControl, &g_commandPoint4Page},
	 {0, g_commandPoint5Item, RunMenuStatus, &g_commandPoint5StatusControl, &g_commandPoint5Page},
	 {0, g_commandAllItem, RunMenuStatus, &g_commandAllStatusControl, &g_commandAllPage},
	 {6, g_escToExitText, NULL, NULL, NULL},
	 {3, g_noStarMatesText, NULL, NULL, NULL}}
};

PANE g_commandMenuTarget = {NULL, 0x3852, 0x4ccd, 0x10000, 0x999a};

PANE g_commandMenuBackgroundTarget = {NULL, 0x3852, 0x4ccd, 0x10000, 0x999a};

MenuDefinition g_commandMenu = {
	&g_commandMenuTarget,
	10,
	g_commandMenuPageStack,
	0,
	-1,
	NULL,
	&g_commandMenuBackgroundTarget,
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
	&g_commandComputerPage
};

MenuPage* g_commandMenuPageStack[8];

MenuPage* g_commandPoint2MenuPageStack[8];
