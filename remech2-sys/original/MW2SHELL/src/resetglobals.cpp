#include "resetglobals.h"

#include "archivereader.h"
#include "briefing.h"
#include "cadettraining.h"
#include "clanhall.h"
#include "credits.h"
#include "debrief.h"
#include "debugout.h"
#include "debugprint.h"
#include "hallofhonor.h"
#include "keyboard.h"
#include "keyboardinput.h"
#include "mainmenu.h"
#include "mechbay.h"
#include "mechvariant.h"
#include "menudata.h"
#include "missionui.h"
#include "mousestate.h"
#include "mw2prj.h"
#include "options.h"
#include "page.h"
#include "prjfile.h"
#include "projectarchive.h"
#include "readyroom.h"
#include "refreshmode.h"
#include "resourcecache.h"
#include "resourcefile.h"
#include "rosterscreen.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "simhandoff.h"
#include "textglyph.h"
#include "textpages.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdlib.h>
#include <string.h>

// Some globals are pointers: their own size is what is copied.
#define RESET_GLOBAL(x) {&(x), sizeof(x)} // NOLINT(bugprone-sizeof-expression)

// Every global the shell defines, by source file
static const struct {
	void* m_address;
	size_t m_size;
} g_resetGlobals[] = {
	// archivereader.cpp
	RESET_GLOBAL(g_archiveReader),
	RESET_GLOBAL(g_archiveReturnMessage),
	RESET_GLOBAL(g_archiveSound),
	RESET_GLOBAL(g_archiveText),
	RESET_GLOBAL(g_archiveTitle),
	RESET_GLOBAL(g_archiveTopicName),
	RESET_GLOBAL(g_prevPageLinkButton),
	RESET_GLOBAL(g_nextPageLinkButton),
	RESET_GLOBAL(g_archiveLine),

	// briefing.cpp
	RESET_GLOBAL(g_unk0x10071cd8),
	RESET_GLOBAL(g_briefingMenu),
	RESET_GLOBAL(g_briefingPage),
	RESET_GLOBAL(g_briefingPages),
	RESET_GLOBAL(g_situationReader),

	// cadettraining.cpp
	RESET_GLOBAL(g_trainerTake),
	RESET_GLOBAL(g_trainerIdleCountdown),
	RESET_GLOBAL(g_trainingButtonsShown),
	RESET_GLOBAL(g_trainingExitVideo),
	RESET_GLOBAL(g_trainingAmbience),
	RESET_GLOBAL(g_cadetTrainingMenu),
	RESET_GLOBAL(g_trainingMessage),

	// clanhall.cpp
	RESET_GLOBAL(g_clanHallMenu),
	RESET_GLOBAL(g_clanHallAmbience),
	RESET_GLOBAL(g_welcomeSound),
	RESET_GLOBAL(g_welcomePending),
	RESET_GLOBAL(g_clanHallExitVideo),
	RESET_GLOBAL(g_clanHallExitMessage),

	// credits.cpp
	RESET_GLOBAL(g_creditLines),
	RESET_GLOBAL(g_creditsMovie),
	RESET_GLOBAL(g_creditsMovieName),
	RESET_GLOBAL(g_creditsFirstLine),
	RESET_GLOBAL(g_creditsTitleColors),
	RESET_GLOBAL(g_creditsScrollTop),

	// debrief.cpp
	RESET_GLOBAL(g_debriefMenu),
	RESET_GLOBAL(g_debriefPage),
	RESET_GLOBAL(g_debriefPages),
	RESET_GLOBAL(g_aftermathReader),
	RESET_GLOBAL(g_objectiveStatus),
	RESET_GLOBAL(g_debriefText),
	RESET_GLOBAL(g_sortedObjectives),
	RESET_GLOBAL(g_objectiveLine),
	RESET_GLOBAL(g_careerHonor),
	RESET_GLOBAL(g_pilotBeforeMission),
	RESET_GLOBAL(g_unk0x10077fe0),
	RESET_GLOBAL(g_missionResults),
	RESET_GLOBAL(g_objectiveDescription),
	RESET_GLOBAL(g_honorPoints),
	RESET_GLOBAL(g_objectiveType),
	RESET_GLOBAL(g_skillName),
	RESET_GLOBAL(g_careerHonorLine),
	RESET_GLOBAL(g_honorLine),
	RESET_GLOBAL(g_objectiveTime),

	// debugout.c
	RESET_GLOBAL(g_debugOutputMode),
	RESET_GLOBAL(g_debugLogFile),
	RESET_GLOBAL(g_debugLogName),

	// debugprint.c
	RESET_GLOBAL(g_debugPrintBuffer),

	// hallofhonor.cpp
	RESET_GLOBAL(g_hallOfHonorMovie),
	RESET_GLOBAL(g_hallOfHonorText),

	// keyboard.c
	RESET_GLOBAL(g_keyCodeWriteIndex),
	RESET_GLOBAL(g_keyCodeReadIndex),
	RESET_GLOBAL(g_keyCodes),
	RESET_GLOBAL(g_keyStates),
	RESET_GLOBAL(g_keyCodeMap),
	RESET_GLOBAL(g_extendedScanCodeMap),

	// keyboardinput.cpp
	RESET_GLOBAL(g_editTextBuffer),

	// mainmenu.cpp
	RESET_GLOBAL(g_mainMenu),
	RESET_GLOBAL(g_mainMenuMusic),
	RESET_GLOBAL(g_mainMenuIntro),
	RESET_GLOBAL(g_mainMenuMusicStarted),
	RESET_GLOBAL(g_mainMenuLogoVideo),

	// mechbay.cpp
	RESET_GLOBAL(g_equipment),
	RESET_GLOBAL(g_locationNames),
	RESET_GLOBAL(g_internalStructure),
	RESET_GLOBAL(g_variant),
	RESET_GLOBAL(g_previousVariant),
	RESET_GLOBAL(g_engines),
	RESET_GLOBAL(g_weapons),
	RESET_GLOBAL(g_componentFields),
	RESET_GLOBAL(g_screenFields),
	RESET_GLOBAL(g_armorTrimLocation),
	RESET_GLOBAL(g_locationMapLefts),
	RESET_GLOBAL(g_locationMapTops),
	RESET_GLOBAL(g_locationSound),
	RESET_GLOBAL(g_mechChassis),
	RESET_GLOBAL(g_chassisCount),
	RESET_GLOBAL(g_wolfChassisVideo),
	RESET_GLOBAL(g_jadeFalconChassisVideo),
	RESET_GLOBAL(g_trialChassisVideo),
	RESET_GLOBAL(g_chassisVideoFormat),
	RESET_GLOBAL(g_selectedChassis),
	RESET_GLOBAL(g_chassisNameSound),
	RESET_GLOBAL(g_pickStarMech),
	RESET_GLOBAL(g_mechBayMenu),
	RESET_GLOBAL(g_mechBayAmbience),
	RESET_GLOBAL(g_mechBayWParam),
	RESET_GLOBAL(g_unk0x1006178c),
	RESET_GLOBAL(g_chassisVideoLeft),
	RESET_GLOBAL(g_chassisVideoTop),
	RESET_GLOBAL(g_pickingStarMech),
	RESET_GLOBAL(g_userMekName),
	RESET_GLOBAL(g_acceptSound),
	RESET_GLOBAL(g_callsignLine),
	RESET_GLOBAL(g_tempBuffer),
	RESET_GLOBAL(g_mekAmmo),
	RESET_GLOBAL(g_variantSound),
	RESET_GLOBAL(g_mekHeader),
	RESET_GLOBAL(g_chassisVideoName),
	RESET_GLOBAL(g_mekVariantName),
	RESET_GLOBAL(g_variantFiles),
	RESET_GLOBAL(g_mekWeapons),
	RESET_GLOBAL(g_mechBayMessage),
	RESET_GLOBAL(g_variantFileName),
	RESET_GLOBAL(g_mekFileBuffer),
	RESET_GLOBAL(g_mekLocations),
	RESET_GLOBAL(g_warningColors),
	RESET_GLOBAL(g_activeColors),
	RESET_GLOBAL(g_textColors),
	RESET_GLOBAL(g_mekPath),
	RESET_GLOBAL(g_selectedVariant),
	RESET_GLOBAL(g_engineFields),
	RESET_GLOBAL(g_heatSinkFields),
	RESET_GLOBAL(g_jumpJetFields),
	RESET_GLOBAL(g_internalFields),
	RESET_GLOBAL(g_armorFields),
	RESET_GLOBAL(g_equipmentFields),
	RESET_GLOBAL(g_weaponFields),
	RESET_GLOBAL(g_criticalFields),
	RESET_GLOBAL(g_customizeFields),
	RESET_GLOBAL(g_mechBayFields),

	// mechvariant.cpp
	RESET_GLOBAL(g_formationLayouts),
	RESET_GLOBAL(g_jadeFalconFormationLayouts),
	RESET_GLOBAL(g_wolfStarVideo),
	RESET_GLOBAL(g_jadeFalconStarVideo),
	RESET_GLOBAL(g_trialStarVideo),
	RESET_GLOBAL(g_playerStar),
	RESET_GLOBAL(g_enemyStar),
	RESET_GLOBAL(g_selectedStar),
	RESET_GLOBAL(g_formationOptions),
	RESET_GLOBAL(g_starLabelLefts),
	RESET_GLOBAL(g_starLabelTops),
	RESET_GLOBAL(g_jadeFalconLabelLefts),
	RESET_GLOBAL(g_jadeFalconLabelTops),
	RESET_GLOBAL(g_formationLine),
	RESET_GLOBAL(g_missionLine),
	RESET_GLOBAL(g_starSizeLine),
	RESET_GLOBAL(g_tonnageLine),
	RESET_GLOBAL(g_starMassLine),
	RESET_GLOBAL(g_positionGlyphs),
	RESET_GLOBAL(g_mechMasses),
	RESET_GLOBAL(g_formationLayout),
	RESET_GLOBAL(g_mechLabSound),
	RESET_GLOBAL(g_starInfoText),
	RESET_GLOBAL(g_labelTops),
	RESET_GLOBAL(g_labelLefts),
	RESET_GLOBAL(g_starSound),
	RESET_GLOBAL(g_starMenu),
	RESET_GLOBAL(g_fitTextBuffer),
	RESET_GLOBAL(g_starVideoFormat),

	// menudata.cpp
	RESET_GLOBAL(g_wolfClanHallCadetTrainingLabel),
	RESET_GLOBAL(g_wolfClanHallArchiveLabel),
	RESET_GLOBAL(g_wolfClanHallReadyRoomLabel),
	RESET_GLOBAL(g_wolfClanHallRegisterLabel),
	RESET_GLOBAL(g_wolfClanHallExitLabel),
	RESET_GLOBAL(g_wolfRosterNewAllegianceLabel),
	RESET_GLOBAL(g_wolfRosterSlot0Label),
	RESET_GLOBAL(g_wolfRosterSlot1Label),
	RESET_GLOBAL(g_wolfRosterSlot2Label),
	RESET_GLOBAL(g_wolfRosterSlot3Label),
	RESET_GLOBAL(g_wolfRosterSlot4Label),
	RESET_GLOBAL(g_wolfRosterSlot5Label),
	RESET_GLOBAL(g_wolfRosterSlot6Label),
	RESET_GLOBAL(g_wolfRosterSlot7Label),
	RESET_GLOBAL(g_wolfRosterSlot8Label),
	RESET_GLOBAL(g_wolfRosterSlot9Label),
	RESET_GLOBAL(g_wolfRosterAcceptLabel),
	RESET_GLOBAL(g_wolfRosterDeleteMechwarriorLabel),
	RESET_GLOBAL(g_wolfRosterLaunchOldMissionLabel),
	RESET_GLOBAL(g_wolfRosterPilotInfoLabel),
	RESET_GLOBAL(g_wolfArchiveExitLabel),
	RESET_GLOBAL(g_wolfArchivePrevPageLabel),
	RESET_GLOBAL(g_wolfArchiveNextPageLabel),
	RESET_GLOBAL(g_wolfArchiveBackLabel),
	RESET_GLOBAL(g_wolfCadetTrainingClanHallLabel),
	RESET_GLOBAL(g_wolfCadetTrainingNavComputerLabel),
	RESET_GLOBAL(g_wolfCadetTrainingMechHandlingLabel),
	RESET_GLOBAL(g_wolfCadetTrainingWeaponsUsageLabel),
	RESET_GLOBAL(g_wolfCadetTrainingHuntingLabel),
	RESET_GLOBAL(g_wolfCadetTrainingInspectionLabel),
	RESET_GLOBAL(g_wolfCadetTrainingTrialLabel),
	RESET_GLOBAL(g_wolfReadyRoomClanHallLabel),
	RESET_GLOBAL(g_wolfReadyRoomMechLabLabel),
	RESET_GLOBAL(g_wolfReadyRoomStarConfigLabel),
	RESET_GLOBAL(g_wolfReadyRoomMissionBriefingLabel),
	RESET_GLOBAL(g_wolfReadyRoomYellowLabel),
	RESET_GLOBAL(g_wolfReadyRoomOrangeLabel),
	RESET_GLOBAL(g_wolfReadyRoomTealLabel),
	RESET_GLOBAL(g_wolfReadyRoomTaupeLabel),
	RESET_GLOBAL(g_wolfReadyRoomJennyLabel),
	RESET_GLOBAL(g_wolfReadyRoomSableLabel),
	RESET_GLOBAL(g_wolfReadyRoomGreyLabel),
	RESET_GLOBAL(g_wolfReadyRoomBrownLabel),
	RESET_GLOBAL(g_wolfReadyRoomAmyLabel),
	RESET_GLOBAL(g_wolfReadyRoomSilverLabel),
	RESET_GLOBAL(g_wolfReadyRoomAquaLabel),
	RESET_GLOBAL(g_wolfReadyRoomKimLabel),
	RESET_GLOBAL(g_wolfReadyRoomCyanLabel),
	RESET_GLOBAL(g_wolfReadyRoomMaroonLabel),
	RESET_GLOBAL(g_wolfReadyRoomGoldLabel),
	RESET_GLOBAL(g_wolfReadyRoomIreneLabel),
	RESET_GLOBAL(g_wolfBriefingAbortLabel),
	RESET_GLOBAL(g_wolfBriefingSituationLabel),
	RESET_GLOBAL(g_wolfBriefingLaunchLabel),
	RESET_GLOBAL(g_wolfBriefingSkipLabel),
	RESET_GLOBAL(g_wolfSituationExitLabel),
	RESET_GLOBAL(g_wolfSituationPrevPageLabel),
	RESET_GLOBAL(g_wolfSituationNextPageLabel),
	RESET_GLOBAL(g_wolfSituationBackLabel),
	RESET_GLOBAL(g_wolfDebriefExitLabel),
	RESET_GLOBAL(g_wolfDebriefAftermathLabel),
	RESET_GLOBAL(g_wolfDebriefReplayLabel),
	RESET_GLOBAL(g_wolfAftermathExitLabel),
	RESET_GLOBAL(g_wolfAftermathPrevPageLabel),
	RESET_GLOBAL(g_wolfAftermathNextPageLabel),
	RESET_GLOBAL(g_wolfAftermathBackLabel),
	RESET_GLOBAL(g_wolfStarConfigExitConfigLabel),
	RESET_GLOBAL(g_wolfStarConfigMechLabLabel),
	RESET_GLOBAL(g_wolfStarConfigNextFormationLabel),
	RESET_GLOBAL(g_wolfStarConfigPrevFormationLabel),
	RESET_GLOBAL(g_wolfStarConfigAddStarmateLabel),
	RESET_GLOBAL(g_wolfStarConfigDeleteStarmateLabel),
	RESET_GLOBAL(g_wolfStarConfigChangeMech0Label),
	RESET_GLOBAL(g_wolfStarConfigChangeMech1Label),
	RESET_GLOBAL(g_wolfStarConfigChangeMech2Label),
	RESET_GLOBAL(g_wolfMechBayExitLabLabel),
	RESET_GLOBAL(g_wolfMechBayStarConfigLabel),
	RESET_GLOBAL(g_wolfMechBayNextChassisLabel),
	RESET_GLOBAL(g_wolfMechBayPrevChassisLabel),
	RESET_GLOBAL(g_wolfMechBayNextVariantLabel),
	RESET_GLOBAL(g_wolfMechBayPrevVariantLabel),
	RESET_GLOBAL(g_wolfMechBayCustomizeLabel),
	RESET_GLOBAL(g_wolfMechBayAcceptMechLabel),
	RESET_GLOBAL(g_wolfMechBaySaveLabel),
	RESET_GLOBAL(g_wolfMechBayAbortLabel),
	RESET_GLOBAL(g_wolfMechBayDeleteLabel),
	RESET_GLOBAL(g_jadeFalconClanHallCadetTrainingLabel),
	RESET_GLOBAL(g_jadeFalconClanHallArchiveLabel),
	RESET_GLOBAL(g_jadeFalconClanHallReadyRoomLabel),
	RESET_GLOBAL(g_jadeFalconClanHallRegisterLabel),
	RESET_GLOBAL(g_jadeFalconClanHallExitLabel),
	RESET_GLOBAL(g_jadeFalconRosterNewAllegianceLabel),
	RESET_GLOBAL(g_jadeFalconRosterSlot0Label),
	RESET_GLOBAL(g_jadeFalconRosterSlot1Label),
	RESET_GLOBAL(g_jadeFalconRosterSlot2Label),
	RESET_GLOBAL(g_jadeFalconRosterSlot3Label),
	RESET_GLOBAL(g_jadeFalconRosterSlot4Label),
	RESET_GLOBAL(g_jadeFalconRosterSlot5Label),
	RESET_GLOBAL(g_jadeFalconRosterSlot6Label),
	RESET_GLOBAL(g_jadeFalconRosterSlot7Label),
	RESET_GLOBAL(g_jadeFalconRosterSlot8Label),
	RESET_GLOBAL(g_jadeFalconRosterSlot9Label),
	RESET_GLOBAL(g_jadeFalconRosterAcceptLabel),
	RESET_GLOBAL(g_jadeFalconRosterDeleteMechwarriorLabel),
	RESET_GLOBAL(g_jadeFalconRosterLaunchOldMissionLabel),
	RESET_GLOBAL(g_jadeFalconRosterPilotInfoLabel),
	RESET_GLOBAL(g_jadeFalconArchiveExitLabel),
	RESET_GLOBAL(g_jadeFalconArchivePrevPageLabel),
	RESET_GLOBAL(g_jadeFalconArchiveNextPageLabel),
	RESET_GLOBAL(g_jadeFalconArchiveBackLabel),
	RESET_GLOBAL(g_jadeFalconCadetTrainingClanHallLabel),
	RESET_GLOBAL(g_jadeFalconCadetTrainingNavComputerLabel),
	RESET_GLOBAL(g_jadeFalconCadetTrainingMechHandlingLabel),
	RESET_GLOBAL(g_jadeFalconCadetTrainingWeaponsUsageLabel),
	RESET_GLOBAL(g_jadeFalconCadetTrainingHuntingLabel),
	RESET_GLOBAL(g_jadeFalconCadetTrainingInspectionLabel),
	RESET_GLOBAL(g_jadeFalconCadetTrainingTrialLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomClanHallLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomMechLabLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomStarConfigLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomMissionBriefingLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomPinkLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomGreenLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomRedLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomFuchsiaLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomCindyLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomRustLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomUmberLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomTanLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomHeidiLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomPlumLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomWhiteLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomJillLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomPuceLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomBlondeLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomBronzeLabel),
	RESET_GLOBAL(g_jadeFalconReadyRoomMaryLabel),
	RESET_GLOBAL(g_jadeFalconBriefingAbortLabel),
	RESET_GLOBAL(g_jadeFalconBriefingSituationLabel),
	RESET_GLOBAL(g_jadeFalconBriefingLaunchLabel),
	RESET_GLOBAL(g_jadeFalconBriefingSkipLabel),
	RESET_GLOBAL(g_jadeFalconSituationExitLabel),
	RESET_GLOBAL(g_jadeFalconSituationPrevPageLabel),
	RESET_GLOBAL(g_jadeFalconSituationNextPageLabel),
	RESET_GLOBAL(g_jadeFalconSituationBackLabel),
	RESET_GLOBAL(g_jadeFalconDebriefExitLabel),
	RESET_GLOBAL(g_jadeFalconDebriefAftermathLabel),
	RESET_GLOBAL(g_jadeFalconDebriefReplayLabel),
	RESET_GLOBAL(g_jadeFalconAftermathExitLabel),
	RESET_GLOBAL(g_jadeFalconAftermathPrevPageLabel),
	RESET_GLOBAL(g_jadeFalconAftermathNextPageLabel),
	RESET_GLOBAL(g_jadeFalconAftermathBackLabel),
	RESET_GLOBAL(g_jadeFalconStarConfigExitConfigLabel),
	RESET_GLOBAL(g_jadeFalconStarConfigMechLabLabel),
	RESET_GLOBAL(g_jadeFalconStarConfigNextFormationLabel),
	RESET_GLOBAL(g_jadeFalconStarConfigPrevFormationLabel),
	RESET_GLOBAL(g_jadeFalconStarConfigAddStarmateLabel),
	RESET_GLOBAL(g_jadeFalconStarConfigDeleteStarmateLabel),
	RESET_GLOBAL(g_jadeFalconStarConfigChangeMech0Label),
	RESET_GLOBAL(g_jadeFalconStarConfigChangeMech1Label),
	RESET_GLOBAL(g_jadeFalconStarConfigChangeMech2Label),
	RESET_GLOBAL(g_jadeFalconMechBayExitLabLabel),
	RESET_GLOBAL(g_jadeFalconMechBayStarConfigLabel),
	RESET_GLOBAL(g_jadeFalconMechBayNextChassisLabel),
	RESET_GLOBAL(g_jadeFalconMechBayPrevChassisLabel),
	RESET_GLOBAL(g_jadeFalconMechBayNextVariantLabel),
	RESET_GLOBAL(g_jadeFalconMechBayPrevVariantLabel),
	RESET_GLOBAL(g_jadeFalconMechBayCustomizeLabel),
	RESET_GLOBAL(g_jadeFalconMechBayAcceptMechLabel),
	RESET_GLOBAL(g_jadeFalconMechBaySaveLabel),
	RESET_GLOBAL(g_jadeFalconMechBayAbortLabel),
	RESET_GLOBAL(g_jadeFalconMechBayDeleteLabel),
	RESET_GLOBAL(g_mainMenuTrialsOfGrievanceLabel),
	RESET_GLOBAL(g_mainMenuWolfClanHallLabel),
	RESET_GLOBAL(g_mainMenuJadeFalconClanHallLabel),
	RESET_GLOBAL(g_mainMenuExitLabel),
	RESET_GLOBAL(g_missionBriefingLaunchLabel),
	RESET_GLOBAL(g_missionBriefingExitLabel),
	RESET_GLOBAL(g_missionBriefingMissionLabel),
	RESET_GLOBAL(g_missionBriefingPlayerMech0NextLabel),
	RESET_GLOBAL(g_missionBriefingPlayerMech1NextLabel),
	RESET_GLOBAL(g_missionBriefingPlayerMech2NextLabel),
	RESET_GLOBAL(g_missionBriefingPlayerFormationNextLabel),
	RESET_GLOBAL(g_missionBriefingPlayerMech0PrevLabel),
	RESET_GLOBAL(g_missionBriefingPlayerMech1PrevLabel),
	RESET_GLOBAL(g_missionBriefingPlayerMech2PrevLabel),
	RESET_GLOBAL(g_missionBriefingPlayerFormationPrevLabel),
	RESET_GLOBAL(g_missionBriefingPlayerClanLabel),
	RESET_GLOBAL(g_missionBriefingPlayerMechLabLabel),
	RESET_GLOBAL(g_missionBriefingPlayerStarConfigLabel),
	RESET_GLOBAL(g_missionBriefingEnemyMech0NextLabel),
	RESET_GLOBAL(g_missionBriefingEnemyMech1NextLabel),
	RESET_GLOBAL(g_missionBriefingEnemyMech2NextLabel),
	RESET_GLOBAL(g_missionBriefingEnemyFormationNextLabel),
	RESET_GLOBAL(g_missionBriefingEnemyMech0PrevLabel),
	RESET_GLOBAL(g_missionBriefingEnemyMech1PrevLabel),
	RESET_GLOBAL(g_missionBriefingEnemyMech2PrevLabel),
	RESET_GLOBAL(g_missionBriefingEnemyFormationPrevLabel),
	RESET_GLOBAL(g_missionBriefingEnemyClanLabel),
	RESET_GLOBAL(g_missionBriefingEnemyMechLabLabel),
	RESET_GLOBAL(g_missionBriefingEnemyStarConfigLabel),
	RESET_GLOBAL(g_textTabStops),
	RESET_GLOBAL(g_databaseName),
	RESET_GLOBAL(g_archiveNames),
	RESET_GLOBAL(g_formations),
	RESET_GLOBAL(g_echelonLeftPositions),
	RESET_GLOBAL(g_echelonRightPositions),
	RESET_GLOBAL(g_lineAbreastPositions),
	RESET_GLOBAL(g_lineAsternPositions),
	RESET_GLOBAL(g_vFormPositions),
	RESET_GLOBAL(g_wedgePositions),
	RESET_GLOBAL(g_wolfClanHallButtons),
	RESET_GLOBAL(g_wolfRosterButtons),
	RESET_GLOBAL(g_wolfArchiveButtons),
	RESET_GLOBAL(g_wolfCadetTrainingButtons),
	RESET_GLOBAL(g_wolfReadyRoomButtons),
	RESET_GLOBAL(g_wolfBriefingButtons),
	RESET_GLOBAL(g_wolfSituationButtons),
	RESET_GLOBAL(g_wolfDebriefButtons),
	RESET_GLOBAL(g_wolfAftermathButtons),
	RESET_GLOBAL(g_wolfStarConfigButtons),
	RESET_GLOBAL(g_wolfMechBayButtons),
	RESET_GLOBAL(g_jadeFalconClanHallButtons),
	RESET_GLOBAL(g_jadeFalconRosterButtons),
	RESET_GLOBAL(g_jadeFalconArchiveButtons),
	RESET_GLOBAL(g_jadeFalconCadetTrainingButtons),
	RESET_GLOBAL(g_jadeFalconReadyRoomButtons),
	RESET_GLOBAL(g_jadeFalconBriefingButtons),
	RESET_GLOBAL(g_jadeFalconSituationButtons),
	RESET_GLOBAL(g_jadeFalconDebriefButtons),
	RESET_GLOBAL(g_jadeFalconAftermathButtons),
	RESET_GLOBAL(g_jadeFalconStarConfigButtons),
	RESET_GLOBAL(g_jadeFalconMechBayButtons),
	RESET_GLOBAL(g_mainMenuButtons),
	RESET_GLOBAL(g_missionBriefingButtons),
	RESET_GLOBAL(g_wolfMissions),
	RESET_GLOBAL(g_jadeFalconMissions),
	RESET_GLOBAL(g_campaignMissions),
	RESET_GLOBAL(g_wolfTrainingScenarios),
	RESET_GLOBAL(g_jadeFalconTrainingScenarios),
	RESET_GLOBAL(g_trainingScenarios),
	RESET_GLOBAL(g_clanHallScreens),
	RESET_GLOBAL(g_rosterScreens),
	RESET_GLOBAL(g_archiveScreens),
	RESET_GLOBAL(g_starConfigScreens),
	RESET_GLOBAL(g_readyRoomScreens),
	RESET_GLOBAL(g_debriefScreens),
	RESET_GLOBAL(g_aftermathScreens),
	RESET_GLOBAL(g_briefingScreens),
	RESET_GLOBAL(g_situationScreens),
	RESET_GLOBAL(g_cadetTrainingScreens),
	RESET_GLOBAL(g_mechBayScreens),

	// missionui.cpp
	RESET_GLOBAL(g_briefingVideos),
	RESET_GLOBAL(g_briefingScenarios),
	RESET_GLOBAL(g_clanVideos),
	RESET_GLOBAL(g_playerMechTags),
	RESET_GLOBAL(g_enemyMechTags),
	RESET_GLOBAL(g_briefingChassisCount),
	RESET_GLOBAL(g_briefingVideo),
	RESET_GLOBAL(g_enemyFormationName),
	RESET_GLOBAL(g_playerFormationName),
	RESET_GLOBAL(g_missionBriefingMenu),
	RESET_GLOBAL(g_launchSound),
	RESET_GLOBAL(g_trialSound),
	RESET_GLOBAL(g_briefingTextColors),
	RESET_GLOBAL(g_briefingClan),
	RESET_GLOBAL(g_briefingLines),
	RESET_GLOBAL(g_playerFormation),
	RESET_GLOBAL(g_briefingRival),
	RESET_GLOBAL(g_briefingMessage),
	RESET_GLOBAL(g_enemyFormation),
	RESET_GLOBAL(g_briefingLine),
	RESET_GLOBAL(g_briefingMission),

	// mousestate.cpp
	RESET_GLOBAL(g_cursorPositionText),

	// mw2prj.c
	RESET_GLOBAL(g_resourceTypeTags),
	RESET_GLOBAL(g_resourceTypeExtensions),
	RESET_GLOBAL(g_mw2PrjHandle),
	RESET_GLOBAL(g_mw2PrjPath),

	// options.cpp
	RESET_GLOBAL(g_optionsMovie),
	RESET_GLOBAL(g_optionsMovieName),
	RESET_GLOBAL(g_volumeTestSample),
	RESET_GLOBAL(g_savedPalette),
	RESET_GLOBAL(g_sliderImages),
	RESET_GLOBAL(g_skillNames),
	RESET_GLOBAL(g_optionFields),

	// page.cpp
	RESET_GLOBAL(g_pageLinkRead),
	RESET_GLOBAL(g_pageBackUpPending),
	RESET_GLOBAL(g_pageGoToPending),
	RESET_GLOBAL(g_pageWord),
	RESET_GLOBAL(g_pageLine),
	RESET_GLOBAL(g_pageTemp),

	// prjfile.c
	RESET_GLOBAL(g_archiveSlots),
	RESET_GLOBAL(g_archiveAlloc),
	RESET_GLOBAL(g_archiveFree),

	// projectarchive.cpp
	RESET_GLOBAL(g_bwdTags),
	RESET_GLOBAL(g_bwdRegistrySize),
	RESET_GLOBAL(g_bwdLargestTemplate),
	RESET_GLOBAL(g_bwdDifficultySettings),
	RESET_GLOBAL(g_enemyStarDifficulty),
	RESET_GLOBAL(g_clanBitmapNames),
	RESET_GLOBAL(g_bwdTemplateRegistry),

	// readyroom.cpp
	RESET_GLOBAL(g_readyRoomExitVideo),
	RESET_GLOBAL(g_readyRoomExitMessage),
	RESET_GLOBAL(g_readyRoomSound),
	RESET_GLOBAL(g_readyRoomMenu),
	RESET_GLOBAL(g_readyRoomMessage),

	// refreshmode.c
	RESET_GLOBAL(g_displayBackend),
	RESET_GLOBAL(g_refreshMode),
	RESET_GLOBAL(g_currentDisplayBackend),
	RESET_GLOBAL(g_currentRefreshMode),
	RESET_GLOBAL(g_refreshModeBuffer),
	RESET_GLOBAL(g_paletteColors),
	RESET_GLOBAL(g_frame),
	RESET_GLOBAL(g_windowMode),
	RESET_GLOBAL(g_refreshModePixelCount),
	RESET_GLOBAL(g_refreshModeWidth),
	RESET_GLOBAL(g_refreshModeHeight),

	// resourcecache.c
	RESET_GLOBAL(g_cacheTable),
	RESET_GLOBAL(g_cacheDumpNumber),
	RESET_GLOBAL(g_cacheEntryCount),
	RESET_GLOBAL(g_purgeListHead),
	RESET_GLOBAL(g_purgeListTail),

	// resourcefile.c
	RESET_GLOBAL(g_resourceDir),
	RESET_GLOBAL(g_resourcePath),

	// rosterscreen.cpp
	RESET_GLOBAL(g_rosterCampaign),
	RESET_GLOBAL(g_rosterFieldText),
	RESET_GLOBAL(g_rosterPilots),
	RESET_GLOBAL(g_pilotRecordFields),
	RESET_GLOBAL(g_missionListFields),
	RESET_GLOBAL(g_rosterSound),
	RESET_GLOBAL(g_missionListShown),
	RESET_GLOBAL(g_rosterMenu),

	// shellglobals.cpp
	RESET_GLOBAL(g_keyboardInput),
	RESET_GLOBAL(g_audioSubsystem),
	RESET_GLOBAL(g_cursorShape),
	RESET_GLOBAL(g_mouseState),
	RESET_GLOBAL(g_videoDriver),
	RESET_GLOBAL(g_defaultFont),
	RESET_GLOBAL(g_textFont),
	RESET_GLOBAL(g_titleFont),
	RESET_GLOBAL(g_buttonFont),
	RESET_GLOBAL(g_unk0x1007121c),
	RESET_GLOBAL(g_unk0x10071220),
	RESET_GLOBAL(g_archiveFont),
	RESET_GLOBAL(g_bodyFont),
	RESET_GLOBAL(g_mw2Database),
	RESET_GLOBAL(g_projectArchive),
	RESET_GLOBAL(g_midiAudio),
	RESET_GLOBAL(g_digitalAudio),
	RESET_GLOBAL(g_unk0x1007123c),
	RESET_GLOBAL(g_runSim),
	RESET_GLOBAL(g_movieOpenFlags),
	RESET_GLOBAL(g_drawFmv),
	RESET_GLOBAL(g_rankNames),
	RESET_GLOBAL(g_clanNames),
	RESET_GLOBAL(g_trialsSongs),
	RESET_GLOBAL(g_wolfSongs),
	RESET_GLOBAL(g_jadeFalconSongs),
	RESET_GLOBAL(g_currentPilot),
	RESET_GLOBAL(g_newPilotRegistered),
	RESET_GLOBAL(g_savedScreenPalette),
	RESET_GLOBAL(g_soundConfig),
	RESET_GLOBAL(g_difficultyConfig),
	RESET_GLOBAL(g_pilotRoster),

	// shellmain.cpp
	RESET_GLOBAL(g_screenFunction),
	RESET_GLOBAL(g_menuFunction),
	RESET_GLOBAL(g_midiBackgroundMusic),
	RESET_GLOBAL(g_cursorHidden),
	RESET_GLOBAL(g_midiSongBaseSet),
	RESET_GLOBAL(g_midiSongBase),
	RESET_GLOBAL(g_midiSongPlaying),
	RESET_GLOBAL(g_scenario),
	RESET_GLOBAL(g_selectedCampaign),
	RESET_GLOBAL(g_pilotChosen),
	RESET_GLOBAL(g_menuCommandEnabled),

	// simhandoff.cpp
	RESET_GLOBAL(g_missionName),
	RESET_GLOBAL(g_simHandoff),

	// textglyph.cpp
	RESET_GLOBAL(g_linkColorMap),
	RESET_GLOBAL(g_unk0x10074758),
	RESET_GLOBAL(g_unk0x10074858),

	// textpages.cpp
	RESET_GLOBAL(g_textPageColors),
	RESET_GLOBAL(g_expandedText),

	// tmpackdatabase.cpp
	RESET_GLOBAL(g_lzWindow),

	// video.cpp
	RESET_GLOBAL(g_fmvSlots),
	RESET_GLOBAL(g_fullscreenVideoMsg),
	RESET_GLOBAL(g_fullscreenVideoWParam),
	RESET_GLOBAL(g_videoPath),
	RESET_GLOBAL(g_shpPath),

	// videodriver.cpp
	RESET_GLOBAL(g_redrawingGlyphs),
	RESET_GLOBAL(g_clearPaletteOnDraw),
	RESET_GLOBAL(g_tempPalette),
	RESET_GLOBAL(g_defaultColorMap),

	// windowstate.c
	RESET_GLOBAL(g_windowClassName),
	RESET_GLOBAL(g_windowActive),
	RESET_GLOBAL(g_paused),
	RESET_GLOBAL(g_menuDialogOpen),
	RESET_GLOBAL(g_primaryHeap),
	RESET_GLOBAL(g_windowHeight),
	RESET_GLOBAL(g_windowWidth),
};

// What the globals held before the first shell run
static MechU8* g_resetInitial = NULL;

void ResetShellGlobals()
{
	size_t i;
	size_t size;
	MechU8* initial;

	if (g_resetInitial == NULL) {
		size = 0;
		for (i = 0; i < sizeof(g_resetGlobals) / sizeof(g_resetGlobals[0]); i++) {
			size += g_resetGlobals[i].m_size;
		}

		// Not from g_primaryHeap, which each run creates and destroys.
		g_resetInitial = (MechU8*) malloc(size);
		initial = g_resetInitial;
		for (i = 0; i < sizeof(g_resetGlobals) / sizeof(g_resetGlobals[0]); i++) {
			memcpy(initial, g_resetGlobals[i].m_address, g_resetGlobals[i].m_size);
			initial += g_resetGlobals[i].m_size;
		}
		return;
	}

	initial = g_resetInitial;
	for (i = 0; i < sizeof(g_resetGlobals) / sizeof(g_resetGlobals[0]); i++) {
		memcpy(g_resetGlobals[i].m_address, initial, g_resetGlobals[i].m_size);
		initial += g_resetGlobals[i].m_size;
	}
}
