#include "menudata.h"

#include "campaignmission.h"
#include "decomp.h"
#include "formation.h"
#include "mainmenubutton.h"
#include "menuscreen.h"
#include "types.h"

// The menu screens' layouts and the campaigns' tables.
//
// Each screen has a MenuScreen table indexed by campaign: Wolf (0), Jade Falcon (1) and, where
// the screen has one, the Trials of Grievance (2), which reuse Wolf's buttons. The button labels
// follow in the order of the button tables: Wolf's, Jade Falcon's, the main menu's and the
// mission briefing's.

extern MechS32 g_echelonLeftPositions[6];
extern MechS32 g_echelonRightPositions[6];
extern MechS32 g_lineAbreastPositions[6];
extern MechS32 g_lineAsternPositions[6];
extern MechS32 g_vFormPositions[6];
extern MechS32 g_wedgePositions[6];

MechChar g_wolfClanHallCadetTrainingLabel[0x10] = "CADET TRAINING";

MechChar g_wolfClanHallArchiveLabel[0x18] = "ARCHIVE HOLOPROJECTOR";

MechChar g_wolfClanHallReadyRoomLabel[0x0c] = "READY ROOM";

MechChar g_wolfClanHallRegisterLabel[0x0c] = "REGISTER";

MechChar g_wolfClanHallExitLabel[0x08] = "~EXIT";

MechChar g_wolfRosterNewAllegianceLabel[0x14] = "<~NEW ALLEGIANCE";

// The ten pilot slots, which show the callsigns instead.
MechChar g_wolfRosterSlot0Label[0x04] = "";

MechChar g_wolfRosterSlot1Label[0x04] = "";

MechChar g_wolfRosterSlot2Label[0x04] = "";

MechChar g_wolfRosterSlot3Label[0x04] = "";

MechChar g_wolfRosterSlot4Label[0x04] = "";

MechChar g_wolfRosterSlot5Label[0x04] = "";

MechChar g_wolfRosterSlot6Label[0x04] = "";

MechChar g_wolfRosterSlot7Label[0x04] = "";

MechChar g_wolfRosterSlot8Label[0x04] = "";

MechChar g_wolfRosterSlot9Label[0x04] = "";

MechChar g_wolfRosterAcceptLabel[0x0c] = "<~ACCEPT";

MechChar g_wolfRosterDeleteMechwarriorLabel[0x18] = "<~DELETE MECHWARRIOR";

MechChar g_wolfRosterLaunchOldMissionLabel[0x18] = "<~LAUNCH OLD MISSION";

MechChar g_wolfRosterPilotInfoLabel[0x10] = "<~PILOT INFO";

MechChar g_wolfArchiveExitLabel[0x08] = "~EXIT";

MechChar g_wolfArchivePrevPageLabel[0x0c] = "~PREV PAGE";

MechChar g_wolfArchiveNextPageLabel[0x0c] = "~NEXT PAGE";

MechChar g_wolfArchiveBackLabel[0x08] = "~BACK";

MechChar g_wolfCadetTrainingClanHallLabel[0x0c] = "CLAN HALL";

MechChar g_wolfCadetTrainingNavComputerLabel[0x10] = "<~NAV COMPUTER";

MechChar g_wolfCadetTrainingMechHandlingLabel[0x10] = "<~MECH HANDLING";

MechChar g_wolfCadetTrainingWeaponsUsageLabel[0x10] = "<~WEAPONS USAGE";

MechChar g_wolfCadetTrainingHuntingLabel[0x0c] = "<~HUNTING";

MechChar g_wolfCadetTrainingInspectionLabel[0x10] = "<~INSPECTION";

MechChar g_wolfCadetTrainingTrialLabel[0x08] = "<~TRIAL";

MechChar g_wolfReadyRoomClanHallLabel[0x0c] = "CLAN HALL";

MechChar g_wolfReadyRoomMechLabLabel[0x0c] = "~MECH LAB";

MechChar g_wolfReadyRoomStarConfigLabel[0x10] = "~STAR CONFIG";

MechChar g_wolfReadyRoomMissionBriefingLabel[0x14] = "~MISSION BRIEFING";

// The campaign's missions, in g_wolfMissions' order, by their scenario names.
MechChar g_wolfReadyRoomYellowLabel[0x0c] = "<~YELLOW";

MechChar g_wolfReadyRoomOrangeLabel[0x0c] = "<~ORANGE";

MechChar g_wolfReadyRoomTealLabel[0x08] = "<~TEAL";

MechChar g_wolfReadyRoomTaupeLabel[0x08] = "<~TAUPE";

MechChar g_wolfReadyRoomJennyLabel[0x08] = "<~JENNY";

MechChar g_wolfReadyRoomSableLabel[0x08] = "<~SABLE";

MechChar g_wolfReadyRoomGreyLabel[0x08] = "<~GREY";

MechChar g_wolfReadyRoomBrownLabel[0x08] = "<~BROWN";

MechChar g_wolfReadyRoomAmyLabel[0x08] = "<~AMY";

MechChar g_wolfReadyRoomSilverLabel[0x0c] = "<~SILVER";

MechChar g_wolfReadyRoomAquaLabel[0x08] = "<~AQUA";

MechChar g_wolfReadyRoomKimLabel[0x08] = "<~KIM";

MechChar g_wolfReadyRoomCyanLabel[0x08] = "<~CYAN";

MechChar g_wolfReadyRoomMaroonLabel[0x0c] = "<~MAROON";

MechChar g_wolfReadyRoomGoldLabel[0x08] = "<~GOLD";

MechChar g_wolfReadyRoomIreneLabel[0x08] = "<~IRENE";

MechChar g_wolfBriefingAbortLabel[0x08] = "<~ABORT";

MechChar g_wolfBriefingSituationLabel[0x0c] = "<~SITUATION";

MechChar g_wolfBriefingLaunchLabel[0x0c] = "<~LAUNCH";

MechChar g_wolfBriefingSkipLabel[0x08] = "<~SKIP";

MechChar g_wolfSituationExitLabel[0x08] = "<~EXIT";

MechChar g_wolfSituationPrevPageLabel[0x0c] = "<~PREV PAGE";

