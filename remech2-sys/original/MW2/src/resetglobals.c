#include "resetglobals.h"

#include "ai.h"
#include "anim2d.h"
#include "animation.h"
#include "audio.h"
#include "audiomenu.h"
#include "bandpoly.h"
#include "bargauges.h"
#include "brightness.h"
#include "bwd.h"
#include "bwdkeywords.h"
#include "bwdnames.h"
#include "callbacks.h"
#include "camerashake.h"
#include "classtable.h"
#include "clock.h"
#include "cockpit.h"
#include "collision.h"
#include "commandmenu.h"
#include "commandpointmenu.h"
#include "config.h"
#include "damagepanel.h"
#include "debris.h"
#include "debugprint.h" // IWYU pragma: keep (sim_names.h renames its globals)
#include "depthsort.h"
#include "dorcs.h"
#include "environment.h"
#include "error.h"
#include "eyepoint.h"
#include "faceshade.h"
#include "gamekeys.h"
#include "geocache.h"
#include "gifsave.h"
#include "gpanim.h"
#include "gridobject.h"
#include "hud.h"
#include "inputmap.h"
#include "keyboard.h"
#include "lancemenu.h"
#include "loadres.h" // IWYU pragma: keep (sim_names.h renames its globals)
#include "logwindow.h" // IWYU pragma: keep (sim_names.h renames its globals)
#include "mainmenu.h"
#include "maneuvers.h"
#include "mapview.h"
#include "mechclass.h"
#include "mechdamage.h"
#include "mechreload.h"
#include "mechviewpanel.h"
#include "mekfile.h"
#include "menu.h"
#include "missionaudio.h"
#include "mouse.h" // IWYU pragma: keep (sim_names.h renames its globals)
#include "mw2log.h"
#include "mw2prj.h" // IWYU pragma: keep (sim_names.h renames its globals)
#include "netio.h"
#include "network.h"
#include "object.h"
#include "objectanim.h"
#include "objective.h"
#include "overlay.h"
#include "palette.h"
#include "pausebanner.h"
#include "perf.h"
#include "players.h"
#include "playertype.h"
#include "polydraw.h"
#include "poolsizes.h"
#include "prjfile.h"
#include "quadtree.h"
#include "random.h"
#include "recordstacks.h"
#include "refreshmode.h" // IWYU pragma: keep (sim_names.h renames its globals)
#include "render.h"
#include "resource.h"
#include "screenscale.h"
#include "screenshot.h"
#include "setres.h"
#include "settings.h"
#include "shape.h"
#include "shapecollision.h"
#include "shapelists.h"
#include "shots.h"
#include "simmain.h"
#include "sndunpack.h"
#include "soundfx.h"
#include "soundlimits.h"
#include "speech.h"
#include "staticmem.h"
#include "statuspanels.h"
#include "supanim.h"
#include "targeting.h"
#include "targetpanel.h"
#include "team.h"
#include "timedoverlays.h"
#include "types.h"
#include "videodriverchoice.h" // IWYU pragma: keep (completes g_videoDriverChoice for sizeof)
#include "view.h"
#include "weapondata.h"
#include "weapons.h"
#include "world.h"
#include "wtbshapes.h"

#include <stdlib.h>
#include <string.h>

// Some globals are pointers: their own size is what is copied.
#define RESET_GLOBAL(x) {&(x), sizeof(x)} // NOLINT(bugprone-sizeof-expression)

