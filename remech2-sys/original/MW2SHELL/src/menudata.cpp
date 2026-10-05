#include "menudata.h"

#include "campaignmission.h"
#include "decomp.h"
#include "formation.h"
#include "mainmenubutton.h"
#include "menuscreen.h"
#include "types.h"

// The menu screens' layouts and the campaigns' tables. The original links this data as an object
// of its own, between cockpitcontrols.cpp and options.cpp, with no code: the tables come first,
// then the strings they point to (from 0x10070020), which are defined first here.
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

// GLOBAL: MW2SHELL 0x10070094
MechChar g_wolfClanHallCadetTrainingLabel[0x10] = "CADET TRAINING";

// GLOBAL: MW2SHELL 0x100700a4
MechChar g_wolfClanHallArchiveLabel[0x18] = "ARCHIVE HOLOPROJECTOR";

// GLOBAL: MW2SHELL 0x100700bc
MechChar g_wolfClanHallReadyRoomLabel[0x0c] = "READY ROOM";

// GLOBAL: MW2SHELL 0x100700c8
MechChar g_wolfClanHallRegisterLabel[0x0c] = "REGISTER";

// GLOBAL: MW2SHELL 0x100700d4
MechChar g_wolfClanHallExitLabel[0x08] = "~EXIT";

// GLOBAL: MW2SHELL 0x100700dc
MechChar g_wolfRosterNewAllegianceLabel[0x14] = "<~NEW ALLEGIANCE";

// The ten pilot slots, which show the callsigns instead.
// GLOBAL: MW2SHELL 0x100700f0
MechChar g_wolfRosterSlot0Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100700f4
MechChar g_wolfRosterSlot1Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100700f8
MechChar g_wolfRosterSlot2Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100700fc
MechChar g_wolfRosterSlot3Label[0x04] = "";

// GLOBAL: MW2SHELL 0x10070100
MechChar g_wolfRosterSlot4Label[0x04] = "";

// GLOBAL: MW2SHELL 0x10070104
MechChar g_wolfRosterSlot5Label[0x04] = "";

// GLOBAL: MW2SHELL 0x10070108
MechChar g_wolfRosterSlot6Label[0x04] = "";

// GLOBAL: MW2SHELL 0x1007010c
MechChar g_wolfRosterSlot7Label[0x04] = "";

// GLOBAL: MW2SHELL 0x10070110
MechChar g_wolfRosterSlot8Label[0x04] = "";

// GLOBAL: MW2SHELL 0x10070114
MechChar g_wolfRosterSlot9Label[0x04] = "";

// GLOBAL: MW2SHELL 0x10070118
MechChar g_wolfRosterAcceptLabel[0x0c] = "<~ACCEPT";

// GLOBAL: MW2SHELL 0x10070124
MechChar g_wolfRosterDeleteMechwarriorLabel[0x18] = "<~DELETE MECHWARRIOR";

// GLOBAL: MW2SHELL 0x1007013c
MechChar g_wolfRosterLaunchOldMissionLabel[0x18] = "<~LAUNCH OLD MISSION";

// GLOBAL: MW2SHELL 0x10070154
MechChar g_wolfRosterPilotInfoLabel[0x10] = "<~PILOT INFO";

// GLOBAL: MW2SHELL 0x10070164
MechChar g_wolfArchiveExitLabel[0x08] = "~EXIT";

// GLOBAL: MW2SHELL 0x1007016c
MechChar g_wolfArchivePrevPageLabel[0x0c] = "~PREV PAGE";

// GLOBAL: MW2SHELL 0x10070178
MechChar g_wolfArchiveNextPageLabel[0x0c] = "~NEXT PAGE";

// GLOBAL: MW2SHELL 0x10070184
MechChar g_wolfArchiveBackLabel[0x08] = "~BACK";

// GLOBAL: MW2SHELL 0x1007018c
MechChar g_wolfCadetTrainingClanHallLabel[0x0c] = "CLAN HALL";

// GLOBAL: MW2SHELL 0x10070198
MechChar g_wolfCadetTrainingNavComputerLabel[0x10] = "<~NAV COMPUTER";

// GLOBAL: MW2SHELL 0x100701a8
MechChar g_wolfCadetTrainingMechHandlingLabel[0x10] = "<~MECH HANDLING";

// GLOBAL: MW2SHELL 0x100701b8
MechChar g_wolfCadetTrainingWeaponsUsageLabel[0x10] = "<~WEAPONS USAGE";

// GLOBAL: MW2SHELL 0x100701c8
MechChar g_wolfCadetTrainingHuntingLabel[0x0c] = "<~HUNTING";

// GLOBAL: MW2SHELL 0x100701d4
MechChar g_wolfCadetTrainingInspectionLabel[0x10] = "<~INSPECTION";

// GLOBAL: MW2SHELL 0x100701e4
MechChar g_wolfCadetTrainingTrialLabel[0x08] = "<~TRIAL";

// GLOBAL: MW2SHELL 0x100701ec
MechChar g_wolfReadyRoomClanHallLabel[0x0c] = "CLAN HALL";

// GLOBAL: MW2SHELL 0x100701f8
MechChar g_wolfReadyRoomMechLabLabel[0x0c] = "~MECH LAB";

// GLOBAL: MW2SHELL 0x10070204
MechChar g_wolfReadyRoomStarConfigLabel[0x10] = "~STAR CONFIG";

// GLOBAL: MW2SHELL 0x10070214
MechChar g_wolfReadyRoomMissionBriefingLabel[0x14] = "~MISSION BRIEFING";