MechChar g_wolfSituationNextPageLabel[0x0c] = "<~NEXT PAGE";

// ArchiveReader's BACK button, placed off screen.
MechChar g_wolfSituationBackLabel[0x04] = "";

MechChar g_wolfDebriefExitLabel[0x08] = "<~EXIT";

MechChar g_wolfDebriefAftermathLabel[0x0c] = "<~AFTERMATH";

MechChar g_wolfDebriefReplayLabel[0x0c] = "<~REPLAY";

MechChar g_wolfAftermathExitLabel[0x08] = "<~EXIT";

MechChar g_wolfAftermathPrevPageLabel[0x0c] = "<~PREV PAGE";

MechChar g_wolfAftermathNextPageLabel[0x0c] = "<~NEXT PAGE";

MechChar g_wolfAftermathBackLabel[0x04] = "";

MechChar g_wolfStarConfigExitConfigLabel[0x10] = "<~EXIT CONFIG";

MechChar g_wolfStarConfigMechLabLabel[0x0c] = "~MECH LAB";

MechChar g_wolfStarConfigNextFormationLabel[0x10] = "NEXT FORMATION";

MechChar g_wolfStarConfigPrevFormationLabel[0x10] = "PREV FORMATION";

MechChar g_wolfStarConfigAddStarmateLabel[0x10] = "ADD STARMATE";

MechChar g_wolfStarConfigDeleteStarmateLabel[0x10] = "DELETE STARMATE";

MechChar g_wolfStarConfigChangeMech0Label[0x0c] = "CHANGE MECH";

MechChar g_wolfStarConfigChangeMech1Label[0x0c] = "CHANGE MECH";

MechChar g_wolfStarConfigChangeMech2Label[0x0c] = "CHANGE MECH";

MechChar g_wolfMechBayExitLabLabel[0x0c] = "<~EXIT LAB";

MechChar g_wolfMechBayStarConfigLabel[0x10] = "~STAR CONFIG";

MechChar g_wolfMechBayNextChassisLabel[0x10] = "NEXT CHASSIS";

MechChar g_wolfMechBayPrevChassisLabel[0x10] = "PREV CHASSIS";

MechChar g_wolfMechBayNextVariantLabel[0x10] = "NEXT VARIANT";

MechChar g_wolfMechBayPrevVariantLabel[0x10] = "PREV VARIANT";

MechChar g_wolfMechBayCustomizeLabel[0x0c] = "<~CUSTOMIZE";

MechChar g_wolfMechBayAcceptMechLabel[0x10] = "<~ACCEPT MECH";

MechChar g_wolfMechBaySaveLabel[0x08] = "<~SAVE";

MechChar g_wolfMechBayAbortLabel[0x08] = "<~ABORT";

MechChar g_wolfMechBayDeleteLabel[0x0c] = "<~DELETE";

MechChar g_jadeFalconClanHallCadetTrainingLabel[0x10] = "CADET TRAINING";

MechChar g_jadeFalconClanHallArchiveLabel[0x18] = "ARCHIVE HOLOPROJECTOR";

MechChar g_jadeFalconClanHallReadyRoomLabel[0x0c] = "READY ROOM";

MechChar g_jadeFalconClanHallRegisterLabel[0x0c] = "REGISTER";

MechChar g_jadeFalconClanHallExitLabel[0x08] = "~EXIT";

MechChar g_jadeFalconRosterNewAllegianceLabel[0x14] = "<~NEW ALLEGIANCE";

MechChar g_jadeFalconRosterSlot0Label[0x04] = "";

MechChar g_jadeFalconRosterSlot1Label[0x04] = "";

MechChar g_jadeFalconRosterSlot2Label[0x04] = "";

MechChar g_jadeFalconRosterSlot3Label[0x04] = "";

MechChar g_jadeFalconRosterSlot4Label[0x04] = "";

MechChar g_jadeFalconRosterSlot5Label[0x04] = "";

MechChar g_jadeFalconRosterSlot6Label[0x04] = "";

MechChar g_jadeFalconRosterSlot7Label[0x04] = "";

MechChar g_jadeFalconRosterSlot8Label[0x04] = "";

MechChar g_jadeFalconRosterSlot9Label[0x04] = "";

MechChar g_jadeFalconRosterAcceptLabel[0x0c] = "<~ACCEPT";

MechChar g_jadeFalconRosterDeleteMechwarriorLabel[0x18] = "<~DELETE MECHWARRIOR";

MechChar g_jadeFalconRosterLaunchOldMissionLabel[0x18] = "<~LAUNCH OLD MISSION";

MechChar g_jadeFalconRosterPilotInfoLabel[0x10] = "<~PILOT INFO";

MechChar g_jadeFalconArchiveExitLabel[0x08] = "~EXIT";

MechChar g_jadeFalconArchivePrevPageLabel[0x0c] = "~PREV PAGE";

MechChar g_jadeFalconArchiveNextPageLabel[0x0c] = "~NEXT PAGE";

MechChar g_jadeFalconArchiveBackLabel[0x08] = "~BACK";

MechChar g_jadeFalconCadetTrainingClanHallLabel[0x0c] = "CLAN HALL";

MechChar g_jadeFalconCadetTrainingNavComputerLabel[0x10] = "<~NAV COMPUTER";

MechChar g_jadeFalconCadetTrainingMechHandlingLabel[0x10] = "<~MECH HANDLING";

MechChar g_jadeFalconCadetTrainingWeaponsUsageLabel[0x10] = "<~WEAPONS USAGE";

MechChar g_jadeFalconCadetTrainingHuntingLabel[0x0c] = "<~HUNTING";

MechChar g_jadeFalconCadetTrainingInspectionLabel[0x10] = "<~INSPECTION";

MechChar g_jadeFalconCadetTrainingTrialLabel[0x08] = "<~TRIAL";

MechChar g_jadeFalconReadyRoomClanHallLabel[0x0c] = "CLAN HALL";

MechChar g_jadeFalconReadyRoomMechLabLabel[0x0c] = "~MECH LAB";

MechChar g_jadeFalconReadyRoomStarConfigLabel[0x10] = "~STAR CONFIG";

