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

// GLOBAL: MW2 0x100a7130
PANE g_commandPoint2MenuTarget = {NULL, 0x3852, 0x4ccd, 0x10000, 0x999a};

// GLOBAL: MW2 0x100a7148
PANE g_commandPoint2MenuBackgroundTarget = {NULL, 0x3852, 0x4ccd, 0x10000, 0x999a};

// GLOBAL: MW2 0x100a7160
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

// GLOBAL: MW2 0x100a71d0
MechChar g_commandComputerTitle[] = "COMMAND COMPUTER";

// GLOBAL: MW2 0x100a71e8
MechChar g_commandAllItem[] = "Command All";

// GLOBAL: MW2 0x100a71f8
MechChar g_changeFormationItem[] = "Change Formation";

// GLOBAL: MW2 0x100a7210
MechChar g_noStarMatesText[] = "NO STAR MATES";

// GLOBAL: MW2 0x100a7220
MechChar g_currentFormationLabel[] = "Current Form:";

// GLOBAL: MW2 0x100a7230
MechChar g_commandPoint2Item[] = "Command Point 2";

// GLOBAL: MW2 0x100a7240
MechChar g_commandPoint3Item[] = "Command Point 3";

// GLOBAL: MW2 0x100a7250
MechChar g_commandPoint4Item[] = "Command Point 4";

// GLOBAL: MW2 0x100a7260
MechChar g_commandPoint5Item[] = "Command Point 5";

// GLOBAL: MW2 0x100a7270
MechChar g_statusLabel[] = "Status:";

// GLOBAL: MW2 0x100a7278
MechChar g_notAvailableText[] = "NOT AVAILABLE";

// GLOBAL: MW2 0x100a7288
MechChar g_commandAllTitle[] = "COMMAND ALL";

// GLOBAL: MW2 0x100a7298
MechChar g_changeFormationTitle[] = "CHANGE FORMATION";

// GLOBAL: MW2 0x100a72b0
MechChar g_commandPoint2Title[] = "COMMAND POINT 2";

// GLOBAL: MW2 0x100a72c0
MechChar g_commandPoint3Title[] = "COMMAND POINT 3";

// GLOBAL: MW2 0x100a72d0
MechChar g_commandPoint4Title[] = "COMMAND POINT 4";

// GLOBAL: MW2 0x100a72e0
MechChar g_commandPoint5Title[] = "COMMAND POINT 5";

// GLOBAL: MW2 0x100a72f0
MechChar g_echelonLeftText[] = "Echelon Left";

// GLOBAL: MW2 0x100a7300
MechChar g_echelonRightText[] = "Echelon Right";

// GLOBAL: MW2 0x100a7310
MechChar g_lineAbreastText[] = "Line Abreast";

// GLOBAL: MW2 0x100a7320
MechChar g_lineAsternText[] = "Line Astern";

// GLOBAL: MW2 0x100a7330
MechChar g_vFormText[] = "V Form";

// GLOBAL: MW2 0x100a7338
MechChar g_wedgeText[] = "Wedge";

// GLOBAL: MW2 0x100a7340
MechChar g_noFormationText[] = "No Formation";

// GLOBAL: MW2 0x100a7350
MechChar g_orderAttackText[] = "Attack";

// GLOBAL: MW2 0x100a7358
MechChar g_orderDefendText[] = "Defend";

// GLOBAL: MW2 0x100a7360
MechChar g_orderJoinFormationText[] = "Join Formation";

// GLOBAL: MW2 0x100a7370
MechChar g_orderChangeFormationText[] = "Change Formation";

// GLOBAL: MW2 0x100a7388
MechChar g_orderDisengageText[] = "Disengage";

// GLOBAL: MW2 0x100a7398
MechChar g_orderEngageAtWillText[] = "Engage at Will";

// GLOBAL: MW2 0x100a73a8
MechChar g_orderShutdownText[] = "Shutdown";

// GLOBAL: MW2 0x100a73b8
MechChar g_orderNoneText[] = "No Cmd";

// GLOBAL: MW2 0x100a73c0
MechChar g_attackMyTargetItem[] = "Attack My Target";

// GLOBAL: MW2 0x100a73d8
MechChar g_defendMyTargetItem[] = "Defend My Target";

// GLOBAL: MW2 0x100a73f0
MechChar g_joinFormationItem[] = "Join Formation";

// GLOBAL: MW2 0x100a7400
MechChar g_disengageItem[] = "Disengage";

// GLOBAL: MW2 0x100a7410
MechChar g_engageAtWillItem[] = "Engage at Will";

// GLOBAL: MW2 0x100a7420
MechChar g_shutdownItem[] = "Shutdown";

// GLOBAL: MW2 0x100a7430
MechChar g_aiStateNoneText[] = "None ";

// GLOBAL: MW2 0x100a7438
MechChar g_aiStateIdleText[] = "Idle ";

// GLOBAL: MW2 0x100a7440
MechChar g_aiStateAvoidText[] = "Avoiding ";

// GLOBAL: MW2 0x100a7450
MechChar g_aiStateTargetText[] = "Targeting ";

// GLOBAL: MW2 0x100a7460
MechChar g_aiStateAttackText[] = "Engaging ";

// GLOBAL: MW2 0x100a7470
MechChar g_aiStateFleeText[] = "Disengaging ";