// The campaign's missions, in g_wolfMissions' order, by their scenario names.
// GLOBAL: MW2SHELL 0x10070228
MechChar g_wolfReadyRoomYellowLabel[0x0c] = "<~YELLOW";

// GLOBAL: MW2SHELL 0x10070234
MechChar g_wolfReadyRoomOrangeLabel[0x0c] = "<~ORANGE";

// GLOBAL: MW2SHELL 0x10070240
MechChar g_wolfReadyRoomTealLabel[0x08] = "<~TEAL";

// GLOBAL: MW2SHELL 0x10070248
MechChar g_wolfReadyRoomTaupeLabel[0x08] = "<~TAUPE";

// GLOBAL: MW2SHELL 0x10070250
MechChar g_wolfReadyRoomJennyLabel[0x08] = "<~JENNY";

// GLOBAL: MW2SHELL 0x10070258
MechChar g_wolfReadyRoomSableLabel[0x08] = "<~SABLE";

// GLOBAL: MW2SHELL 0x10070260
MechChar g_wolfReadyRoomGreyLabel[0x08] = "<~GREY";

// GLOBAL: MW2SHELL 0x10070268
MechChar g_wolfReadyRoomBrownLabel[0x08] = "<~BROWN";

// GLOBAL: MW2SHELL 0x10070270
MechChar g_wolfReadyRoomAmyLabel[0x08] = "<~AMY";

// GLOBAL: MW2SHELL 0x10070278
MechChar g_wolfReadyRoomSilverLabel[0x0c] = "<~SILVER";

// GLOBAL: MW2SHELL 0x10070284
MechChar g_wolfReadyRoomAquaLabel[0x08] = "<~AQUA";

// GLOBAL: MW2SHELL 0x1007028c
MechChar g_wolfReadyRoomKimLabel[0x08] = "<~KIM";

// GLOBAL: MW2SHELL 0x10070294
MechChar g_wolfReadyRoomCyanLabel[0x08] = "<~CYAN";

// GLOBAL: MW2SHELL 0x1007029c
MechChar g_wolfReadyRoomMaroonLabel[0x0c] = "<~MAROON";

// GLOBAL: MW2SHELL 0x100702a8
MechChar g_wolfReadyRoomGoldLabel[0x08] = "<~GOLD";

// GLOBAL: MW2SHELL 0x100702b0
MechChar g_wolfReadyRoomIreneLabel[0x08] = "<~IRENE";

// GLOBAL: MW2SHELL 0x100702b8
MechChar g_wolfBriefingAbortLabel[0x08] = "<~ABORT";

// GLOBAL: MW2SHELL 0x100702c0
MechChar g_wolfBriefingSituationLabel[0x0c] = "<~SITUATION";

// GLOBAL: MW2SHELL 0x100702cc
MechChar g_wolfBriefingLaunchLabel[0x0c] = "<~LAUNCH";

// GLOBAL: MW2SHELL 0x100702d8
MechChar g_wolfBriefingSkipLabel[0x08] = "<~SKIP";

// GLOBAL: MW2SHELL 0x100702e0
MechChar g_wolfSituationExitLabel[0x08] = "<~EXIT";

// GLOBAL: MW2SHELL 0x100702e8
MechChar g_wolfSituationPrevPageLabel[0x0c] = "<~PREV PAGE";

// GLOBAL: MW2SHELL 0x100702f4
MechChar g_wolfSituationNextPageLabel[0x0c] = "<~NEXT PAGE";

// ArchiveReader's BACK button, placed off screen.
// GLOBAL: MW2SHELL 0x10070300
MechChar g_wolfSituationBackLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x10070304
MechChar g_wolfDebriefExitLabel[0x08] = "<~EXIT";

// GLOBAL: MW2SHELL 0x1007030c
MechChar g_wolfDebriefAftermathLabel[0x0c] = "<~AFTERMATH";

// GLOBAL: MW2SHELL 0x10070318
MechChar g_wolfDebriefReplayLabel[0x0c] = "<~REPLAY";

// GLOBAL: MW2SHELL 0x10070324
MechChar g_wolfAftermathExitLabel[0x08] = "<~EXIT";

// GLOBAL: MW2SHELL 0x1007032c
MechChar g_wolfAftermathPrevPageLabel[0x0c] = "<~PREV PAGE";

// GLOBAL: MW2SHELL 0x10070338
MechChar g_wolfAftermathNextPageLabel[0x0c] = "<~NEXT PAGE";

// GLOBAL: MW2SHELL 0x10070344
MechChar g_wolfAftermathBackLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x10070348
MechChar g_wolfStarConfigExitConfigLabel[0x10] = "<~EXIT CONFIG";

// GLOBAL: MW2SHELL 0x10070358
MechChar g_wolfStarConfigMechLabLabel[0x0c] = "~MECH LAB";

// GLOBAL: MW2SHELL 0x10070364
MechChar g_wolfStarConfigNextFormationLabel[0x10] = "NEXT FORMATION";

// GLOBAL: MW2SHELL 0x10070374
MechChar g_wolfStarConfigPrevFormationLabel[0x10] = "PREV FORMATION";

// GLOBAL: MW2SHELL 0x10070384
MechChar g_wolfStarConfigAddStarmateLabel[0x10] = "ADD STARMATE";

// GLOBAL: MW2SHELL 0x10070394
MechChar g_wolfStarConfigDeleteStarmateLabel[0x10] = "DELETE STARMATE";

// GLOBAL: MW2SHELL 0x100703a4
MechChar g_wolfStarConfigChangeMech0Label[0x0c] = "CHANGE MECH";