MechChar g_jadeFalconReadyRoomMissionBriefingLabel[0x14] = "~MISSION BRIEFING";

MechChar g_jadeFalconReadyRoomPinkLabel[0x08] = "<~PINK";

MechChar g_jadeFalconReadyRoomGreenLabel[0x08] = "<~GREEN";

MechChar g_jadeFalconReadyRoomRedLabel[0x08] = "<~RED";

MechChar g_jadeFalconReadyRoomFuchsiaLabel[0x0c] = "<~FUCHSIA";

MechChar g_jadeFalconReadyRoomCindyLabel[0x08] = "<~CINDY";

MechChar g_jadeFalconReadyRoomRustLabel[0x08] = "<~RUST";

MechChar g_jadeFalconReadyRoomUmberLabel[0x08] = "<~UMBER";

MechChar g_jadeFalconReadyRoomTanLabel[0x08] = "<~TAN";

MechChar g_jadeFalconReadyRoomHeidiLabel[0x08] = "<~HEIDI";

MechChar g_jadeFalconReadyRoomPlumLabel[0x08] = "<~PLUM";

MechChar g_jadeFalconReadyRoomWhiteLabel[0x08] = "<~WHITE";

MechChar g_jadeFalconReadyRoomJillLabel[0x08] = "<~JILL";

MechChar g_jadeFalconReadyRoomPuceLabel[0x08] = "<~PUCE";

MechChar g_jadeFalconReadyRoomBlondeLabel[0x0c] = "<~BLONDE";

MechChar g_jadeFalconReadyRoomBronzeLabel[0x0c] = "<~BRONZE";

MechChar g_jadeFalconReadyRoomMaryLabel[0x08] = "<~MARY";

MechChar g_jadeFalconBriefingAbortLabel[0x08] = "<~ABORT";

MechChar g_jadeFalconBriefingSituationLabel[0x0c] = "<~SITUATION";

MechChar g_jadeFalconBriefingLaunchLabel[0x0c] = "<~LAUNCH";

MechChar g_jadeFalconBriefingSkipLabel[0x08] = "<~SKIP";

MechChar g_jadeFalconSituationExitLabel[0x08] = "<~EXIT";

MechChar g_jadeFalconSituationPrevPageLabel[0x0c] = "<~PREV PAGE";

MechChar g_jadeFalconSituationNextPageLabel[0x0c] = "<~NEXT PAGE";

MechChar g_jadeFalconSituationBackLabel[0x04] = "";

MechChar g_jadeFalconDebriefExitLabel[0x08] = "<~EXIT";

MechChar g_jadeFalconDebriefAftermathLabel[0x0c] = "<~AFTERMATH";

MechChar g_jadeFalconDebriefReplayLabel[0x0c] = "<~REPLAY";

MechChar g_jadeFalconAftermathExitLabel[0x08] = "<~EXIT";

MechChar g_jadeFalconAftermathPrevPageLabel[0x0c] = "<~PREV PAGE";

MechChar g_jadeFalconAftermathNextPageLabel[0x0c] = "<~NEXT PAGE";

MechChar g_jadeFalconAftermathBackLabel[0x04] = "";

MechChar g_jadeFalconStarConfigExitConfigLabel[0x10] = "<~EXIT CONFIG";

MechChar g_jadeFalconStarConfigMechLabLabel[0x0c] = "~MECH LAB";

MechChar g_jadeFalconStarConfigNextFormationLabel[0x10] = "NEXT FORMATION";

MechChar g_jadeFalconStarConfigPrevFormationLabel[0x10] = "PREV FORMATION";

MechChar g_jadeFalconStarConfigAddStarmateLabel[0x10] = "ADD STARMATE";

MechChar g_jadeFalconStarConfigDeleteStarmateLabel[0x10] = "DELETE STARMATE";

MechChar g_jadeFalconStarConfigChangeMech0Label[0x0c] = "CHANGE MECH";

MechChar g_jadeFalconStarConfigChangeMech1Label[0x0c] = "CHANGE MECH";

MechChar g_jadeFalconStarConfigChangeMech2Label[0x0c] = "CHANGE MECH";

MechChar g_jadeFalconMechBayExitLabLabel[0x0c] = "<~EXIT LAB";

MechChar g_jadeFalconMechBayStarConfigLabel[0x10] = "~STAR CONFIG";

MechChar g_jadeFalconMechBayNextChassisLabel[0x10] = "NEXT CHASSIS";

MechChar g_jadeFalconMechBayPrevChassisLabel[0x10] = "PREV CHASSIS";

MechChar g_jadeFalconMechBayNextVariantLabel[0x10] = "NEXT VARIANT";

MechChar g_jadeFalconMechBayPrevVariantLabel[0x10] = "PREV VARIANT";

MechChar g_jadeFalconMechBayCustomizeLabel[0x0c] = "<~CUSTOMIZE";

MechChar g_jadeFalconMechBayAcceptMechLabel[0x10] = "<~ACCEPT MECH";

MechChar g_jadeFalconMechBaySaveLabel[0x08] = "<~SAVE";

MechChar g_jadeFalconMechBayAbortLabel[0x08] = "<~ABORT";

MechChar g_jadeFalconMechBayDeleteLabel[0x0c] = "<~DELETE";

MechChar g_mainMenuTrialsOfGrievanceLabel[0x18] = "~TRIALS OF GRIEVANCE";

MechChar g_mainMenuWolfClanHallLabel[0x10] = "~WOLF CLAN HALL";

MechChar g_mainMenuJadeFalconClanHallLabel[0x18] = "~JADE FALCON CLAN HALL";

MechChar g_mainMenuExitLabel[0x08] = "~EXIT";

// The Trials of Grievance's ready room (missionui.cpp) labels only EXIT. Each ...Next/...Prev pair
// steps a mech's chassis or the star's formation, and ...Clan steps the star's clan.
MechChar g_missionBriefingLaunchLabel[0x04] = "";

MechChar g_missionBriefingExitLabel[0x08] = "<~EXIT";

MechChar g_missionBriefingMissionLabel[0x04] = "";

MechChar g_missionBriefingPlayerMech0NextLabel[0x04] = "";

MechChar g_missionBriefingPlayerMech1NextLabel[0x04] = "";