// GLOBAL: MW2 0x100a7480
MechChar g_aiStateFollowText[] = "In Formation ";

// GLOBAL: MW2 0x100a7490
MechChar g_aiStateReconText[] = "Reconning ";

// GLOBAL: MW2 0x100a74a0
MechChar g_aiStatePatrolText[] = "Defending ";

// GLOBAL: MW2 0x100a74b0
MechChar g_aiStateGoDirectText[] = "En Route ";

// GLOBAL: MW2 0x100a74c0
MechChar g_aiState9Text[] = "Disengaging ";

// GLOBAL: MW2 0x100a74d0
MechChar g_aiStateRestText[] = "Silent ";

// GLOBAL: MW2 0x100a74d8
MechChar g_aiStateShutdownText[] = "Shutdown ";

// GLOBAL: MW2 0x100a74e8
MechChar g_aiStateDeadText[] = "Destroyed ";

// GLOBAL: MW2 0x100a74f8
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
// GLOBAL: MW2 0x100a7540
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
// GLOBAL: MW2 0x100a7588
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

// GLOBAL: MW2 0x100a75d0
MenuChoices g_noChoices = {NULL, 0};

// GLOBAL: MW2 0x100a7618
MenuControl g_formationControl = {3, 0, &g_formationChoices, 0, NULL, GetFormation, NULL, NULL, NULL};

// GLOBAL: MW2 0x100a7640
MenuControl g_commandAllStatusControl = {3, 0, &g_orderChoices, 0, NULL, GetLanceOrder, NULL, NULL, NULL};

// GLOBAL: MW2 0x100a7668
MenuControl g_commandPoint2StatusControl =
	{3, 0, &g_aiStateChoices, 1, InstallGoalSuffix, GetSlotAiState, NULL, NULL, NULL};

// GLOBAL: MW2 0x100a7690
MenuControl g_commandPoint3StatusControl =
	{3, 0, &g_aiStateChoices, 2, InstallGoalSuffix, GetSlotAiState, NULL, NULL, NULL};

// GLOBAL: MW2 0x100a76b8
MenuControl g_commandPoint4StatusControl =
	{3, 0, &g_aiStateChoices, 3, InstallGoalSuffix, GetSlotAiState, NULL, NULL, NULL};

// GLOBAL: MW2 0x100a76e0
MenuControl g_commandPoint5StatusControl =
	{3, 0, &g_aiStateChoices, 4, InstallGoalSuffix, GetSlotAiState, NULL, NULL, NULL};

// GLOBAL: MW2 0x100a7708
MenuControl g_echelonLeftControl = {2, 0, &g_noChoices, 0, NULL, NULL, NULL, SelectFormation, NULL};

// GLOBAL: MW2 0x100a7730
MenuControl g_echelonRightControl = {2, 0, &g_noChoices, 1, NULL, NULL, NULL, SelectFormation, NULL};

// GLOBAL: MW2 0x100a7758
MenuControl g_lineAbreastControl = {2, 0, &g_noChoices, 2, NULL, NULL, NULL, SelectFormation, NULL};

// GLOBAL: MW2 0x100a7780
MenuControl g_lineAsternControl = {2, 0, &g_noChoices, 3, NULL, NULL, NULL, SelectFormation, NULL};

// GLOBAL: MW2 0x100a77a8
MenuControl g_vFormControl = {2, 0, &g_noChoices, 4, NULL, NULL, NULL, SelectFormation, NULL};

// GLOBAL: MW2 0x100a77d0
MenuControl g_wedgeControl = {2, 0, &g_noChoices, 5, NULL, NULL, NULL, SelectFormation, NULL};

// GLOBAL: MW2 0x100a77f8
MenuControl g_attackControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderAttack, NULL};

// GLOBAL: MW2 0x100a7820
MenuControl g_engageAtWillControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderEngageAtWill, NULL};

// GLOBAL: MW2 0x100a7848
MenuControl g_joinFormationControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderJoinFormation, NULL};

// GLOBAL: MW2 0x100a7870
MenuControl g_defendControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderDefend, NULL};

// GLOBAL: MW2 0x100a7898
MenuControl g_disengageControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderDisengage, NULL};

// GLOBAL: MW2 0x100a78c0
MenuControl g_shutdownControl = {2, 0, &g_noChoices, 0, SetControlSlot, NULL, NULL, OrderShutdown, NULL};

// GLOBAL: MW2 0x100a78e8
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

// GLOBAL: MW2 0x100a7a40
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

// GLOBAL: MW2 0x100a7b98
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

// GLOBAL: MW2 0x100a7cf0
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

// GLOBAL: MW2 0x100a7e48
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

// GLOBAL: MW2 0x100a7fa0
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

// GLOBAL: MW2 0x100a80f8
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

// GLOBAL: MW2 0x100a8250
PANE g_commandMenuTarget = {NULL, 0x3852, 0x4ccd, 0x10000, 0x999a};

// GLOBAL: MW2 0x100a8268
PANE g_commandMenuBackgroundTarget = {NULL, 0x3852, 0x4ccd, 0x10000, 0x999a};

// GLOBAL: MW2 0x100a8280
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

// GLOBAL: MW2 0x100ea7e0
MenuPage* g_commandMenuPageStack[8];

// GLOBAL: MW2 0x100ea800
MenuPage* g_commandPoint2MenuPageStack[8];