// GLOBAL: MW2SHELL 0x100703b0
MechChar g_wolfStarConfigChangeMech1Label[0x0c] = "CHANGE MECH";

// GLOBAL: MW2SHELL 0x100703bc
MechChar g_wolfStarConfigChangeMech2Label[0x0c] = "CHANGE MECH";

// GLOBAL: MW2SHELL 0x100703c8
MechChar g_wolfMechBayExitLabLabel[0x0c] = "<~EXIT LAB";

// GLOBAL: MW2SHELL 0x100703d4
MechChar g_wolfMechBayStarConfigLabel[0x10] = "~STAR CONFIG";

// GLOBAL: MW2SHELL 0x100703e4
MechChar g_wolfMechBayNextChassisLabel[0x10] = "NEXT CHASSIS";

// GLOBAL: MW2SHELL 0x100703f4
MechChar g_wolfMechBayPrevChassisLabel[0x10] = "PREV CHASSIS";

// GLOBAL: MW2SHELL 0x10070404
MechChar g_wolfMechBayNextVariantLabel[0x10] = "NEXT VARIANT";

// GLOBAL: MW2SHELL 0x10070414
MechChar g_wolfMechBayPrevVariantLabel[0x10] = "PREV VARIANT";

// GLOBAL: MW2SHELL 0x10070424
MechChar g_wolfMechBayCustomizeLabel[0x0c] = "<~CUSTOMIZE";

// GLOBAL: MW2SHELL 0x10070430
MechChar g_wolfMechBayAcceptMechLabel[0x10] = "<~ACCEPT MECH";

// GLOBAL: MW2SHELL 0x10070440
MechChar g_wolfMechBaySaveLabel[0x08] = "<~SAVE";

// GLOBAL: MW2SHELL 0x10070448
MechChar g_wolfMechBayAbortLabel[0x08] = "<~ABORT";

// GLOBAL: MW2SHELL 0x10070450
MechChar g_wolfMechBayDeleteLabel[0x0c] = "<~DELETE";

// GLOBAL: MW2SHELL 0x1007045c
MechChar g_jadeFalconClanHallCadetTrainingLabel[0x10] = "CADET TRAINING";

// GLOBAL: MW2SHELL 0x1007046c
MechChar g_jadeFalconClanHallArchiveLabel[0x18] = "ARCHIVE HOLOPROJECTOR";

// GLOBAL: MW2SHELL 0x10070484
MechChar g_jadeFalconClanHallReadyRoomLabel[0x0c] = "READY ROOM";

// GLOBAL: MW2SHELL 0x10070490
MechChar g_jadeFalconClanHallRegisterLabel[0x0c] = "REGISTER";

// GLOBAL: MW2SHELL 0x1007049c
MechChar g_jadeFalconClanHallExitLabel[0x08] = "~EXIT";

// GLOBAL: MW2SHELL 0x100704a4
MechChar g_jadeFalconRosterNewAllegianceLabel[0x14] = "<~NEW ALLEGIANCE";

// GLOBAL: MW2SHELL 0x100704b8
MechChar g_jadeFalconRosterSlot0Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100704bc
MechChar g_jadeFalconRosterSlot1Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100704c0
MechChar g_jadeFalconRosterSlot2Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100704c4
MechChar g_jadeFalconRosterSlot3Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100704c8
MechChar g_jadeFalconRosterSlot4Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100704cc
MechChar g_jadeFalconRosterSlot5Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100704d0
MechChar g_jadeFalconRosterSlot6Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100704d4
MechChar g_jadeFalconRosterSlot7Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100704d8
MechChar g_jadeFalconRosterSlot8Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100704dc
MechChar g_jadeFalconRosterSlot9Label[0x04] = "";

// GLOBAL: MW2SHELL 0x100704e0
MechChar g_jadeFalconRosterAcceptLabel[0x0c] = "<~ACCEPT";

// GLOBAL: MW2SHELL 0x100704ec
MechChar g_jadeFalconRosterDeleteMechwarriorLabel[0x18] = "<~DELETE MECHWARRIOR";

// GLOBAL: MW2SHELL 0x10070504
MechChar g_jadeFalconRosterLaunchOldMissionLabel[0x18] = "<~LAUNCH OLD MISSION";

// GLOBAL: MW2SHELL 0x1007051c
MechChar g_jadeFalconRosterPilotInfoLabel[0x10] = "<~PILOT INFO";

// GLOBAL: MW2SHELL 0x1007052c
MechChar g_jadeFalconArchiveExitLabel[0x08] = "~EXIT";

// GLOBAL: MW2SHELL 0x10070534
MechChar g_jadeFalconArchivePrevPageLabel[0x0c] = "~PREV PAGE";

// GLOBAL: MW2SHELL 0x10070540
MechChar g_jadeFalconArchiveNextPageLabel[0x0c] = "~NEXT PAGE";

// GLOBAL: MW2SHELL 0x1007054c
MechChar g_jadeFalconArchiveBackLabel[0x08] = "~BACK";

// GLOBAL: MW2SHELL 0x10070554
MechChar g_jadeFalconCadetTrainingClanHallLabel[0x0c] = "CLAN HALL";

// GLOBAL: MW2SHELL 0x10070560
MechChar g_jadeFalconCadetTrainingNavComputerLabel[0x10] = "<~NAV COMPUTER";

// GLOBAL: MW2SHELL 0x10070570
MechChar g_jadeFalconCadetTrainingMechHandlingLabel[0x10] = "<~MECH HANDLING";