MechChar g_missionBriefingPlayerMech2NextLabel[0x04] = "";

MechChar g_missionBriefingPlayerFormationNextLabel[0x04] = "";

MechChar g_missionBriefingPlayerMech0PrevLabel[0x04] = "";

MechChar g_missionBriefingPlayerMech1PrevLabel[0x04] = "";

MechChar g_missionBriefingPlayerMech2PrevLabel[0x04] = "";

MechChar g_missionBriefingPlayerFormationPrevLabel[0x04] = "";

MechChar g_missionBriefingPlayerClanLabel[0x04] = "";

MechChar g_missionBriefingPlayerMechLabLabel[0x04] = "";

MechChar g_missionBriefingPlayerStarConfigLabel[0x04] = "";

MechChar g_missionBriefingEnemyMech0NextLabel[0x04] = "";

MechChar g_missionBriefingEnemyMech1NextLabel[0x04] = "";

MechChar g_missionBriefingEnemyMech2NextLabel[0x04] = "";

MechChar g_missionBriefingEnemyFormationNextLabel[0x04] = "";

MechChar g_missionBriefingEnemyMech0PrevLabel[0x04] = "";

MechChar g_missionBriefingEnemyMech1PrevLabel[0x04] = "";

MechChar g_missionBriefingEnemyMech2PrevLabel[0x04] = "";

MechChar g_missionBriefingEnemyFormationPrevLabel[0x04] = "";

MechChar g_missionBriefingEnemyClanLabel[0x04] = "";

MechChar g_missionBriefingEnemyMechLabLabel[0x04] = "";

MechChar g_missionBriefingEnemyStarConfigLabel[0x04] = "";

// Tab stops for the "\T" text escape, in pixels from the left edge.
MechS32 g_textTabStops[19] =
	{0, 36, 72, 108, 144, 180, 216, 252, 288, 324, 360, 396, 432, 468, 504, 540, 576, 612, 640};

// The shell's database, for TMPackDataBase.
char* g_databaseName = "DATABASE.MW2";

// The clan hall archives, by campaign.
MechChar* g_archiveNames[2] = {"ARCHWO.MW2", "ARCHJF.MW2"};

// The star formations, as the mission briefing screen names them.
Formation g_formations[6] = {
	{g_echelonLeftPositions, "Echelon Left"},
	{g_echelonRightPositions, "Echelon Right"},
	{g_lineAbreastPositions, "Line Abreast"},
	{g_lineAsternPositions, "Line Astern"},
	{g_vFormPositions, "V-Form"},
	{g_wedgePositions, "Wedge"},
};

// Where each formation puts its three mechs: x, y pairs in screen pixels. Nothing reads them;
// the star screens use the layouts in mechvariant.cpp.
MechS32 g_echelonLeftPositions[6] = {285, 176, 344, 222, 450, 320};

MechS32 g_echelonRightPositions[6] = {520, 212, 342, 225, 143, 253};

MechS32 g_lineAbreastPositions[6] = {222, 199, 341, 216, 498, 242};

MechS32 g_lineAsternPositions[6] = {385, 184, 343, 221, 264, 290};

MechS32 g_vFormPositions[6] = {269, 176, 506, 205, 269, 283};

MechS32 g_wedgePositions[6] = {376, 186, 139, 254, 452, 325};

// The buttons of each screen, Wolf's first. The Trials of Grievance use Wolf's.
MainMenuButton g_wolfClanHallButtons[5] = {
	{185, 280, 240, 400, 197, 327, g_wolfClanHallCadetTrainingLabel},
	{320, 300, 470, 400, 246, 371, g_wolfClanHallArchiveLabel},
	{20, 245, 90, 411, 25, 307, g_wolfClanHallReadyRoomLabel},
	{95, 385, 180, 479, 80, 455, g_wolfClanHallRegisterLabel},
	{0, 0, 639, 40, 320, 15, g_wolfClanHallExitLabel},
};

MainMenuButton g_wolfRosterButtons[15] = {
	{466, 450, 619, 474, 543, 455, g_wolfRosterNewAllegianceLabel},
	{32, 83, 297, 116, 41, 91, g_wolfRosterSlot0Label},
	{32, 117, 297, 151, 41, 125, g_wolfRosterSlot1Label},
	{32, 152, 297, 186, 41, 160, g_wolfRosterSlot2Label},
	{32, 187, 297, 221, 41, 195, g_wolfRosterSlot3Label},
	{32, 222, 297, 256, 41, 230, g_wolfRosterSlot4Label},
	{32, 257, 297, 291, 41, 265, g_wolfRosterSlot5Label},
	{32, 292, 297, 326, 41, 300, g_wolfRosterSlot6Label},
	{32, 327, 297, 361, 41, 335, g_wolfRosterSlot7Label},
	{32, 362, 297, 397, 41, 370, g_wolfRosterSlot8Label},
	{32, 398, 297, 432, 41, 406, g_wolfRosterSlot9Label},
	{294, 450, 393, 474, 344, 455, g_wolfRosterAcceptLabel},
	{20, 450, 221, 474, 121, 455, g_wolfRosterDeleteMechwarriorLabel},
	{373, 403, 562, 427, 468, 408, g_wolfRosterLaunchOldMissionLabel},
	{418, 403, 517, 427, 468, 408, g_wolfRosterPilotInfoLabel},
};

MainMenuButton g_wolfArchiveButtons[4] = {
	{0, 440, 639, 479, 320, 455, g_wolfArchiveExitLabel},
	{405, 372, 479, 405, 449, 354, g_wolfArchivePrevPageLabel},
	{480, 372, 556, 405, 511, 408, g_wolfArchiveNextPageLabel},
	{67, 358, 120, 410, 92, 408, g_wolfArchiveBackLabel},
};

MainMenuButton g_wolfCadetTrainingButtons[7] = {
	{5, 149, 57, 440, 18, 203, g_wolfCadetTrainingClanHallLabel},
	{450, 13, 629, 37, 540, 18, g_wolfCadetTrainingNavComputerLabel},
	{450, 38, 629, 62, 540, 43, g_wolfCadetTrainingMechHandlingLabel},
	{450, 63, 629, 87, 540, 68, g_wolfCadetTrainingWeaponsUsageLabel},
	{450, 88, 629, 112, 540, 93, g_wolfCadetTrainingHuntingLabel},
	{450, 113, 629, 137, 540, 118, g_wolfCadetTrainingInspectionLabel},
	{450, 138, 629, 162, 540, 143, g_wolfCadetTrainingTrialLabel},
};