// Every global the sim defines, by source file
static const struct {
	void* m_address;
	size_t m_size;
} g_resetGlobals[] = {
	// ai.c
	RESET_GLOBAL(g_debugStar),
	RESET_GLOBAL(g_debugObjective),
	RESET_GLOBAL(g_debugLastLine),
	RESET_GLOBAL(g_debugFirstLine),
	RESET_GLOBAL(g_debugListedStar),
	RESET_GLOBAL(g_aiMessageNames),
	RESET_GLOBAL(g_aiTransitionNames),
	RESET_GLOBAL(g_aiStateNames),
	RESET_GLOBAL(g_aiStateShortNames),
	RESET_GLOBAL(g_aiSymbolicTargetNames),
	RESET_GLOBAL(g_aiTargetTypeNames),
	RESET_GLOBAL(g_aiTargetTypeLetters),
	RESET_GLOBAL(g_shapeKindNames),
	RESET_GLOBAL(g_playerTypeNames),
	RESET_GLOBAL(g_powerStateNames),
	RESET_GLOBAL(g_objectiveTypeNames),
	RESET_GLOBAL(g_aiBehaviorNames),
	RESET_GLOBAL(g_invalidTargetLogCount),
	RESET_GLOBAL(g_mechScripts),
	RESET_GLOBAL(g_artilleryScripts),
	RESET_GLOBAL(g_wandererScripts),
	RESET_GLOBAL(g_truckScripts),
	RESET_GLOBAL(g_tankScripts),
	RESET_GLOBAL(g_helicopterScripts),
	RESET_GLOBAL(g_lairdoCheat),
	RESET_GLOBAL(g_aiSpreadTargets),
	RESET_GLOBAL(g_localStarAssigned),
	RESET_GLOBAL(g_aiStateFns),
	RESET_GLOBAL(g_aiMessageFns),
	RESET_GLOBAL(g_aiTransitionFns),
	RESET_GLOBAL(g_aiScripts),
	RESET_GLOBAL(g_hiddenTargetCount),
	RESET_GLOBAL(g_aiRules),
	RESET_GLOBAL(g_aiStateTime),

	// anim2d.c
	RESET_GLOBAL(g_framePrjCount),
	RESET_GLOBAL(g_anim2dCount),
	RESET_GLOBAL(g_anim2ds),
	RESET_GLOBAL(g_framePrjs),

	// animation.c
	RESET_GLOBAL(g_animInitialized),
	RESET_GLOBAL(g_lumaResourceId),
	RESET_GLOBAL(g_currentAnimSet),
	RESET_GLOBAL(g_animSetUsed),
	RESET_GLOBAL(g_animSetIsSequence),
	RESET_GLOBAL(g_lumaTables),
	RESET_GLOBAL(g_preloadCels),
	RESET_GLOBAL(g_animFrames),
	RESET_GLOBAL(g_animations),
	RESET_GLOBAL(g_animFrameBuffer),

	// audio.c
	RESET_GLOBAL(g_cdTrack),
	RESET_GLOBAL(g_soundConfig),
	RESET_GLOBAL(g_mw2SndCfgData),
	RESET_GLOBAL(g_audioPaused),
	RESET_GLOBAL(g_musicStarted),
	RESET_GLOBAL(g_nextEngageCheck),

	// audiomenu.c
	RESET_GLOBAL(g_audioItem),
	RESET_GLOBAL(g_audioTitle),
	RESET_GLOBAL(g_bettyMessageItem),
	RESET_GLOBAL(g_soundEffectsItem),
	RESET_GLOBAL(g_voiceItem),
	RESET_GLOBAL(g_musicItem),
	RESET_GLOBAL(g_soundEffectsControl),
	RESET_GLOBAL(g_voiceControl),
	RESET_GLOBAL(g_musicControl),
	RESET_GLOBAL(g_audioPage),

	// bandpoly.c
	RESET_GLOBAL(g_bandTexCoords),

	// bargauges.c
	RESET_GLOBAL(g_heatBarLevel),
	RESET_GLOBAL(g_unk0x100a82fc),
	RESET_GLOBAL(g_throttleBarLevel),
	RESET_GLOBAL(g_unk0x100a830c),
	RESET_GLOBAL(g_heatRateBarLevel),
	RESET_GLOBAL(g_unk0x100a831c),
	RESET_GLOBAL(g_jumpFuelBarLevel),
	RESET_GLOBAL(g_unk0x100a832c),
	RESET_GLOBAL(g_throttleGaugeSize),
	RESET_GLOBAL(g_throttleFrameLeft),
	RESET_GLOBAL(g_throttleFrameTop),
	RESET_GLOBAL(g_throttleFrameRight),
	RESET_GLOBAL(g_throttleFrameBottom),
	RESET_GLOBAL(g_throttleBarLeft),
	RESET_GLOBAL(g_throttleZeroY),
	RESET_GLOBAL(g_heatBarPosition),
	RESET_GLOBAL(g_heatBarSize),
	RESET_GLOBAL(g_heatRateBarPosition),
	RESET_GLOBAL(g_heatRateBarSize),
	RESET_GLOBAL(g_jumpFuelBarPosition),
	RESET_GLOBAL(g_jumpFuelBarSize),

	// brightness.c
	RESET_GLOBAL(g_displayBrightness),
	RESET_GLOBAL(g_brightnessSetting),
	RESET_GLOBAL(g_paletteColorsPreBrightness),
	RESET_GLOBAL(g_gammaTable),

	// bwd.c
	RESET_GLOBAL(g_streamsFromFiles),
	RESET_GLOBAL(g_logStreams),
	RESET_GLOBAL(g_unk0x10109c40),

	// bwdkeywords.c
	RESET_GLOBAL(g_bwdVersion),
	RESET_GLOBAL(g_bwdKeywordNames),
	RESET_GLOBAL(g_bwdExtension),
	RESET_GLOBAL(g_bwdTypeCodes),

	// bwdnames.c
	RESET_GLOBAL(g_bwdNames),
	RESET_GLOBAL(g_unk0x100a9474),
	RESET_GLOBAL(g_bwdNamesTail),

	// callbacks.c
	RESET_GLOBAL(g_currentCallback),

	// camerashake.c
	RESET_GLOBAL(g_cameraShakeKeyCount),
	RESET_GLOBAL(g_cameraShakeActive),
	RESET_GLOBAL(g_cameraShakeHeading),
	RESET_GLOBAL(g_cameraShakeKey),
	RESET_GLOBAL(g_cameraShakeZ),
	RESET_GLOBAL(g_cameraShakeKeyTime),
	RESET_GLOBAL(g_cameraShakeRoll),
	RESET_GLOBAL(g_cameraShakeX),
	RESET_GLOBAL(g_cameraShakePitch),
	RESET_GLOBAL(g_cameraShakeKeys),
	RESET_GLOBAL(g_cameraShakeY),

	// classtable.c
	RESET_GLOBAL(g_classEntryCount),
	RESET_GLOBAL(g_classTableReady),
	RESET_GLOBAL(g_classTable),

	// clock.c
	RESET_GLOBAL(g_currentClock),
	RESET_GLOBAL(g_realClock),
	RESET_GLOBAL(g_deltaTime),
	RESET_GLOBAL(g_clockMode),
	RESET_GLOBAL(g_clockPaused),
	RESET_GLOBAL(g_timeCompressionEnabled),
	RESET_GLOBAL(g_timeExpansionEnabled),
	RESET_GLOBAL(g_framerateLimit),
	RESET_GLOBAL(g_clockHandle),
	RESET_GLOBAL(g_syncTicksHandle),
	RESET_GLOBAL(g_unk0x100ba574),
	RESET_GLOBAL(g_realClockHandle),
	RESET_GLOBAL(g_previousClock),
	RESET_GLOBAL(g_clockModeBeforePause),
	RESET_GLOBAL(g_ticksTimerInitialized),
	RESET_GLOBAL(g_sqrtTable),
	RESET_GLOBAL(g_slopeSines),
	RESET_GLOBAL(g_slopeCosines),
	RESET_GLOBAL(g_sinTable),
	RESET_GLOBAL(g_sqrtTableData),
	RESET_GLOBAL(g_atanTable),

	// cockpit.c
	RESET_GLOBAL(g_satelliteStaticState),
	RESET_GLOBAL(g_savedFrameDrawCallback),
	RESET_GLOBAL(g_savedShowCrosshair),
	RESET_GLOBAL(g_savedShowHud),
	RESET_GLOBAL(g_savedShowTargetMarker),
	RESET_GLOBAL(g_hudSettingsSaved),
	RESET_GLOBAL(g_mapShadeBase),
	RESET_GLOBAL(g_mapShadeTop),
	RESET_GLOBAL(g_mapShadeRange),
	RESET_GLOBAL(g_cockpitGauges),
	RESET_GLOBAL(g_cockpitGaugePanes),
	RESET_GLOBAL(g_unk0x100a5ad0),
	RESET_GLOBAL(g_unk0x100a5b70),
	RESET_GLOBAL(g_unk0x100a5b90),
	RESET_GLOBAL(g_unk0x100a5bb0),
	RESET_GLOBAL(g_unk0x100a5bb8),
	RESET_GLOBAL(g_mapDamageAnimShown),
	RESET_GLOBAL(g_staticCleanUntil),
	RESET_GLOBAL(g_staticNoiseUntil),
	RESET_GLOBAL(g_previousCockpitView),
	RESET_GLOBAL(g_previousMapViewMode),
	RESET_GLOBAL(g_cockpitLayoutIndex),
	RESET_GLOBAL(g_mapViewMode),
	RESET_GLOBAL(g_requestedCockpitView),
	RESET_GLOBAL(g_mapFollowsFreeEye),
	RESET_GLOBAL(g_satelliteClean),

	// collision.c
	RESET_GLOBAL(g_shapeCollisionFns),
	RESET_GLOBAL(g_hitNormalX),
	RESET_GLOBAL(g_hitNormalY),
	RESET_GLOBAL(g_hitNormalZ),
	RESET_GLOBAL(g_groundNormalX),
	RESET_GLOBAL(g_groundNormalY),
	RESET_GLOBAL(g_groundNormalZ),
	RESET_GLOBAL(g_segmentNormalX),
	RESET_GLOBAL(g_segmentNormalY),
	RESET_GLOBAL(g_segmentNormalZ),
	RESET_GLOBAL(g_backgroundColor),
	RESET_GLOBAL(g_skyColor),
	RESET_GLOBAL(g_groundColor),
	RESET_GLOBAL(g_horizonMapColor),
	RESET_GLOBAL(g_unk0x100a5558),

	// commandmenu.c
	RESET_GLOBAL(g_commandPoint2MenuTarget),
	RESET_GLOBAL(g_commandPoint2MenuBackgroundTarget),
	RESET_GLOBAL(g_commandPoint2Menu),
	RESET_GLOBAL(g_commandComputerTitle),
	RESET_GLOBAL(g_commandAllItem),
	RESET_GLOBAL(g_changeFormationItem),
	RESET_GLOBAL(g_noStarMatesText),
	RESET_GLOBAL(g_currentFormationLabel),
	RESET_GLOBAL(g_commandPoint2Item),
	RESET_GLOBAL(g_commandPoint3Item),
	RESET_GLOBAL(g_commandPoint4Item),
	RESET_GLOBAL(g_commandPoint5Item),
	RESET_GLOBAL(g_statusLabel),
	RESET_GLOBAL(g_notAvailableText),
	RESET_GLOBAL(g_commandAllTitle),
	RESET_GLOBAL(g_changeFormationTitle),
	RESET_GLOBAL(g_commandPoint2Title),
	RESET_GLOBAL(g_commandPoint3Title),
	RESET_GLOBAL(g_commandPoint4Title),
	RESET_GLOBAL(g_commandPoint5Title),
	RESET_GLOBAL(g_echelonLeftText),
	RESET_GLOBAL(g_echelonRightText),
	RESET_GLOBAL(g_lineAbreastText),
	RESET_GLOBAL(g_lineAsternText),
	RESET_GLOBAL(g_vFormText),
	RESET_GLOBAL(g_wedgeText),
	RESET_GLOBAL(g_noFormationText),
	RESET_GLOBAL(g_orderAttackText),
	RESET_GLOBAL(g_orderDefendText),
	RESET_GLOBAL(g_orderJoinFormationText),
	RESET_GLOBAL(g_orderChangeFormationText),
	RESET_GLOBAL(g_orderDisengageText),
	RESET_GLOBAL(g_orderEngageAtWillText),
	RESET_GLOBAL(g_orderShutdownText),
	RESET_GLOBAL(g_orderNoneText),
	RESET_GLOBAL(g_attackMyTargetItem),
	RESET_GLOBAL(g_defendMyTargetItem),
	RESET_GLOBAL(g_joinFormationItem),
	RESET_GLOBAL(g_disengageItem),
	RESET_GLOBAL(g_engageAtWillItem),
	RESET_GLOBAL(g_shutdownItem),
	RESET_GLOBAL(g_aiStateNoneText),
	RESET_GLOBAL(g_aiStateIdleText),
	RESET_GLOBAL(g_aiStateAvoidText),
	RESET_GLOBAL(g_aiStateTargetText),
	RESET_GLOBAL(g_aiStateAttackText),
	RESET_GLOBAL(g_aiStateFleeText),
	RESET_GLOBAL(g_aiStateFollowText),
	RESET_GLOBAL(g_aiStateReconText),
	RESET_GLOBAL(g_aiStatePatrolText),
	RESET_GLOBAL(g_aiStateGoDirectText),
	RESET_GLOBAL(g_aiState9Text),
	RESET_GLOBAL(g_aiStateRestText),
	RESET_GLOBAL(g_aiStateShutdownText),
	RESET_GLOBAL(g_aiStateDeadText),
	RESET_GLOBAL(g_formationChoices),
	RESET_GLOBAL(g_orderChoices),
	RESET_GLOBAL(g_aiStateChoices),
	RESET_GLOBAL(g_noChoices),
	RESET_GLOBAL(g_formationControl),
	RESET_GLOBAL(g_commandAllStatusControl),
	RESET_GLOBAL(g_commandPoint2StatusControl),
	RESET_GLOBAL(g_commandPoint3StatusControl),
	RESET_GLOBAL(g_commandPoint4StatusControl),
	RESET_GLOBAL(g_commandPoint5StatusControl),
	RESET_GLOBAL(g_echelonLeftControl),
	RESET_GLOBAL(g_echelonRightControl),
	RESET_GLOBAL(g_lineAbreastControl),
	RESET_GLOBAL(g_lineAsternControl),
	RESET_GLOBAL(g_vFormControl),
	RESET_GLOBAL(g_wedgeControl),
	RESET_GLOBAL(g_attackControl),
	RESET_GLOBAL(g_engageAtWillControl),
	RESET_GLOBAL(g_joinFormationControl),
	RESET_GLOBAL(g_defendControl),
	RESET_GLOBAL(g_disengageControl),
	RESET_GLOBAL(g_shutdownControl),
	RESET_GLOBAL(g_changeFormationPage),
	RESET_GLOBAL(g_commandAllPage),
	RESET_GLOBAL(g_commandPoint2Page),
	RESET_GLOBAL(g_commandPoint3Page),
	RESET_GLOBAL(g_commandPoint4Page),
	RESET_GLOBAL(g_commandPoint5Page),
	RESET_GLOBAL(g_commandComputerPage),
	RESET_GLOBAL(g_commandMenuTarget),
	RESET_GLOBAL(g_commandMenuBackgroundTarget),
	RESET_GLOBAL(g_commandMenu),
	RESET_GLOBAL(g_commandMenuPageStack),
	RESET_GLOBAL(g_commandPoint2MenuPageStack),

	// commandpointmenu.c
	RESET_GLOBAL(g_commandPoint3MenuTarget),
	RESET_GLOBAL(g_commandPoint3MenuBackgroundTarget),
	RESET_GLOBAL(g_commandPoint3Menu),
	RESET_GLOBAL(g_commandPoint3MenuPageStack),

	// config.c
	RESET_GLOBAL(g_cockpitPanelPanes),
	RESET_GLOBAL(g_cockpitPanelTextOrigins),
	RESET_GLOBAL(g_targetTransitionState),
	RESET_GLOBAL(g_mechViewTransitionState),
	RESET_GLOBAL(g_targetTransitionFirst),
	RESET_GLOBAL(g_targetTransitionSecond),
	RESET_GLOBAL(g_targetTransitionRect),
	RESET_GLOBAL(g_targetTransitionDef),
	RESET_GLOBAL(g_targetTransition),
	RESET_GLOBAL(g_mechViewTransitionFirst),
	RESET_GLOBAL(g_mechViewTransitionSecond),
	RESET_GLOBAL(g_mechViewTransitionRect),
	RESET_GLOBAL(g_mechViewTransitionDef),
	RESET_GLOBAL(g_mechViewTransition),
	RESET_GLOBAL(g_cockpitPanelTransitions),
	RESET_GLOBAL(g_punchInAutoHeadingRequested),
	RESET_GLOBAL(g_hitFadePending),
	RESET_GLOBAL(g_cockpitPanelLightUpTimes),
	RESET_GLOBAL(g_lastWarningPowerState),
	RESET_GLOBAL(g_lockedTonePlayed),
	RESET_GLOBAL(g_lockingTonePlayed),
	RESET_GLOBAL(g_hitFadeCount),
	RESET_GLOBAL(g_gameDir),
	RESET_GLOBAL(g_screenshotCount),
	RESET_GLOBAL(g_gamePath),
	RESET_GLOBAL(g_torsoTwistDegrees),
	RESET_GLOBAL(g_headingDegrees),
	RESET_GLOBAL(g_cockpitPanels),
	RESET_GLOBAL(g_cockpitPanelEnabled),
	RESET_GLOBAL(g_cockpitPowerState),
	RESET_GLOBAL(g_hudLayoutValues),

	// damagepanel.c
	RESET_GLOBAL(g_outlinePartSections),
	RESET_GLOBAL(g_htalLabelHPosition),
	RESET_GLOBAL(g_htalLabelTPosition),
	RESET_GLOBAL(g_htalLabelAPosition),
	RESET_GLOBAL(g_htalLabelLPosition),
	RESET_GLOBAL(g_maxSectionArmor),
	RESET_GLOBAL(g_outlineRect),
	RESET_GLOBAL(g_outlinePartRects),
	RESET_GLOBAL(g_outlinePartOffsets),
	RESET_GLOBAL(g_frameOutlineParts),
	RESET_GLOBAL(g_htalLabelH),
	RESET_GLOBAL(g_htalLabelT),
	RESET_GLOBAL(g_htalLabelA),
	RESET_GLOBAL(g_htalLabelL),
	RESET_GLOBAL(g_armorBarPositions),
	RESET_GLOBAL(g_fullSectionArmor),
	RESET_GLOBAL(g_outlineRemap),
	RESET_GLOBAL(g_armorBarSize),

	// debris.c
	RESET_GLOBAL(g_debrisCount),
	RESET_GLOBAL(g_emptyDebrisChunk),
	RESET_GLOBAL(g_debrisPieces),
	RESET_GLOBAL(g_debrisChunks),

	// debugprint.c
	RESET_GLOBAL(g_debugPrintBuffer),

	// depthsort.c
	RESET_GLOBAL(g_depthEntryCount),
	RESET_GLOBAL(g_polygonCount),
	RESET_GLOBAL(g_maxPolygons),
	RESET_GLOBAL(g_shapesDrawn),
	RESET_GLOBAL(g_queueDepth),
	RESET_GLOBAL(g_depthList),
	RESET_GLOBAL(g_queuedShapeFlags),
	RESET_GLOBAL(g_shapesConsidered),

	// dorcs.c
	RESET_GLOBAL(g_dorcsPageTitle),
	RESET_GLOBAL(g_dorcsAboutItem),
	RESET_GLOBAL(g_dorcsClarkeName),
	RESET_GLOBAL(g_dorcsDouglasName),
	RESET_GLOBAL(g_dorcsEthertonName),
	RESET_GLOBAL(g_dorcsHusebyName),
	RESET_GLOBAL(g_dorcsKaminsName),
	RESET_GLOBAL(g_dorcsKeatingName),
	RESET_GLOBAL(g_dorcsMilesName),
	RESET_GLOBAL(g_dorcsMortenName),
	RESET_GLOBAL(g_dorcsMortensenName),
	RESET_GLOBAL(g_dorcsPetersonName),
	RESET_GLOBAL(g_dorcsStanfillName),
	RESET_GLOBAL(g_dorcsWhiteName),
	RESET_GLOBAL(g_dorcsZobelName),
	RESET_GLOBAL(g_dorcsAboutText),
	RESET_GLOBAL(g_dorcsClarkeText),
	RESET_GLOBAL(g_dorcsDouglasText),
	RESET_GLOBAL(g_dorcsEthertonText),
	RESET_GLOBAL(g_dorcsHusebyText),
	RESET_GLOBAL(g_dorcsKaminsText),
	RESET_GLOBAL(g_dorcsKeatingText),
	RESET_GLOBAL(g_dorcsMilesText),
	RESET_GLOBAL(g_dorcsMortenText),
	RESET_GLOBAL(g_dorcsMortensenText),
	RESET_GLOBAL(g_dorcsPetersonText),
	RESET_GLOBAL(g_dorcsStanfillText),
	RESET_GLOBAL(g_dorcsWhiteText),
	RESET_GLOBAL(g_dorcsZobelText),
	RESET_GLOBAL(g_dorcsExitItem),
	RESET_GLOBAL(g_dorcsTextRect),
	RESET_GLOBAL(g_dorcsAboutTextBox),
	RESET_GLOBAL(g_dorcsClarkeTextBox),
	RESET_GLOBAL(g_dorcsDouglasTextBox),
	RESET_GLOBAL(g_dorcsEthertonTextBox),
	RESET_GLOBAL(g_dorcsHusebyTextBox),
	RESET_GLOBAL(g_dorcsKaminsTextBox),
	RESET_GLOBAL(g_dorcsKeatingTextBox),
	RESET_GLOBAL(g_dorcsMilesTextBox),
	RESET_GLOBAL(g_dorcsMortenTextBox),
	RESET_GLOBAL(g_dorcsMortensenTextBox),
	RESET_GLOBAL(g_dorcsPetersonTextBox),
	RESET_GLOBAL(g_dorcsStanfillTextBox),
	RESET_GLOBAL(g_dorcsWhiteTextBox),
	RESET_GLOBAL(g_dorcsZobelTextBox),
	RESET_GLOBAL(g_dorcsAboutControl),
	RESET_GLOBAL(g_dorcsClarkeControl),
	RESET_GLOBAL(g_dorcsDouglasControl),
	RESET_GLOBAL(g_dorcsEthertonControl),
	RESET_GLOBAL(g_dorcsHusebyControl),
	RESET_GLOBAL(g_dorcsKaminsControl),
	RESET_GLOBAL(g_dorcsKeatingControl),
	RESET_GLOBAL(g_dorcsMilesControl),
	RESET_GLOBAL(g_dorcsMortenControl),
	RESET_GLOBAL(g_dorcsMortensenControl),
	RESET_GLOBAL(g_dorcsPetersonControl),
	RESET_GLOBAL(g_dorcsStanfillControl),
	RESET_GLOBAL(g_dorcsWhiteControl),
	RESET_GLOBAL(g_dorcsZobelControl),
	RESET_GLOBAL(g_dorcsAboutPage),
	RESET_GLOBAL(g_dorcsClarkePage),
	RESET_GLOBAL(g_dorcsDouglasPage),
	RESET_GLOBAL(g_dorcsEthertonPage),
	RESET_GLOBAL(g_dorcsHusebyPage),
	RESET_GLOBAL(g_dorcsKaminsPage),
	RESET_GLOBAL(g_dorcsKeatingPage),
	RESET_GLOBAL(g_dorcsMilesPage),
	RESET_GLOBAL(g_dorcsMortenPage),
	RESET_GLOBAL(g_dorcsMortensenPage),
	RESET_GLOBAL(g_dorcsPetersonPage),
	RESET_GLOBAL(g_dorcsStanfillPage),
	RESET_GLOBAL(g_dorcsWhitePage),
	RESET_GLOBAL(g_dorcsZobelPage),
	RESET_GLOBAL(g_dorcsPage),
	RESET_GLOBAL(g_dorcsMenuTarget),
	RESET_GLOBAL(g_dorcsMenuBackgroundTarget),
	RESET_GLOBAL(g_dorcsMenu),
	RESET_GLOBAL(g_fledToWindows),
	RESET_GLOBAL(g_dorcsPreviousDrawCallback),
	RESET_GLOBAL(g_dorcsMenuPageStack),
	RESET_GLOBAL(g_dorcsPoint),
	RESET_GLOBAL(g_dorcsRectFrom),
	RESET_GLOBAL(g_dorcsRectTo),
	RESET_GLOBAL(g_dorcsRect),
	RESET_GLOBAL(g_dorcsTransitionState),
	RESET_GLOBAL(g_dorcsTransitionDef),
	RESET_GLOBAL(g_dorcsTransition),
	RESET_GLOBAL(g_dorcsGif),
	RESET_GLOBAL(g_dorcsPalette),
	RESET_GLOBAL(g_dorcsGifState),
	RESET_GLOBAL(g_dorcsGifLoaded),
	RESET_GLOBAL(g_dorcsTime),
	RESET_GLOBAL(g_dorcsReverse),
	RESET_GLOBAL(g_dorcsState),
	RESET_GLOBAL(g_dorcsSavedTarget),
	RESET_GLOBAL(g_dorcsGifTarget),
	RESET_GLOBAL(g_dorcsBlack),

	// environment.c
	RESET_GLOBAL(g_timeOfDayPhases),
	RESET_GLOBAL(g_timeOfDayPhase),
	RESET_GLOBAL(g_soundDelayPerUnit),
	RESET_GLOBAL(g_gravity),
	RESET_GLOBAL(g_gravityScale),
	RESET_GLOBAL(g_unk0x100ba608),
	RESET_GLOBAL(g_secondsPerDay),
	RESET_GLOBAL(g_daysPerYear),
	RESET_GLOBAL(g_dayOfYear),
	RESET_GLOBAL(g_timeOfDay),
	RESET_GLOBAL(g_startTimeOfDay),
	RESET_GLOBAL(g_timeOfDayFrames),
	RESET_GLOBAL(g_timeOfDayStarts),
	RESET_GLOBAL(g_timeOfDayEnabled),
	RESET_GLOBAL(g_nextTimeOfDayUpdate),
	RESET_GLOBAL(g_infraredOn),

	// error.c
	RESET_GLOBAL(g_fatalErrorTitle),
	RESET_GLOBAL(g_warningTitle),
	RESET_GLOBAL(g_errorCode),
	RESET_GLOBAL(g_errorMessage),

	// eyepoint.c
	RESET_GLOBAL(g_trackDistance),
	RESET_GLOBAL(g_trackMinDistance),
	RESET_GLOBAL(g_trackMaxDistance),
	RESET_GLOBAL(g_trackHeight),
	RESET_GLOBAL(g_trackTurn),
	RESET_GLOBAL(g_normalFov),
	RESET_GLOBAL(g_zoomFov),
	RESET_GLOBAL(g_viewMode),
	RESET_GLOBAL(g_ordinanceReturnMode),
	RESET_GLOBAL(g_initialViewMode),
	RESET_GLOBAL(g_requestedViewMode),
	RESET_GLOBAL(g_autopilotStart),
	RESET_GLOBAL(g_cockpitEyeSteady),
	RESET_GLOBAL(g_inCockpitView),
	RESET_GLOBAL(g_lostViewMode),
	RESET_GLOBAL(g_spectating),
	RESET_GLOBAL(g_localPlayer),
	RESET_GLOBAL(g_trackedPlayer),
	RESET_GLOBAL(g_eyeHeightOffset),
	RESET_GLOBAL(g_eyeTwist),
	RESET_GLOBAL(g_dropSpeed),
	RESET_GLOBAL(g_dropAcceleration),
	RESET_GLOBAL(g_dropStartClock),
	RESET_GLOBAL(g_glanceReleased),
	RESET_GLOBAL(g_ordinanceSavedView),
	RESET_GLOBAL(g_trackMaxHeight),
	RESET_GLOBAL(g_trackOffsetZ),
	RESET_GLOBAL(g_trackPitch),
	RESET_GLOBAL(g_trackHeading),
	RESET_GLOBAL(g_trackMinHeight),
	RESET_GLOBAL(g_pilotTilt),
	RESET_GLOBAL(g_trackOffsetY),
	RESET_GLOBAL(g_savedViews),
	RESET_GLOBAL(g_pilotPan),
	RESET_GLOBAL(g_freeEyeSpeed),
	RESET_GLOBAL(g_trackOffsetX),

	// faceshade.c
	RESET_GLOBAL(g_brightenDamage),
	RESET_GLOBAL(g_ambientLight),

	// gamekeys.c
	RESET_GLOBAL(g_missionEndTime),
	RESET_GLOBAL(g_speechFlushTime),
	RESET_GLOBAL(g_overrideShutdown),
	RESET_GLOBAL(g_feetToTorso),
	RESET_GLOBAL(g_mechViewMode),
	RESET_GLOBAL(g_missionEnded),
	RESET_GLOBAL(g_missionTimerStopped),
	RESET_GLOBAL(g_timeCompressionCheat),
	RESET_GLOBAL(g_chatLength),
	RESET_GLOBAL(g_missionResolved),
	RESET_GLOBAL(g_statusMessage),
	RESET_GLOBAL(g_typedKeys),
	RESET_GLOBAL(g_frontViewForRear),

	// geocache.c
	RESET_GLOBAL(g_thingIndices),
	RESET_GLOBAL(g_thingIds),
	RESET_GLOBAL(g_inRepeat),
	RESET_GLOBAL(g_repeatPass),
	RESET_GLOBAL(g_starIndices),
	RESET_GLOBAL(g_starIds),
	RESET_GLOBAL(g_classCount),
	RESET_GLOBAL(g_classCapacity),
	RESET_GLOBAL(g_classes),
	RESET_GLOBAL(g_staticObjectCount),
	RESET_GLOBAL(g_staticCacheReady),
	RESET_GLOBAL(g_nextBlock),
	RESET_GLOBAL(g_currentBlock),
	RESET_GLOBAL(g_blockDepth),
	RESET_GLOBAL(g_pendingXform),
	RESET_GLOBAL(g_defaultXform),
	RESET_GLOBAL(g_explosionChunks),
	RESET_GLOBAL(g_staticBlocks),
	RESET_GLOBAL(g_staticObjects),
	RESET_GLOBAL(g_starCapacity),
	RESET_GLOBAL(g_thingCount),
	RESET_GLOBAL(g_blockStack),
	RESET_GLOBAL(g_staticObjectsChanged),
	RESET_GLOBAL(g_blockBoxes),
	RESET_GLOBAL(g_thingCapacity),
	RESET_GLOBAL(g_blockBoxesShown),
	RESET_GLOBAL(g_starCount),

	// gifsave.c
	RESET_GLOBAL(g_outFile),
	RESET_GLOBAL(g_gifBuffer),
	RESET_GLOBAL(g_gifIndex),
	RESET_GLOBAL(g_bitsLeft),
	RESET_GLOBAL(g_strChr),
	RESET_GLOBAL(g_strNxt),
	RESET_GLOBAL(g_strHsh),
	RESET_GLOBAL(g_numStrings),
	RESET_GLOBAL(g_bitsPrPrimColor),
	RESET_GLOBAL(g_numColors),
	RESET_GLOBAL(g_colorTable),
	RESET_GLOBAL(g_gifScreenHeight),
	RESET_GLOBAL(g_gifScreenWidth),
	RESET_GLOBAL(g_imageHeight),
	RESET_GLOBAL(g_imageWidth),
	RESET_GLOBAL(g_imageLeft),
	RESET_GLOBAL(g_imageTop),
	RESET_GLOBAL(g_relPixX),
	RESET_GLOBAL(g_relPixY),
	RESET_GLOBAL(g_getPixel),

	// gpanim.c
	RESET_GLOBAL(g_motionSounds),
	RESET_GLOBAL(g_lastMotionSound),

	// gridobject.c
	RESET_GLOBAL(g_gridCellSize),
	RESET_GLOBAL(g_gridObjectSet),
	RESET_GLOBAL(g_gridObjectPlaced),
	RESET_GLOBAL(g_gridObjectShown),
	RESET_GLOBAL(g_gridObjectSnaps),
	RESET_GLOBAL(g_gridObject),
	RESET_GLOBAL(g_gridCell),
	RESET_GLOBAL(g_gridAnchor),
	RESET_GLOBAL(g_gridSnapDistance),

	// hud.c
	RESET_GLOBAL(g_altimeterMarkWidth),
	RESET_GLOBAL(g_altimeterMarkHeight),
	RESET_GLOBAL(g_altimeterOrigin),
	RESET_GLOBAL(g_compassOrigin),
	RESET_GLOBAL(g_hudGaugePositions),
	RESET_GLOBAL(g_showHud),
	RESET_GLOBAL(g_showCrosshair),
	RESET_GLOBAL(g_showTargetMarker),
	RESET_GLOBAL(g_showCompass),
	RESET_GLOBAL(g_showAltimeter),
	RESET_GLOBAL(g_compassArrowWidth),
	RESET_GLOBAL(g_compassArrowHeight),
	RESET_GLOBAL(g_compassSideArrowWidth),
	RESET_GLOBAL(g_compassSideArrowHeight),
	RESET_GLOBAL(g_altimeterGroundX),
	RESET_GLOBAL(g_altimeterTargetX),
	RESET_GLOBAL(g_altimeterScale),
	RESET_GLOBAL(g_compassScale),
	RESET_GLOBAL(g_compassTapeAbove),
	RESET_GLOBAL(g_altimeterLevelX),
	RESET_GLOBAL(g_compassTapeBelow),

	// inputmap.c
	RESET_GLOBAL(g_localSteering),
	RESET_GLOBAL(g_sinkPilotTilt),
	RESET_GLOBAL(g_sinkPilotPan),
	RESET_GLOBAL(g_sinkEyepointTilt),
	RESET_GLOBAL(g_sinkEyepointPanDelta),
	RESET_GLOBAL(g_sinkEyepointSlideDelta),
	RESET_GLOBAL(g_sinkTrackDistanceDelta),
	RESET_GLOBAL(g_sinkTrackHeightDelta),
	RESET_GLOBAL(g_sinkZoomFactor),
	RESET_GLOBAL(g_sinkPilotTiltPlus),
	RESET_GLOBAL(g_sinkPilotTiltMinus),
	RESET_GLOBAL(g_sinkPilotTiltReset),
	RESET_GLOBAL(g_sinkPilotPanPlus),
	RESET_GLOBAL(g_sinkPilotPanMinus),
	RESET_GLOBAL(g_sinkPilotPanReset),
	RESET_GLOBAL(g_sinkGlanceLeft),
	RESET_GLOBAL(g_sinkGlanceRight),
	RESET_GLOBAL(g_sinkGlanceUp),
	RESET_GLOBAL(g_sinkGlanceDown),
	RESET_GLOBAL(g_sinkEyepointTiltPlus),
	RESET_GLOBAL(g_sinkEyepointTiltMinus),
	RESET_GLOBAL(g_sinkEyepointTiltReset),
	RESET_GLOBAL(g_sinkEyepointPanPlus),
	RESET_GLOBAL(g_sinkEyepointPanMinus),
	RESET_GLOBAL(g_sinkEyepointPanReset),
	RESET_GLOBAL(g_sinkEyepointSlidePlus),
	RESET_GLOBAL(g_sinkEyepointSlideMinus),
	RESET_GLOBAL(g_sinkTrackDistancePlus),
	RESET_GLOBAL(g_sinkTrackDistanceMinus),
	RESET_GLOBAL(g_sinkTrackHeightPlus),
	RESET_GLOBAL(g_sinkTrackHeightMinus),
	RESET_GLOBAL(g_sinkZoomFactorPlus),
	RESET_GLOBAL(g_sinkZoomFactorMinus),
	RESET_GLOBAL(g_sinkZoomFactorReset),
	RESET_GLOBAL(g_sinkMenuItem),
	RESET_GLOBAL(g_sinkMenuValue),
	RESET_GLOBAL(g_sinkMenuItemReset),
	RESET_GLOBAL(g_sinkMenuValueReset),
	RESET_GLOBAL(g_sinkMenuEnter),
	RESET_GLOBAL(g_sinkMenuAbort),
	RESET_GLOBAL(g_gameplayInputEnabled),
	RESET_GLOBAL(g_inputSinks),

	// keyboard.c
	RESET_GLOBAL(g_keyCodeWriteIndex),
	RESET_GLOBAL(g_keyCodeReadIndex),
	RESET_GLOBAL(g_keyCodes),
	RESET_GLOBAL(g_keyStates),
	RESET_GLOBAL(g_keyCodeMap),
	RESET_GLOBAL(g_extendedScanCodeMap),

	// lancemenu.c
	RESET_GLOBAL(g_lanceOrders),

	// loadres.c
	RESET_GLOBAL(g_cacheTable),
	RESET_GLOBAL(g_cacheDumpNumber),
	RESET_GLOBAL(g_cacheEntryCount),
	RESET_GLOBAL(g_purgeListHead),
	RESET_GLOBAL(g_purgeListTail),

	// logwindow.c
	RESET_GLOBAL(g_debugOutputMode),
	RESET_GLOBAL(g_debugLogFile),
	RESET_GLOBAL(g_debugLogName),

	// mainmenu.c
	RESET_GLOBAL(g_mainMenuTitle),
	RESET_GLOBAL(g_abortMissionItem),
	RESET_GLOBAL(g_monitorBrightnessItem),
	RESET_GLOBAL(g_fleeToWindowsItem),
	RESET_GLOBAL(g_acceptText),
	RESET_GLOBAL(g_escToExitText),
	RESET_GLOBAL(g_fleeToWindowsTitle),
	RESET_GLOBAL(g_abortMissionTitle),
	RESET_GLOBAL(g_monitorBrightnessTitle),
	RESET_GLOBAL(g_areYouSureText),
	RESET_GLOBAL(g_confirmCowardiceText),
	RESET_GLOBAL(g_confirmationRequestedText),
	RESET_GLOBAL(g_noText),
	RESET_GLOBAL(g_yesText),
	RESET_GLOBAL(g_offText),
	RESET_GLOBAL(g_onText),
	RESET_GLOBAL(g_lowText),
	RESET_GLOBAL(g_mediumText),
	RESET_GLOBAL(g_highText),
	RESET_GLOBAL(g_offOnChoices),
	RESET_GLOBAL(g_noYesChoices),
	RESET_GLOBAL(g_lowHighChoices),
	RESET_GLOBAL(g_offLowMediumHighChoices),
	RESET_GLOBAL(g_sliderShapes),
	RESET_GLOBAL(g_confirmControl),
	RESET_GLOBAL(g_brightnessControl),
	RESET_GLOBAL(g_abortMissionPage),
	RESET_GLOBAL(g_brightnessPage),
	RESET_GLOBAL(g_fleePage),
	RESET_GLOBAL(g_mainMenuPage),
	RESET_GLOBAL(g_mainMenuTarget),
	RESET_GLOBAL(g_mainMenuBackgroundTarget),
	RESET_GLOBAL(g_mainMenu),
	RESET_GLOBAL(g_mainMenuPageStack),

	// maneuvers.c
	RESET_GLOBAL(g_probeDirections),
	RESET_GLOBAL(g_mechManeuvers),
	RESET_GLOBAL(g_stupidManeuvers),
	RESET_GLOBAL(g_circleManeuvers),
	RESET_GLOBAL(g_behindManeuvers),
	RESET_GLOBAL(g_altMechManeuvers),
	RESET_GLOBAL(g_maneuverTablesReady),
	RESET_GLOBAL(g_jumpJetDrag),
	RESET_GLOBAL(g_slideSlope),
	RESET_GLOBAL(g_maneuverTables),

	// mapview.c
	RESET_GLOBAL(g_savedPalettePending),
	RESET_GLOBAL(g_savedEyepoint),
	RESET_GLOBAL(g_mapViewMinX),
	RESET_GLOBAL(g_mapViewMaxX),
	RESET_GLOBAL(g_mapViewMaxY),
	RESET_GLOBAL(g_mapViewMinY),
	RESET_GLOBAL(g_mapViewNear),
	RESET_GLOBAL(g_mapViewFar),
	RESET_GLOBAL(g_mapViewScale),
	RESET_GLOBAL(g_savedRenderSettings),

	// mechclass.c
	RESET_GLOBAL(g_infiniteJumpFuel),
	RESET_GLOBAL(g_jettisonAmmoRequested),
	RESET_GLOBAL(g_toggleMascRequested),
	RESET_GLOBAL(g_mascEngaged),
	RESET_GLOBAL(g_unk0x100a2bf4),
	RESET_GLOBAL(g_manualWeaponCycle),
	RESET_GLOBAL(g_unk0x100a2bfc),
	RESET_GLOBAL(g_unk0x100a2c00),
	RESET_GLOBAL(g_localMechLost),
	RESET_GLOBAL(g_powerRequest),
	RESET_GLOBAL(g_recenterLastHeading),
	RESET_GLOBAL(g_mechPoweredUp),
	RESET_GLOBAL(g_lastMascRoll),
	RESET_GLOBAL(g_localMechDestroyed),
	RESET_GLOBAL(g_ejectStarted),
	RESET_GLOBAL(g_collisionSoundPlayed),

	// mechdamage.c
	RESET_GLOBAL(g_autoEject),
	RESET_GLOBAL(g_otherArmorPerLevel),
	RESET_GLOBAL(g_localArmorPerLevel),
	RESET_GLOBAL(g_localMechHidden),
	RESET_GLOBAL(g_killCount),
	RESET_GLOBAL(g_criticalHeatWarned),
	RESET_GLOBAL(g_criticalHeatWarningTime),

	// mechreload.c
	RESET_GLOBAL(g_reloadingPlayer),
	RESET_GLOBAL(g_unk0x100ba694),
	RESET_GLOBAL(g_rememberedMechs),
	RESET_GLOBAL(g_mechSegments),

	// mechviewpanel.c
	RESET_GLOBAL(g_mechViewStatic),

	// mekfile.c
	RESET_GLOBAL(g_weaponValues),

	// menu.c
	RESET_GLOBAL(g_menuRepeatTimer),
	RESET_GLOBAL(g_menuDefinitions),
	RESET_GLOBAL(g_textColors),
	RESET_GLOBAL(g_openMenuCount),
	RESET_GLOBAL(g_menuSlotsTail),
	RESET_GLOBAL(g_menuSlots),
	RESET_GLOBAL(g_menuKey),

	// missionaudio.c
	RESET_GLOBAL(g_soundFileCount),
	RESET_GLOBAL(g_soundFileEntries),
	RESET_GLOBAL(g_soundFileTable),
	RESET_GLOBAL(g_soundFileDir),

	// mouse.c
	RESET_GLOBAL(g_cursorClipped),
	RESET_GLOBAL(g_reclipCursor),

	// mw2log.c
	RESET_GLOBAL(g_logFileEnabled),
	RESET_GLOBAL(g_mw2Log),

	// mw2prj.c
	RESET_GLOBAL(g_resourceTypeTags),
	RESET_GLOBAL(g_resourceTypeExtensions),
	RESET_GLOBAL(g_mw2PrjHandle),
	RESET_GLOBAL(g_mw2PrjPath),

	// netio.c
	RESET_GLOBAL(g_sendRetries),
	RESET_GLOBAL(g_recvRetries),
	RESET_GLOBAL(g_sendResult),

	// network.c
	RESET_GLOBAL(g_stateInterval),
	RESET_GLOBAL(g_isNetworkGame),
	RESET_GLOBAL(g_sessionName),
	RESET_GLOBAL(g_clockSynced),
	RESET_GLOBAL(g_launchedByShell),
	RESET_GLOBAL(g_stateCount),
	RESET_GLOBAL(g_messagesReceived),
	RESET_GLOBAL(g_unrecognizedMessages),
	RESET_GLOBAL(g_successSent),
	RESET_GLOBAL(g_gameStarted),
	RESET_GLOBAL(g_netLaunch),
	RESET_GLOBAL(g_stateMsg),
	RESET_GLOBAL(g_weaponsMsg),
	RESET_GLOBAL(g_thingsMsg),
	RESET_GLOBAL(g_chatMsg),
	RESET_GLOBAL(g_netState),
	RESET_GLOBAL(g_netRole),
	RESET_GLOBAL(g_sessionGuid),
	RESET_GLOBAL(g_directPlay),
	RESET_GLOBAL(g_localDpid),
	RESET_GLOBAL(g_masterDpid),
	RESET_GLOBAL(g_serviceProvider),
	RESET_GLOBAL(g_nextStateTime),
	RESET_GLOBAL(g_netRecvBuffer),
	RESET_GLOBAL(g_unk0x101770cc),
	RESET_GLOBAL(g_stateMsgSize),
	RESET_GLOBAL(g_lastHeard),
	RESET_GLOBAL(g_lastStateClock),
	RESET_GLOBAL(g_playerReady),
	RESET_GLOBAL(g_playersFound),
	RESET_GLOBAL(g_thingsMsgSize),
	RESET_GLOBAL(g_playerDestroyed),
	RESET_GLOBAL(g_lastThingsCount),

	// object.c
	RESET_GLOBAL(g_nextRenormalizeCountdown),

	// objectanim.c
	RESET_GLOBAL(g_reelMotionError),
	RESET_GLOBAL(g_pathCount),
	RESET_GLOBAL(g_maxAnimNumber),
	RESET_GLOBAL(g_animBase),
	RESET_GLOBAL(g_animFileCount),
	RESET_GLOBAL(g_directionalLight),
	RESET_GLOBAL(g_polygonOrCodes),
	RESET_GLOBAL(g_polygonAndCodes),
	RESET_GLOBAL(g_polygonPoints),
	RESET_GLOBAL(g_polygonPointCount),
	RESET_GLOBAL(g_polygonPointCursor),
	RESET_GLOBAL(g_facesTried),
	RESET_GLOBAL(g_facesFrontFacing),
	RESET_GLOBAL(g_verticesTransformed),
	RESET_GLOBAL(g_polygonsQueued),
	RESET_GLOBAL(g_paths),
	RESET_GLOBAL(g_reels),
	RESET_GLOBAL(g_animFiles),

	// objective.c
	RESET_GLOBAL(g_forceMissionSuccess),
	RESET_GLOBAL(g_missionResultAnnounced),
	RESET_GLOBAL(g_missionResultTag),
	RESET_GLOBAL(g_missionTime),
	RESET_GLOBAL(g_currentObjective),
	RESET_GLOBAL(g_objectiveAnnounced),
	RESET_GLOBAL(g_objectiveCount),
	RESET_GLOBAL(g_objectiveTable),

	// overlay.c
	RESET_GLOBAL(g_blankText),
	RESET_GLOBAL(g_showPalette),
	RESET_GLOBAL(g_unk0x100a94a0),
	RESET_GLOBAL(g_monoLastRow),
	RESET_GLOBAL(g_unk0x100a94a8),
	RESET_GLOBAL(g_unk0x100a94ac),
	RESET_GLOBAL(g_frameRateShown),
	RESET_GLOBAL(g_showFrameRate),
	RESET_GLOBAL(g_showFrameRateMain),
	RESET_GLOBAL(g_unk0x100a94bc),
	RESET_GLOBAL(g_frameRateOrigin),
	RESET_GLOBAL(g_memInfoOrigin),
	RESET_GLOBAL(g_eyePositionOrigin),
	RESET_GLOBAL(g_showSceneInfo),
	RESET_GLOBAL(g_showSceneInfoMain),
	RESET_GLOBAL(g_sceneInfoShown),
	RESET_GLOBAL(g_showEyePosition),
	RESET_GLOBAL(g_showEyePositionMain),
	RESET_GLOBAL(g_eyePositionShown),
	RESET_GLOBAL(g_showMemInfo),
	RESET_GLOBAL(g_showMemInfoMain),
	RESET_GLOBAL(g_memInfoShown),
	RESET_GLOBAL(g_showCacheInfo),
	RESET_GLOBAL(g_cacheInfoShown),
	RESET_GLOBAL(g_showSpinner),
	RESET_GLOBAL(g_spinnerArrows),
	RESET_GLOBAL(g_nextCacheDump),
	RESET_GLOBAL(g_dumpCacheRepeat),
	RESET_GLOBAL(g_unk0x100a9514),
	RESET_GLOBAL(g_unk0x100a9518),
	RESET_GLOBAL(g_frameRate),
	RESET_GLOBAL(g_frameRateTenths),
	RESET_GLOBAL(g_frameRateTime),
	RESET_GLOBAL(g_frameRateFrames),
	RESET_GLOBAL(g_spinnerDelay),
	RESET_GLOBAL(g_spinnerArrow),
	RESET_GLOBAL(g_monoRow),
	RESET_GLOBAL(g_monoColumn),
	RESET_GLOBAL(g_sceneVertexCount),
	RESET_GLOBAL(g_sceneMemory),
	RESET_GLOBAL(g_sceneShapeCount),
	RESET_GLOBAL(g_sceneFaceCount),
	RESET_GLOBAL(g_monoEnabled),
	RESET_GLOBAL(g_monoBlankLine),
	RESET_GLOBAL(g_monoBlankLineEnd),

	// palette.c
	RESET_GLOBAL(g_paneIndex),
	RESET_GLOBAL(g_currentPalette),
	RESET_GLOBAL(g_palettePending),
	RESET_GLOBAL(g_basePalette),
	RESET_GLOBAL(g_settledPalette),
	RESET_GLOBAL(g_paletteFadeTarget),
	RESET_GLOBAL(g_paletteFadeBack),
	RESET_GLOBAL(g_paletteFadeBackSteps),
	RESET_GLOBAL(g_paletteFadeSteps),
	RESET_GLOBAL(g_paletteCycling),
	RESET_GLOBAL(g_paletteCycleResource),
	RESET_GLOBAL(g_paletteFade),
	RESET_GLOBAL(g_paletteCycle),
	RESET_GLOBAL(g_panes),
	RESET_GLOBAL(g_paletteResourceIds),

	// pausebanner.c
	RESET_GLOBAL(g_debugSlot),
	RESET_GLOBAL(g_debugSection),
	RESET_GLOBAL(g_pausedBannerRect),
	RESET_GLOBAL(g_pausedBannerUnscaled),

	// perf.c
	RESET_GLOBAL(g_combatVariablesItem),
	RESET_GLOBAL(g_combatVariablesTitle),
	RESET_GLOBAL(g_objectTextmapsItem),
	RESET_GLOBAL(g_terrainTextmapsItem),
	RESET_GLOBAL(g_displayDetailItem),
	RESET_GLOBAL(g_objectDensityItem),
	RESET_GLOBAL(g_explosionChunksItem),
	RESET_GLOBAL(g_affineText),
	RESET_GLOBAL(g_perspectiveText),
	RESET_GLOBAL(g_affinePerspectiveChoices),
	RESET_GLOBAL(g_objectTextmapsControl),
	RESET_GLOBAL(g_terrainTextmapsControl),
	RESET_GLOBAL(g_displayDetailControl),
	RESET_GLOBAL(g_objectDensityControl),
	RESET_GLOBAL(g_explosionChunksControl),
	RESET_GLOBAL(g_combatVariablesPage),

	// players.c
	RESET_GLOBAL(g_playerTypes),
	RESET_GLOBAL(g_playerCount),
	RESET_GLOBAL(g_gameThingCount),
	RESET_GLOBAL(g_players),
	RESET_GLOBAL(g_gameThings),

	// polydraw.c
	RESET_GLOBAL(g_mainEyepoint),
	RESET_GLOBAL(g_eyepoint),
	RESET_GLOBAL(g_renderSettings),
	RESET_GLOBAL(g_horizonBandHeight),
	RESET_GLOBAL(g_lineEndX),
	RESET_GLOBAL(g_lineEndY),

	// poolsizes.c
	RESET_GLOBAL(g_staticPoolTags),
	RESET_GLOBAL(g_missionPlayers),
	RESET_GLOBAL(g_unk0x100e9daa),
	RESET_GLOBAL(g_missionObjects),
	RESET_GLOBAL(g_missionClassEntries),
	RESET_GLOBAL(g_unk0x100e9db0),
	RESET_GLOBAL(g_unk0x100e9db2),
	RESET_GLOBAL(g_missionAnims),
	RESET_GLOBAL(g_missionAnimTracks),
	RESET_GLOBAL(g_missionAnimFrameBytes),
	RESET_GLOBAL(g_staticPoolSizes),

	// prjfile.c
	RESET_GLOBAL(g_prjAlloc),
	RESET_GLOBAL(g_prjFree),
	RESET_GLOBAL(g_prjFiles),

	// quadtree.c
	RESET_GLOBAL(g_quadtreesDisabled),

	// random.c
	RESET_GLOBAL(g_normalRandomIndex),
	RESET_GLOBAL(g_randomIndex2),
	RESET_GLOBAL(g_normalRandomIndex2),
	RESET_GLOBAL(g_normalRandomInts),
	RESET_GLOBAL(g_randomInts),

	// recordstacks.c
	RESET_GLOBAL(g_drawBufferSize),
	RESET_GLOBAL(g_drawBufferMemory),
	RESET_GLOBAL(g_depthListCapacity),
	RESET_GLOBAL(g_drawBuffer),
	RESET_GLOBAL(g_drawBufferTop),
	RESET_GLOBAL(g_drawList),
	RESET_GLOBAL(g_drawBufferBottom),
	RESET_GLOBAL(g_depthQueue),
	RESET_GLOBAL(g_queueHasRoom),

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

	// render.c
	RESET_GLOBAL(g_drawModeIndex),
	RESET_GLOBAL(g_initDrawModeParam2),
	RESET_GLOBAL(g_showBoundingSpheres),
	RESET_GLOBAL(g_bannerName),
	RESET_GLOBAL(g_bannerBuffer),
	RESET_GLOBAL(g_projectionDirty),
	RESET_GLOBAL(g_displayReady),
	RESET_GLOBAL(g_framePane),
	RESET_GLOBAL(g_hasLightObject),
	RESET_GLOBAL(g_lightFollowsObject),
	RESET_GLOBAL(g_lightObject),
	RESET_GLOBAL(g_skyObject),
	RESET_GLOBAL(g_cockpitObject),
	RESET_GLOBAL(g_drawnPolygonCount),
	RESET_GLOBAL(g_screenPane),
	RESET_GLOBAL(g_gameWindowGeometry),
	RESET_GLOBAL(g_screenHeight),
	RESET_GLOBAL(g_unk0x10176eb0),
	RESET_GLOBAL(g_stretchPending),
	RESET_GLOBAL(g_screenHeightMinus1),
	RESET_GLOBAL(g_screenPixelCount),
	RESET_GLOBAL(g_screenWidth),
	RESET_GLOBAL(g_currentPane),
	RESET_GLOBAL(g_screenWidthMinus1),
	RESET_GLOBAL(g_screenHalfWidth),
	RESET_GLOBAL(g_screenHalfHeight),
	RESET_GLOBAL(g_mainPixelBuffer),

	// resource.c
	RESET_GLOBAL(g_scenarios),
	RESET_GLOBAL(g_unk0x100a860c),
	RESET_GLOBAL(g_nextScenario),
	RESET_GLOBAL(g_thingRecordCount),
	RESET_GLOBAL(g_nextThingRecord),
	RESET_GLOBAL(g_mangleBase),
	RESET_GLOBAL(g_nextMangleBase),
	RESET_GLOBAL(g_playerTeamFormation),
	RESET_GLOBAL(g_otherTeamFormation),
	RESET_GLOBAL(g_lastPlayer),
	RESET_GLOBAL(g_taskFns),
	RESET_GLOBAL(g_missionTables),
	RESET_GLOBAL(g_missionTableCounts),
	RESET_GLOBAL(g_thingRecordIndices),
	RESET_GLOBAL(g_scenarioCount),

	// screenscale.c
	RESET_GLOBAL(g_textMargins),
	RESET_GLOBAL(g_pulseInset),
	RESET_GLOBAL(g_pulseColor),
	RESET_GLOBAL(g_pulseTime),
	RESET_GLOBAL(g_pulseStep),

	// screenshot.c
	RESET_GLOBAL(g_screenshotState),
	RESET_GLOBAL(g_screenshotTarget),

	// setres.c
	RESET_GLOBAL(g_artResolutionSuffixes),
	RESET_GLOBAL(g_artResolutionSizes),
	RESET_GLOBAL(g_pixelAspect),
	RESET_GLOBAL(g_artResolution),

	// settings.c
	RESET_GLOBAL(g_gameCtrlItem),
	RESET_GLOBAL(g_systemsStatusTitle),
	RESET_GLOBAL(g_lightAmplificationItem),
	RESET_GLOBAL(g_imageEnhancementItem),
	RESET_GLOBAL(g_hudItem),
	RESET_GLOBAL(g_autoThermOverrideItem),
	RESET_GLOBAL(g_autoEjectItem),
	RESET_GLOBAL(g_infraredControl),
	RESET_GLOBAL(g_enhancedVisionControl),
	RESET_GLOBAL(g_hudControl),
	RESET_GLOBAL(g_overrideShutdownControl),
	RESET_GLOBAL(g_autoEjectControl),
	RESET_GLOBAL(g_systemsStatusPage),
	RESET_GLOBAL(g_systemsMenuTarget),
	RESET_GLOBAL(g_systemsMenuBackgroundTarget),
	RESET_GLOBAL(g_systemsMenu),
	RESET_GLOBAL(g_systemsMenuPageStack),

	// shape.c
	RESET_GLOBAL(g_newShapeFlags),

	// shapecollision.c
	RESET_GLOBAL(g_rayBoxEntryBehind),

	// shapelists.c
	RESET_GLOBAL(g_sceneShapes),
	RESET_GLOBAL(g_hiddenShapes),
	RESET_GLOBAL(g_sceneShapeHead),
	RESET_GLOBAL(g_hiddenShapeHead),
	RESET_GLOBAL(g_detachedShapeHead),

	// shots.c
	RESET_GLOBAL(g_effectCameraActive),
	RESET_GLOBAL(g_effectCameraEffect),
	RESET_GLOBAL(g_lastLocalMissile),
	RESET_GLOBAL(g_trackedShot),
	RESET_GLOBAL(g_launchEffectPlayer),
	RESET_GLOBAL(g_effectCameraEnabled),
	RESET_GLOBAL(g_lastHitShooter),
	RESET_GLOBAL(g_nukeTimeLeft),
	RESET_GLOBAL(g_nukeMaxRadius),
	RESET_GLOBAL(g_trackedShotView),
	RESET_GLOBAL(g_nukeRadius),
	RESET_GLOBAL(g_nukePosition),
	RESET_GLOBAL(g_savedLightX),
	RESET_GLOBAL(g_savedLightY),
	RESET_GLOBAL(g_savedLightZ),
	RESET_GLOBAL(g_savedDirectionalLight),
	RESET_GLOBAL(g_savedAmbientLight),
	RESET_GLOBAL(g_savedDistanceFade),
	RESET_GLOBAL(g_careerRecord),
	RESET_GLOBAL(g_shots),
	RESET_GLOBAL(g_effects),

	// simmain.c
	RESET_GLOBAL(g_shouldQuit),
	RESET_GLOBAL(g_quitStage),
	RESET_GLOBAL(g_detachedTasks),
	RESET_GLOBAL(g_difficulty),
	RESET_GLOBAL(g_localPlayerId),
	RESET_GLOBAL(g_remoteWaitTime),
	RESET_GLOBAL(g_startOnAutopilot),
	RESET_GLOBAL(g_primaryHeap),
	RESET_GLOBAL(g_gameWindowWidth),
	RESET_GLOBAL(g_gameWindowHeight),
	RESET_GLOBAL(g_windowActive),
	RESET_GLOBAL(g_drawModeReady),
	RESET_GLOBAL(g_simPaused),
	RESET_GLOBAL(g_pauseRequested),
	RESET_GLOBAL(g_mouseOutsideClientWindow),
	RESET_GLOBAL(g_goLaunch),
	RESET_GLOBAL(g_videoDriverChoice),

	// sndunpack.c
	RESET_GLOBAL(g_soundUpsampleBuffer),
	RESET_GLOBAL(g_soundFrame),
	RESET_GLOBAL(g_soundDeltas),

	// soundfx.c
	RESET_GLOBAL(g_temperature),
	RESET_GLOBAL(g_hostileAtmosphere),
	RESET_GLOBAL(g_sampleRates),
	RESET_GLOBAL(g_audioEngine),
	RESET_GLOBAL(g_sampleTag),
	RESET_GLOBAL(g_soundInfo),

	// soundlimits.c
	RESET_GLOBAL(g_soundTable),

	// speech.c
	RESET_GLOBAL(g_slotSpeech),
	RESET_GLOBAL(g_unk0x100a96ec),
	RESET_GLOBAL(g_lancemateSpeech),
	RESET_GLOBAL(g_unk0x100a9774),
	RESET_GLOBAL(g_formationSpeech),
	RESET_GLOBAL(g_unk0x100a97cc),
	RESET_GLOBAL(g_engageSpeech),
	RESET_GLOBAL(g_unk0x100a97f4),
	RESET_GLOBAL(g_cockpitSpeech),
	RESET_GLOBAL(g_damageSpeech),
	RESET_GLOBAL(g_speechQueue),
	RESET_GLOBAL(g_speechSample),
	RESET_GLOBAL(g_speechLocked),
	RESET_GLOBAL(g_speechEntries),

	// staticmem.c
	RESET_GLOBAL(g_staticPoolGroupCount),
	RESET_GLOBAL(g_staticPoolCount),
	RESET_GLOBAL(g_staticPoolGroups),
	RESET_GLOBAL(g_staticPools),

	// statuspanels.c
	RESET_GLOBAL(g_chatRecipient),
	RESET_GLOBAL(g_showObjectives),
	RESET_GLOBAL(g_ticksText),
	RESET_GLOBAL(g_secondsText),
	RESET_GLOBAL(g_chatMessage),

	// supanim.c
	RESET_GLOBAL(g_supAnimBackdrop),
	RESET_GLOBAL(g_supAnimShape),
	RESET_GLOBAL(g_supAnimFrameCount),
	RESET_GLOBAL(g_supAnimBackdropName),
	RESET_GLOBAL(g_supAnimShapeName),
	RESET_GLOBAL(g_supAnimPalette),
	RESET_GLOBAL(g_supAnimY),
	RESET_GLOBAL(g_supAnimX),
	RESET_GLOBAL(g_supAnimTarget),
	RESET_GLOBAL(g_supAnimBuffer),

	// targeting.c
	RESET_GLOBAL(g_navCount),
	RESET_GLOBAL(g_inspectResult),
	RESET_GLOBAL(g_reticleTargeting),
	RESET_GLOBAL(g_satelliteRangeText),
	RESET_GLOBAL(g_satelliteExtraText),
	RESET_GLOBAL(g_satelliteSavedViewport),
	RESET_GLOBAL(g_smallMapRangeText),
	RESET_GLOBAL(g_smallMapExtraText),
	RESET_GLOBAL(g_readoutText),
	RESET_GLOBAL(g_largeMapSavedViewport),
	RESET_GLOBAL(g_largeMapRangeText),
	RESET_GLOBAL(g_bearingText),
	RESET_GLOBAL(g_smallMapSavedViewport),
	RESET_GLOBAL(g_largeMapExtraText),
	RESET_GLOBAL(g_readoutLabel),
	RESET_GLOBAL(g_readout),
	RESET_GLOBAL(g_cockpitReadout),
	RESET_GLOBAL(g_smallMapExtraLabel),
	RESET_GLOBAL(g_smallMapRangeLabel),
	RESET_GLOBAL(g_smallMapBearingLabel),
	RESET_GLOBAL(g_largeMapExtraLabel),
	RESET_GLOBAL(g_largeMapRangeLabel),
	RESET_GLOBAL(g_largeMapBearingLabel),
	RESET_GLOBAL(g_satelliteExtraLabel),
	RESET_GLOBAL(g_satelliteRangeLabel),
	RESET_GLOBAL(g_bearingLabel),
	RESET_GLOBAL(g_metersUnit),
	RESET_GLOBAL(g_kilometersUnit),
	RESET_GLOBAL(g_smallMapColors),
	RESET_GLOBAL(g_largeMapColors),
	RESET_GLOBAL(g_satelliteColors),
	RESET_GLOBAL(g_mapColors),
	RESET_GLOBAL(g_smallMapAnims),
	RESET_GLOBAL(g_largeMapAnims),
	RESET_GLOBAL(g_satelliteAnims),
	RESET_GLOBAL(g_smallMapTransitionFirst),
	RESET_GLOBAL(g_smallMapTransitionSecond),
	RESET_GLOBAL(g_smallMapTransitionRect),
	RESET_GLOBAL(g_mapTransitionState),
	RESET_GLOBAL(g_smallMapTransitionDef),
	RESET_GLOBAL(g_smallMapTransition),
	RESET_GLOBAL(g_largeMapTransitionFirst),
	RESET_GLOBAL(g_largeMapTransitionSecond),
	RESET_GLOBAL(g_largeMapTransitionRect),
	RESET_GLOBAL(g_largeMapTransitionDef),
	RESET_GLOBAL(g_largeMapTransition),
	RESET_GLOBAL(g_satelliteTransitionFirst),
	RESET_GLOBAL(g_satelliteTransitionSecond),
	RESET_GLOBAL(g_satelliteTransitionRect),
	RESET_GLOBAL(g_satelliteTransitionState),
	RESET_GLOBAL(g_satelliteTransitionDef),
	RESET_GLOBAL(g_satelliteTransition),
	RESET_GLOBAL(g_smallMapViewport),
	RESET_GLOBAL(g_smallMapLayout),
	RESET_GLOBAL(g_largeMapViewport),
	RESET_GLOBAL(g_largeMapLayout),
	RESET_GLOBAL(g_satelliteViewport),
	RESET_GLOBAL(g_satelliteLayout),
	RESET_GLOBAL(g_cockpitLayouts),
	RESET_GLOBAL(g_navTable),

	// targetpanel.c
	RESET_GLOBAL(g_targetPanelMode),
	RESET_GLOBAL(g_announceTargetSide),
	RESET_GLOBAL(g_targetFullNameTime),
	RESET_GLOBAL(g_lastPanelTarget),
	RESET_GLOBAL(g_targetPanelStatic),
	RESET_GLOBAL(g_anonymousInstallationName),

	// team.c
	RESET_GLOBAL(g_localStar),
	RESET_GLOBAL(g_formationTemplateCount),
	RESET_GLOBAL(g_formationTemplates),
	RESET_GLOBAL(g_teams),
	RESET_GLOBAL(g_starSides),
	RESET_GLOBAL(g_teamFormations),

	// timedoverlays.c
	RESET_GLOBAL(g_topMessagePane),
	RESET_GLOBAL(g_unk0x100adef4),
	RESET_GLOBAL(g_bottomMessagePane),
	RESET_GLOBAL(g_unk0x100adf0c),
	RESET_GLOBAL(g_bottomMessageText),
	RESET_GLOBAL(g_topMessageText),
	RESET_GLOBAL(g_timedOverlays),

	// view.c
	RESET_GLOBAL(g_lodQuality),
	RESET_GLOBAL(g_viewNear),
	RESET_GLOBAL(g_viewShiftX),
	RESET_GLOBAL(g_viewShiftY),
	RESET_GLOBAL(g_viewFar),
	RESET_GLOBAL(g_viewLeft),
	RESET_GLOBAL(g_viewCenterX),
	RESET_GLOBAL(g_viewHalfHeight),
	RESET_GLOBAL(g_viewHalfWidth),
	RESET_GLOBAL(g_viewBottom),
	RESET_GLOBAL(g_viewProjectScaleX),
	RESET_GLOBAL(g_viewProjectScaleY),
	RESET_GLOBAL(g_viewRight),
	RESET_GLOBAL(g_viewTop),
	RESET_GLOBAL(g_viewLeftScaled),
	RESET_GLOBAL(g_viewCenterY),
	RESET_GLOBAL(g_viewBottomScaled),
	RESET_GLOBAL(g_viewFarPlane),
	RESET_GLOBAL(g_viewProjX0),
	RESET_GLOBAL(g_viewProjX1),
	RESET_GLOBAL(g_viewProjX2),
	RESET_GLOBAL(g_viewProjY0),
	RESET_GLOBAL(g_viewProjY1),
	RESET_GLOBAL(g_viewProjY2),
	RESET_GLOBAL(g_viewProjZ0),
	RESET_GLOBAL(g_viewProjZ1),
	RESET_GLOBAL(g_viewProjZ2),
	RESET_GLOBAL(g_viewProjectScaleX16),
	RESET_GLOBAL(g_viewProjectScaleY16),
	RESET_GLOBAL(g_viewRotX0),
	RESET_GLOBAL(g_viewRotX1),
	RESET_GLOBAL(g_viewRotX2),
	RESET_GLOBAL(g_viewRotY0),
	RESET_GLOBAL(g_viewRotY1),
	RESET_GLOBAL(g_viewRotY2),
	RESET_GLOBAL(g_viewRotZ0),
	RESET_GLOBAL(g_viewRotZ1),
	RESET_GLOBAL(g_viewRotZ2),
	RESET_GLOBAL(g_viewEyeY),
	RESET_GLOBAL(g_viewEyeX),
	RESET_GLOBAL(g_viewEyeZ),
	RESET_GLOBAL(g_viewLightZ),
	RESET_GLOBAL(g_viewLightX),
	RESET_GLOBAL(g_viewLightY),
	RESET_GLOBAL(g_viewTopScaled),
	RESET_GLOBAL(g_viewNearPlane),
	RESET_GLOBAL(g_viewRightScaled),

	// weapondata.c
	RESET_GLOBAL(g_weaponDefs),
	RESET_GLOBAL(g_effectInfo),

	// weapons.c
	RESET_GLOBAL(g_aimedShape),
	RESET_GLOBAL(g_singleWeaponFire),
	RESET_GLOBAL(g_remoteWeaponsFired),
	RESET_GLOBAL(g_localWeaponsFired),

	// world.c
	RESET_GLOBAL(g_loadHudFile),
	RESET_GLOBAL(g_nextShotRecord),
	RESET_GLOBAL(g_nextEffectRecord),
	RESET_GLOBAL(g_musicName),
	RESET_GLOBAL(g_musicResource),

	// wtbshapes.c
	RESET_GLOBAL(g_shapeLoadError),
	RESET_GLOBAL(g_unk0x100ba660),
	RESET_GLOBAL(g_unk0x100ba664),
	RESET_GLOBAL(g_shapeOffsetX),
	RESET_GLOBAL(g_shapeOffsetY),
	RESET_GLOBAL(g_shapeOffsetZ),
	RESET_GLOBAL(g_shapeScaleX),
	RESET_GLOBAL(g_shapeScaleY),
	RESET_GLOBAL(g_shapeScaleZ),
	RESET_GLOBAL(g_shapeFlags),
	RESET_GLOBAL(g_faceIdCount),
	RESET_GLOBAL(g_subShapeCollisionType),
	RESET_GLOBAL(g_unk0x100ba68c),
	RESET_GLOBAL(g_shapeHasObject),
	RESET_GLOBAL(g_faceIds),
	RESET_GLOBAL(g_shapeHasKey),
	RESET_GLOBAL(g_shapeOwnerSet),
	RESET_GLOBAL(g_shapeOwnerKind),
	RESET_GLOBAL(g_shapeOwner),
};

// What the globals held before the first mission
static MechU8* g_resetInitial = NULL;

void ResetSimGlobals(void)
{
	size_t i;
	size_t size;
	MechU8* initial;

	if (g_resetInitial == NULL) {
		size = 0;
		for (i = 0; i < sizeof(g_resetGlobals) / sizeof(g_resetGlobals[0]); i++) {
			size += g_resetGlobals[i].m_size;
		}

		// Not from g_primaryHeap, which each mission creates and destroys.
		g_resetInitial = malloc(size);
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