// GLOBAL: MW2SHELL 0x10070580
MechChar g_jadeFalconCadetTrainingWeaponsUsageLabel[0x10] = "<~WEAPONS USAGE";

// GLOBAL: MW2SHELL 0x10070590
MechChar g_jadeFalconCadetTrainingHuntingLabel[0x0c] = "<~HUNTING";

// GLOBAL: MW2SHELL 0x1007059c
MechChar g_jadeFalconCadetTrainingInspectionLabel[0x10] = "<~INSPECTION";

// GLOBAL: MW2SHELL 0x100705ac
MechChar g_jadeFalconCadetTrainingTrialLabel[0x08] = "<~TRIAL";

// GLOBAL: MW2SHELL 0x100705b4
MechChar g_jadeFalconReadyRoomClanHallLabel[0x0c] = "CLAN HALL";

// GLOBAL: MW2SHELL 0x100705c0
MechChar g_jadeFalconReadyRoomMechLabLabel[0x0c] = "~MECH LAB";

// GLOBAL: MW2SHELL 0x100705cc
MechChar g_jadeFalconReadyRoomStarConfigLabel[0x10] = "~STAR CONFIG";

// GLOBAL: MW2SHELL 0x100705dc
MechChar g_jadeFalconReadyRoomMissionBriefingLabel[0x14] = "~MISSION BRIEFING";

// GLOBAL: MW2SHELL 0x100705f0
MechChar g_jadeFalconReadyRoomPinkLabel[0x08] = "<~PINK";

// GLOBAL: MW2SHELL 0x100705f8
MechChar g_jadeFalconReadyRoomGreenLabel[0x08] = "<~GREEN";

// GLOBAL: MW2SHELL 0x10070600
MechChar g_jadeFalconReadyRoomRedLabel[0x08] = "<~RED";

// GLOBAL: MW2SHELL 0x10070608
MechChar g_jadeFalconReadyRoomFuchsiaLabel[0x0c] = "<~FUCHSIA";

// GLOBAL: MW2SHELL 0x10070614
MechChar g_jadeFalconReadyRoomCindyLabel[0x08] = "<~CINDY";

// GLOBAL: MW2SHELL 0x1007061c
MechChar g_jadeFalconReadyRoomRustLabel[0x08] = "<~RUST";

// GLOBAL: MW2SHELL 0x10070624
MechChar g_jadeFalconReadyRoomUmberLabel[0x08] = "<~UMBER";

// GLOBAL: MW2SHELL 0x1007062c
MechChar g_jadeFalconReadyRoomTanLabel[0x08] = "<~TAN";

// GLOBAL: MW2SHELL 0x10070634
MechChar g_jadeFalconReadyRoomHeidiLabel[0x08] = "<~HEIDI";

// GLOBAL: MW2SHELL 0x1007063c
MechChar g_jadeFalconReadyRoomPlumLabel[0x08] = "<~PLUM";

// GLOBAL: MW2SHELL 0x10070644
MechChar g_jadeFalconReadyRoomWhiteLabel[0x08] = "<~WHITE";

// GLOBAL: MW2SHELL 0x1007064c
MechChar g_jadeFalconReadyRoomJillLabel[0x08] = "<~JILL";

// GLOBAL: MW2SHELL 0x10070654
MechChar g_jadeFalconReadyRoomPuceLabel[0x08] = "<~PUCE";

// GLOBAL: MW2SHELL 0x1007065c
MechChar g_jadeFalconReadyRoomBlondeLabel[0x0c] = "<~BLONDE";

// GLOBAL: MW2SHELL 0x10070668
MechChar g_jadeFalconReadyRoomBronzeLabel[0x0c] = "<~BRONZE";

// GLOBAL: MW2SHELL 0x10070674
MechChar g_jadeFalconReadyRoomMaryLabel[0x08] = "<~MARY";

// GLOBAL: MW2SHELL 0x1007067c
MechChar g_jadeFalconBriefingAbortLabel[0x08] = "<~ABORT";

// GLOBAL: MW2SHELL 0x10070684
MechChar g_jadeFalconBriefingSituationLabel[0x0c] = "<~SITUATION";

// GLOBAL: MW2SHELL 0x10070690
MechChar g_jadeFalconBriefingLaunchLabel[0x0c] = "<~LAUNCH";

// GLOBAL: MW2SHELL 0x1007069c
MechChar g_jadeFalconBriefingSkipLabel[0x08] = "<~SKIP";

// GLOBAL: MW2SHELL 0x100706a4
MechChar g_jadeFalconSituationExitLabel[0x08] = "<~EXIT";

// GLOBAL: MW2SHELL 0x100706ac
MechChar g_jadeFalconSituationPrevPageLabel[0x0c] = "<~PREV PAGE";

// GLOBAL: MW2SHELL 0x100706b8
MechChar g_jadeFalconSituationNextPageLabel[0x0c] = "<~NEXT PAGE";

// GLOBAL: MW2SHELL 0x100706c4
MechChar g_jadeFalconSituationBackLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100706c8
MechChar g_jadeFalconDebriefExitLabel[0x08] = "<~EXIT";

// GLOBAL: MW2SHELL 0x100706d0
MechChar g_jadeFalconDebriefAftermathLabel[0x0c] = "<~AFTERMATH";

// GLOBAL: MW2SHELL 0x100706dc
MechChar g_jadeFalconDebriefReplayLabel[0x0c] = "<~REPLAY";

// GLOBAL: MW2SHELL 0x100706e8
MechChar g_jadeFalconAftermathExitLabel[0x08] = "<~EXIT";