MainMenuButton g_wolfReadyRoomButtons[20] = {
	{0, 0, 130, 479, 56, 223, g_wolfReadyRoomClanHallLabel},
	{300, 320, 559, 419, 450, 364, g_wolfReadyRoomMechLabLabel},
	{510, 420, 559, 469, 542, 455, g_wolfReadyRoomStarConfigLabel},
	{140, 90, 399, 313, 265, 226, g_wolfReadyRoomMissionBriefingLabel},
	{400, 25, 519, 49, 460, 30, g_wolfReadyRoomYellowLabel},
	{400, 55, 519, 79, 460, 60, g_wolfReadyRoomOrangeLabel},
	{400, 85, 519, 109, 460, 90, g_wolfReadyRoomTealLabel},
	{400, 115, 519, 139, 460, 120, g_wolfReadyRoomTaupeLabel},
	{400, 145, 519, 169, 460, 150, g_wolfReadyRoomJennyLabel},
	{400, 175, 519, 199, 460, 180, g_wolfReadyRoomSableLabel},
	{400, 205, 519, 229, 460, 210, g_wolfReadyRoomGreyLabel},
	{400, 235, 519, 259, 460, 240, g_wolfReadyRoomBrownLabel},
	{520, 25, 639, 49, 580, 30, g_wolfReadyRoomAmyLabel},
	{520, 55, 639, 79, 580, 60, g_wolfReadyRoomSilverLabel},
	{520, 85, 639, 109, 580, 90, g_wolfReadyRoomAquaLabel},
	{520, 115, 639, 139, 580, 120, g_wolfReadyRoomKimLabel},
	{520, 145, 639, 169, 580, 150, g_wolfReadyRoomCyanLabel},
	{520, 175, 639, 199, 580, 180, g_wolfReadyRoomMaroonLabel},
	{520, 205, 639, 229, 580, 210, g_wolfReadyRoomGoldLabel},
	{520, 235, 639, 259, 580, 240, g_wolfReadyRoomIreneLabel},
};

MainMenuButton g_wolfBriefingButtons[4] = {
	{430, 450, 529, 474, 480, 455, g_wolfBriefingAbortLabel},
	{110, 450, 209, 474, 160, 455, g_wolfBriefingSituationLabel},
	{270, 450, 369, 474, 320, 455, g_wolfBriefingLaunchLabel},
	{540, 450, 639, 474, 590, 455, g_wolfBriefingSkipLabel},
};

MainMenuButton g_wolfSituationButtons[4] = {
	{110, 450, 209, 474, 160, 455, g_wolfSituationExitLabel},
	{270, 450, 369, 474, 320, 455, g_wolfSituationPrevPageLabel},
	{430, 450, 529, 474, 480, 455, g_wolfSituationNextPageLabel},
	{510, 550, 609, 574, 560, 555, g_wolfSituationBackLabel},
};

MainMenuButton g_wolfDebriefButtons[3] = {
	{270, 450, 369, 474, 320, 455, g_wolfDebriefExitLabel},
	{110, 450, 209, 474, 160, 455, g_wolfDebriefAftermathLabel},
	{430, 450, 529, 474, 480, 455, g_wolfDebriefReplayLabel},
};

MainMenuButton g_wolfAftermathButtons[4] = {
	{110, 450, 209, 474, 160, 455, g_wolfAftermathExitLabel},
	{270, 450, 369, 474, 320, 455, g_wolfAftermathPrevPageLabel},
	{430, 450, 529, 474, 480, 455, g_wolfAftermathNextPageLabel},
	{510, 550, 609, 574, 560, 555, g_wolfAftermathBackLabel},
};

MainMenuButton g_wolfStarConfigButtons[9] = {
	{50, 445, 149, 469, 100, 450, g_wolfStarConfigExitConfigLabel},
	{404, 414, 474, 474, 440, 460, g_wolfStarConfigMechLabLabel},
	{263, 425, 302, 469, 280, 465, g_wolfStarConfigNextFormationLabel},
	{237, 425, 262, 469, 260, 465, g_wolfStarConfigPrevFormationLabel},
	{303, 425, 330, 469, 300, 465, g_wolfStarConfigAddStarmateLabel},
	{200, 425, 236, 469, 240, 465, g_wolfStarConfigDeleteStarmateLabel},
	{159, 195, 187, 229, 73, 234, g_wolfStarConfigChangeMech0Label},
	{391, 162, 419, 196, 305, 201, g_wolfStarConfigChangeMech1Label},
	{586, 218, 614, 252, 500, 257, g_wolfStarConfigChangeMech2Label},
};

MainMenuButton g_wolfMechBayButtons[11] = {
	{50, 445, 149, 469, 100, 450, g_wolfMechBayExitLabLabel},
	{404, 414, 474, 474, 440, 460, g_wolfMechBayStarConfigLabel},
	{303, 425, 330, 469, 300, 465, g_wolfMechBayNextChassisLabel},
	{200, 425, 236, 469, 240, 465, g_wolfMechBayPrevChassisLabel},
	{263, 425, 302, 469, 280, 465, g_wolfMechBayNextVariantLabel},
	{237, 425, 262, 469, 260, 465, g_wolfMechBayPrevVariantLabel},
	{50, 420, 149, 444, 100, 425, g_wolfMechBayCustomizeLabel},
	{50, 395, 149, 419, 100, 400, g_wolfMechBayAcceptMechLabel},
	{50, 420, 149, 444, 100, 425, g_wolfMechBaySaveLabel},
	{50, 445, 149, 469, 100, 450, g_wolfMechBayAbortLabel},
	{490, 445, 589, 469, 540, 450, g_wolfMechBayDeleteLabel},
};

MainMenuButton g_jadeFalconClanHallButtons[5] = {
	{66, 167, 152, 303, 69, 187, g_jadeFalconClanHallCadetTrainingLabel},
	{160, 290, 375, 322, 110, 340, g_jadeFalconClanHallArchiveLabel},
	{523, 154, 636, 332, 450, 200, g_jadeFalconClanHallReadyRoomLabel},
	{397, 385, 492, 466, 397, 443, g_jadeFalconClanHallRegisterLabel},
	{0, 0, 639, 40, 320, 15, g_jadeFalconClanHallExitLabel},
};

MainMenuButton g_jadeFalconRosterButtons[15] = {
	{466, 450, 619, 474, 543, 455, g_jadeFalconRosterNewAllegianceLabel},
	{32, 83, 297, 116, 41, 91, g_jadeFalconRosterSlot0Label},
	{32, 117, 297, 151, 41, 125, g_jadeFalconRosterSlot1Label},
	{32, 152, 297, 186, 41, 160, g_jadeFalconRosterSlot2Label},
	{32, 187, 297, 221, 41, 195, g_jadeFalconRosterSlot3Label},
	{32, 222, 297, 256, 41, 230, g_jadeFalconRosterSlot4Label},
	{32, 257, 297, 291, 41, 265, g_jadeFalconRosterSlot5Label},
	{32, 292, 297, 326, 41, 300, g_jadeFalconRosterSlot6Label},
	{32, 327, 297, 361, 41, 335, g_jadeFalconRosterSlot7Label},
	{32, 362, 297, 397, 41, 370, g_jadeFalconRosterSlot8Label},
	{32, 398, 297, 432, 41, 406, g_jadeFalconRosterSlot9Label},
	{294, 450, 393, 474, 344, 455, g_jadeFalconRosterAcceptLabel},
	{20, 450, 221, 474, 121, 455, g_jadeFalconRosterDeleteMechwarriorLabel},
	{373, 403, 562, 427, 468, 408, g_jadeFalconRosterLaunchOldMissionLabel},
	{418, 403, 517, 427, 468, 408, g_jadeFalconRosterPilotInfoLabel},
};

MainMenuButton g_jadeFalconArchiveButtons[4] = {
	{0, 440, 639, 479, 320, 455, g_jadeFalconArchiveExitLabel},
	{405, 364, 479, 397, 449, 346, g_jadeFalconArchivePrevPageLabel},
	{480, 364, 556, 397, 511, 400, g_jadeFalconArchiveNextPageLabel},
	{67, 358, 120, 410, 92, 400, g_jadeFalconArchiveBackLabel},
};

MainMenuButton g_jadeFalconCadetTrainingButtons[7] = {
	{5, 149, 57, 440, 18, 203, g_jadeFalconCadetTrainingClanHallLabel},
	{450, 13, 629, 37, 540, 18, g_jadeFalconCadetTrainingNavComputerLabel},
	{450, 38, 629, 62, 540, 43, g_jadeFalconCadetTrainingMechHandlingLabel},
	{450, 63, 629, 87, 540, 68, g_jadeFalconCadetTrainingWeaponsUsageLabel},
	{450, 88, 629, 112, 540, 93, g_jadeFalconCadetTrainingHuntingLabel},
	{450, 113, 629, 137, 540, 118, g_jadeFalconCadetTrainingInspectionLabel},
	{450, 138, 629, 162, 540, 143, g_jadeFalconCadetTrainingTrialLabel},
};

MainMenuButton g_jadeFalconReadyRoomButtons[20] = {
	{0, 0, 130, 479, 56, 223, g_jadeFalconReadyRoomClanHallLabel},
	{280, 340, 534, 439, 415, 370, g_jadeFalconReadyRoomMechLabLabel},
	{432, 440, 482, 479, 464, 460, g_jadeFalconReadyRoomStarConfigLabel},
	{140, 90, 399, 313, 277, 226, g_jadeFalconReadyRoomMissionBriefingLabel},
	{400, 25, 519, 49, 460, 30, g_jadeFalconReadyRoomPinkLabel},
	{400, 55, 519, 79, 460, 60, g_jadeFalconReadyRoomGreenLabel},
	{400, 85, 519, 109, 460, 90, g_jadeFalconReadyRoomRedLabel},
	{400, 115, 519, 139, 460, 120, g_jadeFalconReadyRoomFuchsiaLabel},
	{400, 145, 519, 169, 460, 150, g_jadeFalconReadyRoomCindyLabel},
	{400, 175, 519, 199, 460, 180, g_jadeFalconReadyRoomRustLabel},
	{400, 205, 519, 229, 460, 210, g_jadeFalconReadyRoomUmberLabel},
	{400, 235, 519, 259, 460, 240, g_jadeFalconReadyRoomTanLabel},
	{520, 25, 639, 49, 580, 30, g_jadeFalconReadyRoomHeidiLabel},
	{520, 55, 639, 79, 580, 60, g_jadeFalconReadyRoomPlumLabel},
	{520, 85, 639, 109, 580, 90, g_jadeFalconReadyRoomWhiteLabel},
	{520, 115, 639, 139, 580, 120, g_jadeFalconReadyRoomJillLabel},
	{520, 145, 639, 169, 580, 150, g_jadeFalconReadyRoomPuceLabel},
	{520, 175, 639, 199, 580, 180, g_jadeFalconReadyRoomBlondeLabel},
	{520, 205, 639, 229, 580, 210, g_jadeFalconReadyRoomBronzeLabel},
	{520, 235, 639, 259, 580, 240, g_jadeFalconReadyRoomMaryLabel},
};

MainMenuButton g_jadeFalconBriefingButtons[4] = {
	{430, 450, 529, 474, 480, 455, g_jadeFalconBriefingAbortLabel},
	{110, 450, 209, 474, 160, 455, g_jadeFalconBriefingSituationLabel},
	{270, 450, 369, 474, 320, 455, g_jadeFalconBriefingLaunchLabel},
	{540, 450, 639, 474, 590, 455, g_jadeFalconBriefingSkipLabel},
};