// GLOBAL: MW2SHELL 0x100706f0
MechChar g_jadeFalconAftermathPrevPageLabel[0x0c] = "<~PREV PAGE";

// GLOBAL: MW2SHELL 0x100706fc
MechChar g_jadeFalconAftermathNextPageLabel[0x0c] = "<~NEXT PAGE";

// GLOBAL: MW2SHELL 0x10070708
MechChar g_jadeFalconAftermathBackLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x1007070c
MechChar g_jadeFalconStarConfigExitConfigLabel[0x10] = "<~EXIT CONFIG";

// GLOBAL: MW2SHELL 0x1007071c
MechChar g_jadeFalconStarConfigMechLabLabel[0x0c] = "~MECH LAB";

// GLOBAL: MW2SHELL 0x10070728
MechChar g_jadeFalconStarConfigNextFormationLabel[0x10] = "NEXT FORMATION";

// GLOBAL: MW2SHELL 0x10070738
MechChar g_jadeFalconStarConfigPrevFormationLabel[0x10] = "PREV FORMATION";

// GLOBAL: MW2SHELL 0x10070748
MechChar g_jadeFalconStarConfigAddStarmateLabel[0x10] = "ADD STARMATE";

// GLOBAL: MW2SHELL 0x10070758
MechChar g_jadeFalconStarConfigDeleteStarmateLabel[0x10] = "DELETE STARMATE";

// GLOBAL: MW2SHELL 0x10070768
MechChar g_jadeFalconStarConfigChangeMech0Label[0x0c] = "CHANGE MECH";

// GLOBAL: MW2SHELL 0x10070774
MechChar g_jadeFalconStarConfigChangeMech1Label[0x0c] = "CHANGE MECH";

// GLOBAL: MW2SHELL 0x10070780
MechChar g_jadeFalconStarConfigChangeMech2Label[0x0c] = "CHANGE MECH";

// GLOBAL: MW2SHELL 0x1007078c
MechChar g_jadeFalconMechBayExitLabLabel[0x0c] = "<~EXIT LAB";

// GLOBAL: MW2SHELL 0x10070798
MechChar g_jadeFalconMechBayStarConfigLabel[0x10] = "~STAR CONFIG";

// GLOBAL: MW2SHELL 0x100707a8
MechChar g_jadeFalconMechBayNextChassisLabel[0x10] = "NEXT CHASSIS";

// GLOBAL: MW2SHELL 0x100707b8
MechChar g_jadeFalconMechBayPrevChassisLabel[0x10] = "PREV CHASSIS";

// GLOBAL: MW2SHELL 0x100707c8
MechChar g_jadeFalconMechBayNextVariantLabel[0x10] = "NEXT VARIANT";

// GLOBAL: MW2SHELL 0x100707d8
MechChar g_jadeFalconMechBayPrevVariantLabel[0x10] = "PREV VARIANT";

// GLOBAL: MW2SHELL 0x100707e8
MechChar g_jadeFalconMechBayCustomizeLabel[0x0c] = "<~CUSTOMIZE";

// GLOBAL: MW2SHELL 0x100707f4
MechChar g_jadeFalconMechBayAcceptMechLabel[0x10] = "<~ACCEPT MECH";

// GLOBAL: MW2SHELL 0x10070804
MechChar g_jadeFalconMechBaySaveLabel[0x08] = "<~SAVE";

// GLOBAL: MW2SHELL 0x1007080c
MechChar g_jadeFalconMechBayAbortLabel[0x08] = "<~ABORT";

// GLOBAL: MW2SHELL 0x10070814
MechChar g_jadeFalconMechBayDeleteLabel[0x0c] = "<~DELETE";

// GLOBAL: MW2SHELL 0x10070820
MechChar g_mainMenuTrialsOfGrievanceLabel[0x18] = "~TRIALS OF GRIEVANCE";

// GLOBAL: MW2SHELL 0x10070838
MechChar g_mainMenuWolfClanHallLabel[0x10] = "~WOLF CLAN HALL";

// GLOBAL: MW2SHELL 0x10070848
MechChar g_mainMenuJadeFalconClanHallLabel[0x18] = "~JADE FALCON CLAN HALL";

// GLOBAL: MW2SHELL 0x10070860
MechChar g_mainMenuExitLabel[0x08] = "~EXIT";

// The Trials of Grievance's ready room (missionui.cpp) labels only EXIT. Each ...Next/...Prev pair
// steps a mech's chassis or the star's formation, and ...Clan steps the star's clan.
// GLOBAL: MW2SHELL 0x1007088c
MechChar g_missionBriefingLaunchLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x10070890
MechChar g_missionBriefingExitLabel[0x08] = "<~EXIT";

// GLOBAL: MW2SHELL 0x10070898
MechChar g_missionBriefingMissionLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x1007089c
MechChar g_missionBriefingPlayerMech0NextLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708a0
MechChar g_missionBriefingPlayerMech1NextLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708a4
MechChar g_missionBriefingPlayerMech2NextLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708a8
MechChar g_missionBriefingPlayerFormationNextLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708ac
MechChar g_missionBriefingPlayerMech0PrevLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708b0
MechChar g_missionBriefingPlayerMech1PrevLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708b4
MechChar g_missionBriefingPlayerMech2PrevLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708b8
MechChar g_missionBriefingPlayerFormationPrevLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708bc
MechChar g_missionBriefingPlayerClanLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708c0
MechChar g_missionBriefingPlayerMechLabLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708c4
MechChar g_missionBriefingPlayerStarConfigLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708c8
MechChar g_missionBriefingEnemyMech0NextLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708cc
MechChar g_missionBriefingEnemyMech1NextLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708d0
MechChar g_missionBriefingEnemyMech2NextLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708d4
MechChar g_missionBriefingEnemyFormationNextLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708d8
MechChar g_missionBriefingEnemyMech0PrevLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708dc
MechChar g_missionBriefingEnemyMech1PrevLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708e0
MechChar g_missionBriefingEnemyMech2PrevLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708e4
MechChar g_missionBriefingEnemyFormationPrevLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708e8
MechChar g_missionBriefingEnemyClanLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708ec
MechChar g_missionBriefingEnemyMechLabLabel[0x04] = "";

// GLOBAL: MW2SHELL 0x100708f0
MechChar g_missionBriefingEnemyStarConfigLabel[0x04] = "";

// Tab stops for the "\T" text escape, in pixels from the left edge.
// GLOBAL: MW2SHELL 0x1006e150
MechS32 g_textTabStops[19] =
	{0, 36, 72, 108, 144, 180, 216, 252, 288, 324, 360, 396, 432, 468, 504, 540, 576, 612, 640};

// The shell's database, for TMPackDataBase.
// GLOBAL: MW2SHELL 0x1006e19c
char* g_databaseName = "DATABASE.MW2";

// The clan hall archives, by campaign.
// GLOBAL: MW2SHELL 0x1006e1a0
MechChar* g_archiveNames[2] = {"ARCHWO.MW2", "ARCHJF.MW2"};

// The star formations, as the mission briefing screen names them.
// GLOBAL: MW2SHELL 0x1006e1a8
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
// GLOBAL: MW2SHELL 0x1006e1d8
MechS32 g_echelonLeftPositions[6] = {285, 176, 344, 222, 450, 320};

// GLOBAL: MW2SHELL 0x1006e1f0
MechS32 g_echelonRightPositions[6] = {520, 212, 342, 225, 143, 253};

// GLOBAL: MW2SHELL 0x1006e208
MechS32 g_lineAbreastPositions[6] = {222, 199, 341, 216, 498, 242};

// GLOBAL: MW2SHELL 0x1006e220
MechS32 g_lineAsternPositions[6] = {385, 184, 343, 221, 264, 290};

// GLOBAL: MW2SHELL 0x1006e238
MechS32 g_vFormPositions[6] = {269, 176, 506, 205, 269, 283};

// GLOBAL: MW2SHELL 0x1006e250
MechS32 g_wedgePositions[6] = {376, 186, 139, 254, 452, 325};

// The buttons of each screen, Wolf's first. The Trials of Grievance use Wolf's.
// GLOBAL: MW2SHELL 0x1006e268
MainMenuButton g_wolfClanHallButtons[5] = {
	{185, 280, 240, 400, 197, 327, g_wolfClanHallCadetTrainingLabel},
	{320, 300, 470, 400, 246, 371, g_wolfClanHallArchiveLabel},
	{20, 245, 90, 411, 25, 307, g_wolfClanHallReadyRoomLabel},
	{95, 385, 180, 479, 80, 455, g_wolfClanHallRegisterLabel},
	{0, 0, 639, 40, 320, 15, g_wolfClanHallExitLabel},
};

// GLOBAL: MW2SHELL 0x1006e2f8
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

// GLOBAL: MW2SHELL 0x1006e4a0
MainMenuButton g_wolfArchiveButtons[4] = {
	{0, 440, 639, 479, 320, 455, g_wolfArchiveExitLabel},
	{405, 372, 479, 405, 449, 354, g_wolfArchivePrevPageLabel},
	{480, 372, 556, 405, 511, 408, g_wolfArchiveNextPageLabel},
	{67, 358, 120, 410, 92, 408, g_wolfArchiveBackLabel},
};

// GLOBAL: MW2SHELL 0x1006e510
MainMenuButton g_wolfCadetTrainingButtons[7] = {
	{5, 149, 57, 440, 18, 203, g_wolfCadetTrainingClanHallLabel},
	{450, 13, 629, 37, 540, 18, g_wolfCadetTrainingNavComputerLabel},
	{450, 38, 629, 62, 540, 43, g_wolfCadetTrainingMechHandlingLabel},
	{450, 63, 629, 87, 540, 68, g_wolfCadetTrainingWeaponsUsageLabel},
	{450, 88, 629, 112, 540, 93, g_wolfCadetTrainingHuntingLabel},
	{450, 113, 629, 137, 540, 118, g_wolfCadetTrainingInspectionLabel},
	{450, 138, 629, 162, 540, 143, g_wolfCadetTrainingTrialLabel},
};

// GLOBAL: MW2SHELL 0x1006e5d8
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

// GLOBAL: MW2SHELL 0x1006e808
MainMenuButton g_wolfBriefingButtons[4] = {
	{430, 450, 529, 474, 480, 455, g_wolfBriefingAbortLabel},
	{110, 450, 209, 474, 160, 455, g_wolfBriefingSituationLabel},
	{270, 450, 369, 474, 320, 455, g_wolfBriefingLaunchLabel},
	{540, 450, 639, 474, 590, 455, g_wolfBriefingSkipLabel},
};

// GLOBAL: MW2SHELL 0x1006e878
MainMenuButton g_wolfSituationButtons[4] = {
	{110, 450, 209, 474, 160, 455, g_wolfSituationExitLabel},
	{270, 450, 369, 474, 320, 455, g_wolfSituationPrevPageLabel},
	{430, 450, 529, 474, 480, 455, g_wolfSituationNextPageLabel},
	{510, 550, 609, 574, 560, 555, g_wolfSituationBackLabel},
};