MainMenuButton g_jadeFalconSituationButtons[4] = {
	{110, 450, 209, 474, 160, 455, g_jadeFalconSituationExitLabel},
	{270, 450, 369, 474, 320, 455, g_jadeFalconSituationPrevPageLabel},
	{430, 450, 529, 474, 480, 455, g_jadeFalconSituationNextPageLabel},
	{510, 550, 609, 574, 560, 555, g_jadeFalconSituationBackLabel},
};

MainMenuButton g_jadeFalconDebriefButtons[3] = {
	{270, 450, 369, 474, 320, 455, g_jadeFalconDebriefExitLabel},
	{110, 450, 209, 474, 160, 455, g_jadeFalconDebriefAftermathLabel},
	{430, 450, 529, 474, 480, 455, g_jadeFalconDebriefReplayLabel},
};

MainMenuButton g_jadeFalconAftermathButtons[4] = {
	{110, 450, 209, 474, 160, 455, g_jadeFalconAftermathExitLabel},
	{270, 450, 369, 474, 320, 455, g_jadeFalconAftermathPrevPageLabel},
	{430, 450, 529, 474, 480, 455, g_jadeFalconAftermathNextPageLabel},
	{510, 550, 609, 574, 560, 555, g_jadeFalconAftermathBackLabel},
};

MainMenuButton g_jadeFalconStarConfigButtons[9] = {
	{50, 445, 149, 469, 100, 450, g_jadeFalconStarConfigExitConfigLabel},
	{404, 414, 474, 474, 440, 460, g_jadeFalconStarConfigMechLabLabel},
	{263, 425, 302, 469, 280, 465, g_jadeFalconStarConfigNextFormationLabel},
	{237, 425, 262, 469, 260, 465, g_jadeFalconStarConfigPrevFormationLabel},
	{303, 425, 330, 469, 300, 465, g_jadeFalconStarConfigAddStarmateLabel},
	{200, 425, 236, 469, 240, 465, g_jadeFalconStarConfigDeleteStarmateLabel},
	{125, 201, 153, 235, 39, 240, g_jadeFalconStarConfigChangeMech0Label},
	{358, 156, 386, 190, 272, 195, g_jadeFalconStarConfigChangeMech1Label},
	{573, 246, 601, 280, 487, 285, g_jadeFalconStarConfigChangeMech2Label},
};

MainMenuButton g_jadeFalconMechBayButtons[11] = {
	{50, 445, 149, 469, 100, 450, g_jadeFalconMechBayExitLabLabel},
	{404, 414, 474, 474, 440, 460, g_jadeFalconMechBayStarConfigLabel},
	{303, 425, 330, 469, 300, 465, g_jadeFalconMechBayNextChassisLabel},
	{200, 425, 236, 469, 240, 465, g_jadeFalconMechBayPrevChassisLabel},
	{263, 425, 302, 469, 280, 465, g_jadeFalconMechBayNextVariantLabel},
	{237, 425, 262, 469, 260, 465, g_jadeFalconMechBayPrevVariantLabel},
	{50, 420, 149, 444, 100, 425, g_jadeFalconMechBayCustomizeLabel},
	{50, 395, 149, 419, 100, 400, g_jadeFalconMechBayAcceptMechLabel},
	{50, 420, 149, 444, 100, 425, g_jadeFalconMechBaySaveLabel},
	{50, 445, 149, 469, 100, 450, g_jadeFalconMechBayAbortLabel},
	{490, 445, 589, 469, 540, 450, g_jadeFalconMechBayDeleteLabel},
};

MainMenuButton g_mainMenuButtons[4] = {
	{0xdb, 0x126, 0x1aa, 0x1a3, 0x140, 0x18b, g_mainMenuTrialsOfGrievanceLabel},
	{0x1ab, 0xf5, 0x27a, 0x175, 0x20d, 0x176, g_mainMenuWolfClanHallLabel},
	{0x0a, 0xc5, 0xc8, 0x172, 0x7c, 0x176, g_mainMenuJadeFalconClanHallLabel},
	{0, 0x1c2, 0x27f, 0x1df, 0x140, 0x1c7, g_mainMenuExitLabel},
};

// The mission briefing screen (missionui.cpp): both stars' mechs and formations.
MainMenuButton g_missionBriefingButtons[0x19] = {
	{209, 371, 420, 452, 0, 0, g_missionBriefingLaunchLabel},
	{50, 445, 149, 469, 100, 450, g_missionBriefingExitLabel},
	{238, 67, 400, 102, 239, 69, g_missionBriefingMissionLabel},
	{13, 124, 103, 137, 29, 126, g_missionBriefingPlayerMech0NextLabel},
	{13, 138, 103, 151, 29, 140, g_missionBriefingPlayerMech1NextLabel},
	{13, 152, 103, 165, 29, 154, g_missionBriefingPlayerMech2NextLabel},
	{13, 181, 103, 194, 29, 183, g_missionBriefingPlayerFormationNextLabel},
	{4, 124, 12, 137, 29, 126, g_missionBriefingPlayerMech0PrevLabel},
	{4, 138, 12, 151, 29, 140, g_missionBriefingPlayerMech1PrevLabel},
	{4, 152, 12, 165, 29, 154, g_missionBriefingPlayerMech2PrevLabel},
	{4, 181, 12, 194, 29, 183, g_missionBriefingPlayerFormationPrevLabel},
	{13, 205, 160, 352, 0, 0, g_missionBriefingPlayerClanLabel},
	{134, 122, 172, 166, 0, 0, g_missionBriefingPlayerMechLabLabel},
	{134, 168, 172, 200, 0, 0, g_missionBriefingPlayerStarConfigLabel},
	{481, 249, 572, 262, 498, 251, g_missionBriefingEnemyMech0NextLabel},
	{481, 263, 572, 276, 498, 265, g_missionBriefingEnemyMech1NextLabel},
	{481, 277, 572, 290, 498, 279, g_missionBriefingEnemyMech2NextLabel},
	{481, 306, 572, 319, 498, 308, g_missionBriefingEnemyFormationNextLabel},
	{472, 249, 480, 262, 498, 251, g_missionBriefingEnemyMech0PrevLabel},
	{472, 263, 480, 276, 498, 265, g_missionBriefingEnemyMech1PrevLabel},
	{472, 277, 480, 290, 498, 279, g_missionBriefingEnemyMech2PrevLabel},
	{472, 306, 480, 319, 498, 308, g_missionBriefingEnemyFormationPrevLabel},
	{482, 330, 629, 477, 0, 0, g_missionBriefingEnemyClanLabel},
	{583, 247, 621, 291, 0, 0, g_missionBriefingEnemyMechLabLabel},
	{583, 293, 621, 325, 0, 0, g_missionBriefingEnemyStarConfigLabel},
};