// GLOBAL: MW2SHELL 0x1006e8e8
MainMenuButton g_wolfDebriefButtons[3] = {
	{270, 450, 369, 474, 320, 455, g_wolfDebriefExitLabel},
	{110, 450, 209, 474, 160, 455, g_wolfDebriefAftermathLabel},
	{430, 450, 529, 474, 480, 455, g_wolfDebriefReplayLabel},
};

// GLOBAL: MW2SHELL 0x1006e940
MainMenuButton g_wolfAftermathButtons[4] = {
	{110, 450, 209, 474, 160, 455, g_wolfAftermathExitLabel},
	{270, 450, 369, 474, 320, 455, g_wolfAftermathPrevPageLabel},
	{430, 450, 529, 474, 480, 455, g_wolfAftermathNextPageLabel},
	{510, 550, 609, 574, 560, 555, g_wolfAftermathBackLabel},
};

// GLOBAL: MW2SHELL 0x1006e9b0
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

// GLOBAL: MW2SHELL 0x1006eab0
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

// GLOBAL: MW2SHELL 0x1006ebe8
MainMenuButton g_jadeFalconClanHallButtons[5] = {
	{66, 167, 152, 303, 69, 187, g_jadeFalconClanHallCadetTrainingLabel},
	{160, 290, 375, 322, 110, 340, g_jadeFalconClanHallArchiveLabel},
	{523, 154, 636, 332, 450, 200, g_jadeFalconClanHallReadyRoomLabel},
	{397, 385, 492, 466, 397, 443, g_jadeFalconClanHallRegisterLabel},
	{0, 0, 639, 40, 320, 15, g_jadeFalconClanHallExitLabel},
};

// GLOBAL: MW2SHELL 0x1006ec78
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

// GLOBAL: MW2SHELL 0x1006ee20
MainMenuButton g_jadeFalconArchiveButtons[4] = {
	{0, 440, 639, 479, 320, 455, g_jadeFalconArchiveExitLabel},
	{405, 364, 479, 397, 449, 346, g_jadeFalconArchivePrevPageLabel},
	{480, 364, 556, 397, 511, 400, g_jadeFalconArchiveNextPageLabel},
	{67, 358, 120, 410, 92, 400, g_jadeFalconArchiveBackLabel},
};

// GLOBAL: MW2SHELL 0x1006ee90
MainMenuButton g_jadeFalconCadetTrainingButtons[7] = {
	{5, 149, 57, 440, 18, 203, g_jadeFalconCadetTrainingClanHallLabel},
	{450, 13, 629, 37, 540, 18, g_jadeFalconCadetTrainingNavComputerLabel},
	{450, 38, 629, 62, 540, 43, g_jadeFalconCadetTrainingMechHandlingLabel},
	{450, 63, 629, 87, 540, 68, g_jadeFalconCadetTrainingWeaponsUsageLabel},
	{450, 88, 629, 112, 540, 93, g_jadeFalconCadetTrainingHuntingLabel},
	{450, 113, 629, 137, 540, 118, g_jadeFalconCadetTrainingInspectionLabel},
	{450, 138, 629, 162, 540, 143, g_jadeFalconCadetTrainingTrialLabel},
};

// GLOBAL: MW2SHELL 0x1006ef58
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

// GLOBAL: MW2SHELL 0x1006f188
MainMenuButton g_jadeFalconBriefingButtons[4] = {
	{430, 450, 529, 474, 480, 455, g_jadeFalconBriefingAbortLabel},
	{110, 450, 209, 474, 160, 455, g_jadeFalconBriefingSituationLabel},
	{270, 450, 369, 474, 320, 455, g_jadeFalconBriefingLaunchLabel},
	{540, 450, 639, 474, 590, 455, g_jadeFalconBriefingSkipLabel},
};

// GLOBAL: MW2SHELL 0x1006f1f8
MainMenuButton g_jadeFalconSituationButtons[4] = {
	{110, 450, 209, 474, 160, 455, g_jadeFalconSituationExitLabel},
	{270, 450, 369, 474, 320, 455, g_jadeFalconSituationPrevPageLabel},
	{430, 450, 529, 474, 480, 455, g_jadeFalconSituationNextPageLabel},
	{510, 550, 609, 574, 560, 555, g_jadeFalconSituationBackLabel},
};

// GLOBAL: MW2SHELL 0x1006f268
MainMenuButton g_jadeFalconDebriefButtons[3] = {
	{270, 450, 369, 474, 320, 455, g_jadeFalconDebriefExitLabel},
	{110, 450, 209, 474, 160, 455, g_jadeFalconDebriefAftermathLabel},
	{430, 450, 529, 474, 480, 455, g_jadeFalconDebriefReplayLabel},
};

// GLOBAL: MW2SHELL 0x1006f2c0
MainMenuButton g_jadeFalconAftermathButtons[4] = {
	{110, 450, 209, 474, 160, 455, g_jadeFalconAftermathExitLabel},
	{270, 450, 369, 474, 320, 455, g_jadeFalconAftermathPrevPageLabel},
	{430, 450, 529, 474, 480, 455, g_jadeFalconAftermathNextPageLabel},
	{510, 550, 609, 574, 560, 555, g_jadeFalconAftermathBackLabel},
};

// GLOBAL: MW2SHELL 0x1006f330
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

// GLOBAL: MW2SHELL 0x1006f430
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