// The missions of each campaign.
CampaignMission g_wolfMissions[17] = {
	{"yellSCN1", 0, "Pyre Light"},
	{"oranSCN1", 0, "Flame Tongue "},
	{"tealSCN1", 0, "Blade Splint"},
	{"taupSCN1", 0, "Temper Edge"},
	{"jennSCN1", 1, "Trial 1"},
	{"sablSCN1", 0, "Sable Flame"},
	{"greySCN1", 0, "Burning Chrome"},
	{"browSCN1", 0, "Scorching Sand"},
	{"amy_SCN1", 1, "Trial 2"},
	{"silvSCN1", 0, "Silver Staff"},
	{"aquaSCN1", 0, "Aquiline Fire"},
	{"kim_SCN1", 1, "Trial 3"},
	{"cyanSCN1", 0, "Cold Crescent"},
	{"maroSCN1", 0, "Velvet Hammer"},
	{"goldSCN1", 0, "Golden Spade"},
	{"irenSCN1", 1, "Trial 4"},
	{NULL, 0, "Retired"},
};

CampaignMission g_jadeFalconMissions[17] = {
	{"pinkSCN1", 0, "Silent Thunder"},
	{"greeSCN1", 0, "Arkham Bridge"},
	{"red_SCN1", 0, "Mirror Cage"},
	{"fuchSCN1", 0, "Bone Machine"},
	{"cindSCN1", 1, "Trial 1"},
	{"rustSCN1", 0, "Bouk Obelisk"},
	{"umbeSCN1", 0, "Umber Wall"},
	{"tan_SCN1", 0, "Rogue Chariot"},
	{"heidSCN1", 1, "Trial 2"},
	{"plumSCN1", 0, "Plum Wine"},
	{"whitSCN1", 0, "Rust Heart"},
	{"jillSCN1", 1, "Trial 3"},
	{"puceSCN1", 0, "Armor Veil"},
	{"blonSCN1", 0, "Iron Piston"},
	{"bronSCN1", 0, "Bronze Anvil"},
	{"marySCN1", 1, "Trial 4"},
	{NULL, 0, "Retired"},
};

CampaignMission* g_campaignMissions[2] = {g_wolfMissions, g_jadeFalconMissions};

// The training missions of each campaign, one per button from the second.
MechChar* g_wolfTrainingScenarios[6] = {"tnw1SCN1", "tnw2SCN1", "tnw3SCN1", "tnw4SCN1", "tnw5SCN1", "tnw6SCN1"};

MechChar* g_jadeFalconTrainingScenarios[6] = {"tnj1SCN1", "tnj2SCN1", "tnj3SCN1", "tnj4SCN1", "tnj5SCN1", "tnj6SCN1"};

MechChar** g_trainingScenarios[2] = {g_wolfTrainingScenarios, g_jadeFalconTrainingScenarios};

// The screens' MenuScreen tables, by campaign.
MenuScreen g_clanHallScreens[3] = {
	{g_wolfClanHallButtons, 5, 11, 0x24},
	{g_jadeFalconClanHallButtons, 5, 18, 0x27},
	{NULL, 0, 0, 0},
};

MenuScreen g_rosterScreens[3] = {
	{g_wolfRosterButtons, 15, 17, -1},
	{g_jadeFalconRosterButtons, 15, 24, -1},
	{NULL, 0, 0, 0},
};

MenuScreen g_archiveScreens[3] = {
	{g_wolfArchiveButtons, 4, 12, -1},
	{g_jadeFalconArchiveButtons, 4, 19, -1},
	{NULL, 0, 0, 0},
};

MenuScreen g_starConfigScreens[3] = {
	{g_wolfStarConfigButtons, 9, 15, -1},
	{g_jadeFalconStarConfigButtons, 9, 22, -1},
	{g_wolfStarConfigButtons, 9, 10, -1},
};

MenuScreen g_readyRoomScreens[3] = {
	{g_wolfReadyRoomButtons, 20, 14, 0x25},
	{g_jadeFalconReadyRoomButtons, 20, 21, 0x28},
	{NULL, 0, 0, 0},
};

// The debriefing screen of each campaign.
MenuScreen g_debriefScreens[3] = {
	{g_wolfDebriefButtons, 3, 16, -1},
	{g_jadeFalconDebriefButtons, 3, 23, -1},
	{g_wolfDebriefButtons, 3, 10, -1},
};

// The aftermath reader of each campaign.
MenuScreen g_aftermathScreens[3] = {
	{g_wolfAftermathButtons, 4, 16, -1},
	{g_jadeFalconAftermathButtons, 4, 23, -1},
	{g_wolfAftermathButtons, 4, 10, -1},
};

// The briefing screen of each campaign. SKIP (the fourth button) is dropped for every pilot
// but FERRARI.
MenuScreen g_briefingScreens[3] = {
	{g_wolfBriefingButtons, 4, 16, -1},
	{g_jadeFalconBriefingButtons, 4, 23, -1},
	{g_wolfBriefingButtons, 4, 10, -1},
};

// The situation reader of each campaign.
MenuScreen g_situationScreens[3] = {
	{g_wolfSituationButtons, 4, 16, -1},
	{g_jadeFalconSituationButtons, 4, 23, -1},
	{g_wolfSituationButtons, 4, 10, -1},
};

MenuScreen g_cadetTrainingScreens[3] = {
	{g_wolfCadetTrainingButtons, 7, 13, 38},
	{g_jadeFalconCadetTrainingButtons, 7, 20, 41},
	{NULL, 0, 0, 0},
};

MenuScreen g_mechBayScreens[3] = {
	{g_wolfMechBayButtons, 11, 15, -1},
	{g_jadeFalconMechBayButtons, 11, 22, -1},
	{g_wolfMechBayButtons, 11, 10, -1},
};