// GLOBAL: MW2SHELL 0x1006f568
MainMenuButton g_mainMenuButtons[4] = {
	{0xdb, 0x126, 0x1aa, 0x1a3, 0x140, 0x18b, g_mainMenuTrialsOfGrievanceLabel},
	{0x1ab, 0xf5, 0x27a, 0x175, 0x20d, 0x176, g_mainMenuWolfClanHallLabel},
	{0x0a, 0xc5, 0xc8, 0x172, 0x7c, 0x176, g_mainMenuJadeFalconClanHallLabel},
	{0, 0x1c2, 0x27f, 0x1df, 0x140, 0x1c7, g_mainMenuExitLabel},
};

// The mission briefing screen (missionui.cpp): both stars' mechs and formations.
// GLOBAL: MW2SHELL 0x1006f618
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
// GLOBAL: MW2SHELL 0x1006fc90
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

// GLOBAL: MW2SHELL 0x1006fd30
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

// GLOBAL: MW2SHELL 0x1006fdd0
CampaignMission* g_campaignMissions[2] = {g_wolfMissions, g_jadeFalconMissions};

// The training missions of each campaign, one per button from the second.
// GLOBAL: MW2SHELL 0x1006fdd8
MechChar* g_wolfTrainingScenarios[6] = {"tnw1SCN1", "tnw2SCN1", "tnw3SCN1", "tnw4SCN1", "tnw5SCN1", "tnw6SCN1"};

// GLOBAL: MW2SHELL 0x1006fdf0
MechChar* g_jadeFalconTrainingScenarios[6] = {"tnj1SCN1", "tnj2SCN1", "tnj3SCN1", "tnj4SCN1", "tnj5SCN1", "tnj6SCN1"};

// GLOBAL: MW2SHELL 0x1006fe08
MechChar** g_trainingScenarios[2] = {g_wolfTrainingScenarios, g_jadeFalconTrainingScenarios};

// The screens' MenuScreen tables, by campaign.
// GLOBAL: MW2SHELL 0x1006fe10
MenuScreen g_clanHallScreens[3] = {
	{g_wolfClanHallButtons, 5, 11, 0x24},
	{g_jadeFalconClanHallButtons, 5, 18, 0x27},
	{NULL, 0, 0, 0},
};

// GLOBAL: MW2SHELL 0x1006fe40
MenuScreen g_rosterScreens[3] = {
	{g_wolfRosterButtons, 15, 17, -1},
	{g_jadeFalconRosterButtons, 15, 24, -1},
	{NULL, 0, 0, 0},
};

// GLOBAL: MW2SHELL 0x1006fe70
MenuScreen g_archiveScreens[3] = {
	{g_wolfArchiveButtons, 4, 12, -1},
	{g_jadeFalconArchiveButtons, 4, 19, -1},
	{NULL, 0, 0, 0},
};

// GLOBAL: MW2SHELL 0x1006fea0
MenuScreen g_starConfigScreens[3] = {
	{g_wolfStarConfigButtons, 9, 15, -1},
	{g_jadeFalconStarConfigButtons, 9, 22, -1},
	{g_wolfStarConfigButtons, 9, 10, -1},
};

// GLOBAL: MW2SHELL 0x1006fed0
MenuScreen g_readyRoomScreens[3] = {
	{g_wolfReadyRoomButtons, 20, 14, 0x25},
	{g_jadeFalconReadyRoomButtons, 20, 21, 0x28},
	{NULL, 0, 0, 0},
};

// The debriefing screen of each campaign.
// GLOBAL: MW2SHELL 0x1006ff00
MenuScreen g_debriefScreens[3] = {
	{g_wolfDebriefButtons, 3, 16, -1},
	{g_jadeFalconDebriefButtons, 3, 23, -1},
	{g_wolfDebriefButtons, 3, 10, -1},
};

// The aftermath reader of each campaign.
// GLOBAL: MW2SHELL 0x1006ff30
MenuScreen g_aftermathScreens[3] = {
	{g_wolfAftermathButtons, 4, 16, -1},
	{g_jadeFalconAftermathButtons, 4, 23, -1},
	{g_wolfAftermathButtons, 4, 10, -1},
};

// The briefing screen of each campaign. SKIP (the fourth button) is dropped for every pilot
// but FERRARI.
// GLOBAL: MW2SHELL 0x1006ff60
MenuScreen g_briefingScreens[3] = {
	{g_wolfBriefingButtons, 4, 16, -1},
	{g_jadeFalconBriefingButtons, 4, 23, -1},
	{g_wolfBriefingButtons, 4, 10, -1},
};

// The situation reader of each campaign.
// GLOBAL: MW2SHELL 0x1006ff90
MenuScreen g_situationScreens[3] = {
	{g_wolfSituationButtons, 4, 16, -1},
	{g_jadeFalconSituationButtons, 4, 23, -1},
	{g_wolfSituationButtons, 4, 10, -1},
};

// GLOBAL: MW2SHELL 0x1006ffc0
MenuScreen g_cadetTrainingScreens[3] = {
	{g_wolfCadetTrainingButtons, 7, 13, 38},
	{g_jadeFalconCadetTrainingButtons, 7, 20, 41},
	{NULL, 0, 0, 0},
};

// GLOBAL: MW2SHELL 0x1006fff0
MenuScreen g_mechBayScreens[3] = {
	{g_wolfMechBayButtons, 11, 15, -1},
	{g_jadeFalconMechBayButtons, 11, 22, -1},
	{g_wolfMechBayButtons, 11, 10, -1},
};
