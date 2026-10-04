/* The input devices and INPUT.MAP. The unit is not named input.c: the CRT has an input.c (the
   scanf engine) whose line records reccmp confuses with it. */
#include "inputmap.h"

#include "analogbinding.h"
#include "clock.h"
#include "decomp.h"
#include "discretebinding.h"
#include "error.h"
#include "files.h"
#include "gamekeymodifier.h"
#include "gamekeyname.h"
#include "inputaxis.h"
#include "inputcondition.h"
#include "inputdeviceinfo.h"
#include "inputdevicestate.h"
#include "inputdriver.h"
#include "inputsink.h"
#include "joystick.h"
#include "keyboard.h"
#include "log.h"
#include "mouse.h"
#include "mw2log.h"
#include "pointer.h"
#include "refreshmode.h"
#include "simmain.h"
#include "statuspanels.h"
#include "types.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The local player's steering: the outputs of INPUT.MAP's sinks.
// GLOBAL: MW2 0x100b2500
PlayerSteering g_localSteering = {0};

// The outputs of INPUT.MAP's sinks past the steering. pilot_tilt and pilot_pan are the cockpit
// view's look offsets, zoom_factor the view scale (16.16, reset to 1 by FirstEyepoint), and the
// glance sinks look aside while one is held.
// GLOBAL: MW2 0x100b2548
MechS32 g_sinkPilotTilt = 0;

// GLOBAL: MW2 0x100b254c
MechS32 g_sinkPilotPan = 0;

// GLOBAL: MW2 0x100b2550
MechS32 g_sinkEyepointTilt = 0;

// GLOBAL: MW2 0x100b2554
MechS32 g_sinkEyepointPanDelta = 0;

// GLOBAL: MW2 0x100b2558
MechS32 g_sinkEyepointSlideDelta = 0;

// GLOBAL: MW2 0x100b255c
MechS32 g_sinkTrackDistanceDelta = 0;

// GLOBAL: MW2 0x100b2560
MechS32 g_sinkTrackHeightDelta = 0;

// GLOBAL: MW2 0x100b2564
MechS32 g_sinkZoomFactor = 0;

// GLOBAL: MW2 0x100b2568
MechS8 g_sinkPilotTiltPlus = 0;

// GLOBAL: MW2 0x100b2569
MechS8 g_sinkPilotTiltMinus = 0;

// GLOBAL: MW2 0x100b256a
MechS8 g_sinkPilotTiltReset = 0;

// GLOBAL: MW2 0x100b256b
MechS8 g_sinkPilotPanPlus = 0;

// GLOBAL: MW2 0x100b256c
MechS8 g_sinkPilotPanMinus = 0;

// GLOBAL: MW2 0x100b256d
MechS8 g_sinkPilotPanReset = 0;

// GLOBAL: MW2 0x100b256e
MechS8 g_sinkGlanceLeft = 0;

// GLOBAL: MW2 0x100b256f
MechS8 g_sinkGlanceRight = 0;

// GLOBAL: MW2 0x100b2570
MechS8 g_sinkGlanceUp = 0;

// GLOBAL: MW2 0x100b2571
MechS8 g_sinkGlanceDown = 0;

// GLOBAL: MW2 0x100b2572
MechS8 g_sinkEyepointTiltPlus = 0;

// GLOBAL: MW2 0x100b2573
MechS8 g_sinkEyepointTiltMinus = 0;

// GLOBAL: MW2 0x100b2574
MechS8 g_sinkEyepointTiltReset = 0;

// GLOBAL: MW2 0x100b2575
MechS8 g_sinkEyepointPanPlus = 0;

// GLOBAL: MW2 0x100b2576
MechS8 g_sinkEyepointPanMinus = 0;

// GLOBAL: MW2 0x100b2577
MechS8 g_sinkEyepointPanReset = 0;

// GLOBAL: MW2 0x100b2578
MechS8 g_sinkEyepointSlidePlus = 0;

// GLOBAL: MW2 0x100b2579
MechS8 g_sinkEyepointSlideMinus = 0;

// GLOBAL: MW2 0x100b257a
MechS8 g_sinkTrackDistancePlus = 0;

// GLOBAL: MW2 0x100b257b
MechS8 g_sinkTrackDistanceMinus = 0;

// GLOBAL: MW2 0x100b257c
MechS8 g_sinkTrackHeightPlus = 0;

// GLOBAL: MW2 0x100b257d
MechS8 g_sinkTrackHeightMinus = 0;

// GLOBAL: MW2 0x100b257e
MechS8 g_sinkZoomFactorPlus = 0;

// GLOBAL: MW2 0x100b257f
MechS8 g_sinkZoomFactorMinus = 0;

// GLOBAL: MW2 0x100b2580
MechS8 g_sinkZoomFactorReset = 0;

// GLOBAL: MW2 0x100b2588
MechS32 g_sinkMenuItem = 0;

// GLOBAL: MW2 0x100b258c
MechS32 g_sinkMenuValue = 0;

// GLOBAL: MW2 0x100b2592
MechS8 g_sinkMenuItemReset = 0;

// GLOBAL: MW2 0x100b2595
MechS8 g_sinkMenuValueReset = 0;

// GLOBAL: MW2 0x100b2596
MechS8 g_sinkMenuEnter = 0;

// GLOBAL: MW2 0x100b2597
MechS8 g_sinkMenuAbort = 0;

// GLOBAL: MW2 0x100b2598
MechS32 g_analogBindingCount = 0;

// GLOBAL: MW2 0x100b259c
MechS32 g_inputAxisCount = 0;

// GLOBAL: MW2 0x100b25a0
MechS32 g_discreteBindingCount = 0;

// GLOBAL: MW2 0x100b25a4
MechS32 g_gameplayInputEnabled = 1;

// GLOBAL: MW2 0x100b25a8
MechS32 g_keyboardDeviceIndex = 0;

// The line of INPUT.MAP being parsed.
// GLOBAL: MW2 0x100b25ac
MechS32 g_inputMapLine = 0;

// GLOBAL: MW2 0x100b25b0
AnalogBinding g_analogBindings[50] = {0};

// GLOBAL: MW2 0x100b3eb0
InputAxis g_inputAxes[50] = {0};

// GLOBAL: MW2 0x100b4878
DiscreteBinding g_discreteBindings[100] = {0};

// GLOBAL: MW2 0x100b7370
MechS32 g_inputDeviceCount = 0;

// The input driver classes, in the order RegisterInputDevice asks them for a device.
// GLOBAL: MW2 0x100b7378
InputDriverModule* g_inputDriverClasses[3] = {&g_keyboardDriver, &g_mouseDriver, &g_joystickDriver};

// The sinks INPUT.MAP can bind.
// GLOBAL: MW2 0x100b7388
InputSink g_inputSinks[] = {
	{"torso_tilt", &g_localSteering.m_torsoTilt, 0, 0, -60, 60, 4, 0},
	{"torso_tilt_plus", &g_localSteering.m_torsoTiltPlus, 1, 0, 0, 0, 0, 0},
	{"torso_tilt_minus", &g_localSteering.m_torsoTiltMinus, 1, 0, 0, 0, 0, 0},
	{"torso_tilt_reset", &g_localSteering.m_torsoTiltReset, 1, 0, 0, 0, 0, 0},
	{"torso_pan", &g_localSteering.m_torsoPan, 0, 0, -140, 140, 3, 0},
	{"torso_pan_plus", &g_localSteering.m_torsoPanPlus, 1, 0, 0, 0, 0, 0},
	{"torso_pan_minus", &g_localSteering.m_torsoPanMinus, 1, 0, 0, 0, 0, 0},
	{"torso_pan_set", &g_localSteering.m_torsoPanSet, 1, 0, 0, 0, 0, 0},
	{"torso_pan_reset", &g_localSteering.m_torsoPanReset, 1, 0, 0, 0, 0, 0},
	{"throttle", &g_localSteering.m_throttle, 0, 0, -120, 1082, 6, 16},
	{"legs_pan_delta", &g_localSteering.m_legsPanDelta, 0, 0, -1024, 1024, 7, 0},
	{"jumpjet_enabled", &g_localSteering.m_jumpJetEnabled, 1, 0, 0, 0, 0, 0},
	{"jumpjet_fire_left", &g_localSteering.m_jumpJetFireLeft, 1, 0, 0, 0, 0, 0},
	{"jumpjet_fire_right", &g_localSteering.m_jumpJetFireRight, 1, 0, 0, 0, 0, 0},
	{"jumpjet_fire_forward", &g_localSteering.m_jumpJetFireForward, 1, 0, 0, 0, 0, 0},
	{"jumpjet_fire_backward", &g_localSteering.m_jumpJetFireBackward, 1, 0, 0, 0, 0, 0},
	{"throttle_plus", &g_localSteering.m_throttlePlus, 1, 0, 0, 0, 0, 0},
	{"throttle_minus", &g_localSteering.m_throttleMinus, 1, 0, 0, 0, 0, 0},
	{"throttle_set", &g_localSteering.m_throttleSet, 1, 0, 0, 0, 0, 0},
	{"weapon_fire", &g_localSteering.m_weaponFire, 1, 0, 0, 0, 0, 0},
	{"weapon_cycle", &g_localSteering.m_weaponCycle, 2, 0, 0, 0, 0, 0},
	{"weapon_fire_group", &g_localSteering.m_weaponFireGroup, 1, 0, 0, 0, 0, 0},
	{"weapon_fire_group_1", &g_localSteering.m_weaponFireGroup1, 1, 0, 0, 0, 0, 0},
	{"weapon_fire_group_2", &g_localSteering.m_weaponFireGroup2, 1, 0, 0, 0, 0, 0},
	{"weapon_fire_group_3", &g_localSteering.m_weaponFireGroup3, 1, 0, 0, 0, 0, 0},
	{"weapon_cycle_group", &g_localSteering.m_weaponCycleGroup, 2, 0, 0, 0, 0, 0},
	{"toggle_group_fire", &g_localSteering.m_toggleGroupFire, 2, 0, 0, 0, 0, 0},
	{"legs_pan_minus", &g_localSteering.m_legsPanMinus, 1, 0, 0, 0, 0, 0},
	{"legs_pan_plus", &g_localSteering.m_legsPanPlus, 1, 0, 0, 0, 0, 0},
	{"advance_nav", &g_localSteering.m_advanceNav, 2, 0, 0, 0, 0, 0},
	{"previous_nav", &g_localSteering.m_previousNav, 2, 0, 0, 0, 0, 0},
	{"reset_nav", &g_localSteering.m_resetNav, 2, 0, 0, 0, 0, 0},
	{"advance_target", &g_localSteering.m_advanceTarget, 2, 0, 0, 0, 0, 0},
	{"previous_target", &g_localSteering.m_previousTarget, 2, 0, 0, 0, 0, 0},
	{"reset_target", &g_localSteering.m_resetTarget, 2, 0, 0, 0, 0, 0},
	{"target_reticle", &g_localSteering.m_targetReticle, 2, 0, 0, 0, 0, 0},
	{"target_friendly", &g_localSteering.m_targetFriendly, 2, 0, 0, 0, 0, 0},
	{"nearest_enemy", &g_localSteering.m_nearestEnemy, 2, 0, 0, 0, 0, 0},
	{"target_last_shot", &g_localSteering.m_targetLastShot, 2, 0, 0, 0, 0, 0},
	{"inspect_target", &g_localSteering.m_inspectTarget, 2, 0, 0, 0, 0, 0},
	{"advance_gamepiece", &g_localSteering.m_advanceGamepiece, 2, 0, 0, 0, 0, 0},
	{"previous_gamepiece", &g_localSteering.m_previousGamepiece, 2, 0, 0, 0, 0, 0},
	{"reset_gamepiece", &g_localSteering.m_resetGamepiece, 2, 0, 0, 0, 0, 0},
	{"advance_gamething", &g_localSteering.m_advanceGamething, 2, 0, 0, 0, 0, 0},
	{"previous_gamething", &g_localSteering.m_previousGamething, 2, 0, 0, 0, 0, 0},
	{"reset_gamething", &g_localSteering.m_resetGamething, 2, 0, 0, 0, 0, 0},
	{"self_destruct", &g_localSteering.m_selfDestruct, 2, 0, 0, 0, 0, 0},
	{"autopilot", &g_localSteering.m_autopilot, 2, 0, 0, 0, 0, 0},
	{"pilot_tilt", &g_sinkPilotTilt, 0, 0, -60, 60, 5, 0},
	{"pilot_tilt_plus", &g_sinkPilotTiltPlus, 1, 0, 0, 0, 0, 0},
	{"pilot_tilt_minus", &g_sinkPilotTiltMinus, 1, 0, 0, 0, 0, 0},
	{"pilot_tilt_reset", &g_sinkPilotTiltReset, 1, 0, 0, 0, 0, 0},
	{"pilot_pan", &g_sinkPilotPan, 0, 0, -70, 70, 5, 0},
	{"pilot_pan_plus", &g_sinkPilotPanPlus, 1, 0, 0, 0, 0, 0},
	{"pilot_pan_minus", &g_sinkPilotPanMinus, 1, 0, 0, 0, 0, 0},
	{"pilot_pan_reset", &g_sinkPilotPanReset, 1, 0, 0, 0, 0, 0},
	{"glance_left", &g_sinkGlanceLeft, 1, 0, 0, 0, 0, 0},
	{"glance_right", &g_sinkGlanceRight, 1, 0, 0, 0, 0, 0},
	{"glance_up", &g_sinkGlanceUp, 1, 0, 0, 0, 0, 0},
	{"glance_down", &g_sinkGlanceDown, 1, 0, 0, 0, 0, 0},
	{"eyepoint_tilt", &g_sinkEyepointTilt, 0, 0, -16, 16, 4, 0},
	{"eyepoint_tilt_plus", &g_sinkEyepointTiltPlus, 1, 0, 0, 0, 0, 0},
	{"eyepoint_tilt_minus", &g_sinkEyepointTiltMinus, 1, 0, 0, 0, 0, 0},
	{"eyepoint_tilt_reset", &g_sinkEyepointTiltReset, 1, 0, 0, 0, 0, 0},
	{"eyepoint_pan_delta", &g_sinkEyepointPanDelta, 0, 0, -72, 72, 4, 0},
	{"eyepoint_pan_plus", &g_sinkEyepointPanPlus, 1, 0, 0, 0, 0, 0},
	{"eyepoint_pan_minus", &g_sinkEyepointPanMinus, 1, 0, 0, 0, 0, 0},
	{"eyepoint_pan_reset", &g_sinkEyepointPanReset, 1, 0, 0, 0, 0, 0},
	{"eyepoint_slide_delta", &g_sinkEyepointSlideDelta, 0, 0, -3600, 3600, 6, 0},
	{"eyepoint_slide_plus", &g_sinkEyepointSlidePlus, 1, 0, 0, 0, 0, 0},
	{"eyepoint_slide_minus", &g_sinkEyepointSlideMinus, 1, 0, 0, 0, 0, 0},
	{"track_distance_delta", &g_sinkTrackDistanceDelta, 0, 0, -3600, 3600, 6, 0},
	{"track_distance_plus", &g_sinkTrackDistancePlus, 1, 0, 0, 0, 0, 0},
	{"track_distance_minus", &g_sinkTrackDistanceMinus, 1, 0, 0, 0, 0, 0},
	{"track_height_delta", &g_sinkTrackHeightDelta, 0, 0, -3600, 3600, 6, 0},
	{"track_height_plus", &g_sinkTrackHeightPlus, 1, 0, 0, 0, 0, 0},
	{"track_height_minus", &g_sinkTrackHeightMinus, 1, 0, 0, 0, 0, 0},
	{"zoom_factor", &g_sinkZoomFactor, 0, 1, 1, 4, 6, 0},
	{"zoom_factor_plus", &g_sinkZoomFactorPlus, 1, 0, 0, 0, 0, 0},
	{"zoom_factor_minus", &g_sinkZoomFactorMinus, 1, 0, 0, 0, 0, 0},
	{"zoom_factor_reset", &g_sinkZoomFactorReset, 1, 0, 0, 0, 0, 0},
	{"menu_item", &g_sinkMenuItem, 0, 0, -1, 1, 4, 0},
	{"menu_item_reset", &g_sinkMenuItemReset, 1, 0, 0, 0, 0, 0},
	{"menu_value", &g_sinkMenuValue, 0, 0, -1, 1, 4, 0},
	{"menu_value_reset", &g_sinkMenuValueReset, 1, 0, 0, 0, 0, 0},
	{"menu_enter", &g_sinkMenuEnter, 2, 0, 0, 0, 0, 0},
	{"menu_abort", &g_sinkMenuAbort, 2, 0, 0, 0, 0, 0},
};

// The GAMEKEY.MAP action names.
// GLOBAL: MW2 0x100b7e68
GameKeyName g_gameKeyNames[] = {
	{"RESET_INPUTS", 1},
	{"MFD_CYCLE", 2},
	{"TOGGLE_HTAL", 3},
	{"TOGGLE_REAR_VIEW", 4},
	{"TOGGLE_DOWN_VIEW", 5},
	{"TOGGLE_WEAPON_DISPLAY", 6},
	{"TOGGLE_DAMAGE_DISPLAY", 7},
	{"TOGGLE_TARGET_DISPLAY", 8},
	{"COCKPIT_VIEW", 9},
	{"FREEEYE_VIEW", 10},
	{"TRACK_VIEW", 11},
	{"TRACK_NEXT", 12},
	{"TRACK_PREV", 13},
	{"ORDINANCE_VIEW", 14},
	{"JUMP_DEBUGGER", 15},
	{"REQUEST_ORDINANCE", 16},
	{"FEET_TO_TORSO", 17},
	{"INSPECT_TARGET", 18},
	{"TOGGLE_HUD", 19},
	{"JETTISON_AMMO", 20},
	{"COCKPIT_ZOOM_IN", 21},
	{"COCKPIT_ZOOM_OUT", 22},
	{"COCKPIT_RESET_ZOOM", 23},
	{"DECREASE_THROTTLE", 24},
	{"INCREASE_THROTTLE", 25},
	{"THROTTLE_STOP", 26},
	{"THROTTLE_2", 27},
	{"THROTTLE_3", 28},
	{"THROTTLE_4", 29},
	{"THROTTLE_5", 30},
	{"THROTTLE_6", 31},
	{"THROTTLE_7", 32},
	{"THROTTLE_8", 33},
	{"THROTTLE_9", 34},
	{"THROTTLE_FULL", 35},
	{"NEXT_GAMETHING", 36},
	{"PREV_GAMETHING", 37},
	{"RESET_GAMETHING", 38},
	{"RESET_TARGETTING", 39},
	{"NEXT_NAVPOINT", 40},
	{"PREV_NAVPOINT", 41},
	{"RESET_NAVPOINT", 42},
	{"NEXT_GAMEPIECE", 43},
	{"PREV_GAMEPIECE", 44},
	{"RESET_GAMEPIECE", 45},
	{"NEXT_RADAR_MODE", 46},
	{"RADAR_ZOOM_IN", 47},
	{"RADAR_ZOOM_OUT", 48},
	{"RADAR_RESET_ZOOM", 49},
	{"RADAR_MAP_TOGGLE", 50},
	{"MAIN_MENU", 51},
	{"USER_MENU", 52},
	{"ALL_PT_MENU", 53},
	{"PT_2_MENU", 54},
	{"PT_3_MENU", 55},
	{"HELP_MENU", 58},
	{"EJECT", 59},
	{"TOGGLE_AUTOEJECT", 60},
	{"STARTUP_MECH", 61},
	{"SHUTDOWN_MECH", 62},
	{"REVERSE_DIRECTION", 63},
	{"OVERRIDE_SHUTDOWN", 64},
	{"INFRARED", 166},
	{"ENHANCED_VISION", 167},
	{"DISPLAY_OBJECTIVES", 65},
	{"STARMATE_MENU", 66},
	{"STARMATE_ONE_MENU", 67},
	{"STARMATE_TWO_MENU", 68},
	{"STARMATE_THREE_MENU", 69},
	{"STARMATE_FOUR_MENU", 70},
	{"STARMATE_FIVE_MENU", 71},
	{"STARMATE_SIX_MENU", 72},
	{"STARMATE_SEVEN_MENU", 73},
	{"TOGGLE_AUTOPILOT", 74},
	{"TOGGLE_GROUP_FIRE", 75},
	{"NEXT_TARGET", 76},
	{"PREV_TARGET", 77},
	{"RESET_TARGET", 78},
	{"TARGET_NEAREST_ENEMY", 80},
	{"TARGET_FRIENDLY", 81},
	{"TARGET_AT_RETICLE", 82},
	{"TARGET_LAST_SHOT", 83},
	{"NEXT_OBJECTIVE", 84},
	{"PUNCH_IN_AUTO_HDG", 85},
	{"TOGGLE_MASC", 86},
	{"SELF_DESTRUCT", 87},
	{"PAUSE_GAME", 88},
	{"EXIT_SIM", 89},
	{"ABOUT_BANNER", 90},
	{"DUMP_GIF", 91},
	{"DEBUG_CRITICAL_HITS_1", 92},
	{"DEBUG_CRITICAL_HITS_2", 93},
	{"DEBUG_CRITICAL_HITS_3", 94},
	{"DEBUG_CRITICAL_HITS_4", 95},
	{"DEBUG_CRITICAL_HITS_5", 96},
	{"DEBUG_CRITICAL_HITS_6", 97},
	{"DEBUG_CRITICAL_HITS_7", 98},
	{"DEBUG_CRITICAL_HITS_8", 99},
	{"DEBUG_CRITICAL_HITS_9", 100},
	{"DEBUG_CRITICAL_HITS_0", 101},
	{"SELECT_HEAD", 102},
	{"SELECT_RIGHT_TORSO", 103},
	{"SELECT_CENTER_TORSO", 104},
	{"SELECT_LEFT_TORSO", 105},
	{"SELECT_RIGHT_ARM", 106},
	{"SELECT_LEFT_ARM", 107},
	{"SELECT_RIGHT_LEG", 108},
	{"SELECT_LEFT_LEG", 109},
	{"AI_0_MONO_OBJECTIVES", 110},
	{"AI_1_MONO_OBJECTIVES", 111},
	{"AI_2_MONO_OBJECTIVES", 112},
	{"AI_3_MONO_OBJECTIVES", 113},
	{"AI_4_MONO_OBJECTIVES", 114},
	{"AI_5_MONO_OBJECTIVES", 115},
	{"AI_6_MONO_OBJECTIVES", 116},
	{"AI_7_MONO_OBJECTIVES", 117},
	{"AI_8_MONO_OBJECTIVES", 118},
	{"AI_9_MONO_OBJECTIVES", 119},
	{"OBJECTIVES_OFF", 120},
	{"NEXT_MONO_OBJECTIVE", 121},
	{"PREV_MONO_OBJECTIVE", 122},
	{"FAIL_MONO_OBJECTIVE", 123},
	{"SUCCEED_MONO_OBJECTIVE", 124},
	{"TOGGLE_GP_TEXT_MAPS", 125},
	{"TOGGLE_GT_TEXT_MAPS", 126},
	{"TOGGLE_GS_TEXT_MAPS", 127},
	{"TOGGLE_TERRAIN_TEXT_MAPS", 128},
	{"GO_TURBO_CONTROL", 129},
	{"TOGGLE_MEM_INFO", 130},
	{"TOGGLE_CACHE_INFO", 131},
	{"COLLISION_WIREFRAME", 132},
	{"TOGGLE_FRAMERATE_MAIN", 133},
	{"TOGGLE_SCENE_INFO_MAIN", 135},
	{"TOGGLE_EYE_POSITION_MAIN", 136},
	{"TOGGLE_VULNERABILITY", 137},
	{"TOGGLE_AMMO_LIMIT", 138},
	{"TOGGLE_SPLASH_DAMAGE", 139},
	{"TOGGLE_COLLISION_DAMAGE", 140},
	{"TOGGLE_HEAT_TRACKING", 141},
	{"TOGGLE_LOD_QUALITY", 142},
	{"DUMP_CACHE_ONCE", 143},
	{"DUMP_CACHE_REPEAT", 144},
	{"TOGGLE_RADAR_DAMAGE", 134},
	{"TOGGLE_FRAMERATE", 145},
	{"CRACK_MODE", 146},
	{"TOGGLE_EYE_POSITION", 147},
	{"TOGGLE_SCENE_INFO", 148},
	{"TOGGLE_WIREFRAME", 149},
	{"TOGGLE_PALETTE", 150},
	{"ADD_WEAPON_TO_GROUP_1", 151},
	{"ADD_WEAPON_TO_GROUP_2", 152},
	{"ADD_WEAPON_TO_GROUP_3", 153},
	{"NEXT_WEAPON_GROUP", 154},
	{"FIRE_WEAPON_GROUP", 155},
	{"CREATE_NAVPOINT_TEST", 159},
	{"DELETE_NAVPOINT_TEST", 160},
	{"TOGGLE_SAVE_FRAMES", 161},
	{"PLAY_CURRENT_MIDI", 162},
	{"PLAY_NEXT_MIDI", 163},
	{"HACK_JETS", 164},
	{"KILL_TARGETTED_MECH", 165},
};

// The game key action of each key code (0: none), loaded from GAMEKEY.MAP.
// GLOBAL: MW2 0x100b8190
MechU8 g_gameKeyByKeyCode[0x800] = {0};

// The key names GAMEKEY.MAP can use in a key sequence, and their key codes (or modifier bits).
// GLOBAL: MW2 0x100b8990
GameKeyModifier g_gameKeyModifiers[] = {
	{"ALT", 0x400},       {"CTRL", 0x100},     {"SHIFT", 0x200}, {"F1", 0xb1},         {"F2", 0xb2},
	{"F3", 0xb3},         {"F4", 0xb4},        {"F5", 0xb5},     {"F6", 0xb6},         {"F7", 0xb7},
	{"F8", 0xb8},         {"F9", 0xb9},        {"F10", 0xba},    {"F11", 0xbb},        {"F12", 0xbc},
	{"TAB", 0x9},         {"INSERT", 0xc0},    {"ESC", 0x1b},    {"BSP", 0x8},         {"ENTER", 0xd},
	{"PAUSE", 0x1ff},     {"POUND", 0x23},     {"PLUS", 0x2b},   {"GREY SLASH", 0xca}, {"GREY ASTERICK", 0xcb},
	{"GREY MINUS", 0xcc}, {"GREY PLUS", 0xcd},
};

// The text of ReportInputDeviceError's message box.
// GLOBAL: MW2 0x100bf1d0
MechChar g_inputErrorText[0x400];

// GLOBAL: MW2 0x100bf5d0
MechS32 g_inputDevicePresent[5];

// GLOBAL: MW2 0x100bf5e8
InputDeviceInfo g_inputDeviceInfos[5];

// What each device's last poll reported.
// GLOBAL: MW2 0x100bf830
InputDeviceState g_inputDeviceStates[5];

// The INPUT.MAP name of each device.
// GLOBAL: MW2 0x100bf9c0
MechChar g_inputDeviceNames[5][0x28];

// GLOBAL: MW2 0x100bfa88
InputDriverModule* g_inputDrivers[5];

// Finds the device INPUT.MAP calls p_name among the drivers' devices and opens it. Returns its
// index, or -1 if it can't be opened; a missing device still takes an index.
// Stack-slot permutation: index, driver, i and count.
// FUNCTION: MW2 0x10079340
MechS32 RegisterInputDevice(MechChar* p_name)
{
	MechS32 index;
	MechS32 driver;
	MechS32 i;
	MechS32 count;

	for (driver = 0; driver < 3; driver++) {
		if (g_inputDriverClasses[driver]) {
			count = g_inputDriverClasses[driver]->m_getDeviceCount();
			if (count) {
				for (i = 0; i < count; i++) {
					index = g_inputDeviceCount;
					if (g_inputDeviceCount >= 5) {
						Error(0x11, "Too many input devices specified", 0);
						return -1;
					}

					if (!g_inputDriverClasses[driver]->m_fillDeviceInfo(i, &g_inputDeviceInfos[index])) {
						if (!_strcmpi(g_inputDeviceInfos[index].m_shortName, p_name)) {
							strcpy(g_inputDeviceNames[index], p_name);
							g_inputDrivers[index] = g_inputDriverClasses[driver];
							g_inputDeviceCount++;
							if (g_inputDrivers[index]->m_openDevice(&g_inputDeviceInfos[index])) {
								ReportInputDeviceError(0x70, NULL, g_inputDeviceInfos[index].m_displayName);
								g_inputDevicePresent[index] = FALSE;
								return -1;
							}
							else {
								g_inputDevicePresent[index] = TRUE;
								return index;
							}
						}
						else {
							g_inputDriverClasses[driver]->m_closeDevice(&g_inputDeviceInfos[index]);
						}
					}
				}
			}
		}
	}

	index = g_inputDeviceCount;
	g_inputDeviceCount++;
	strcpy(g_inputDeviceNames[index], p_name);
	g_inputDevicePresent[index] = FALSE;
	ReportInputDeviceError(0x6e, NULL, p_name);
	return -1;
}

// Returns the index of the device INPUT.MAP calls p_name, registering it the first time, or -1
// if it isn't there.
// Stack-slot permutation: index and i.
// FUNCTION: MW2 0x1007959c
MechS32 FindInputDevice(MechChar* p_name)
{
	MechS32 index;
	MechS32 i;

	for (i = 0; i < g_inputDeviceCount; i++) {
		if (!_strcmpi(g_inputDeviceNames[i], p_name)) {
			return g_inputDevicePresent[i] ? i : -1;
		}
	}

	index = RegisterInputDevice(p_name);
	if (!_strcmpi(p_name, "keyboard")) {
		if (index != -1) {
			g_keyboardDeviceIndex = index;
		}
		else {
			Error(0x11, "\nKeyboard initialization error", 0);
		}
	}

	return index;
}

// Returns the axis of device p_device that INPUT.MAP names p_name, a number or a short name, or
// -1.
// FUNCTION: MW2 0x1007966a
MechS32 FindInputAxis(MechS32 p_device, MechChar* p_name)
{
	MechS32 i;

	if (isdigit(*p_name)) {
		return atoi(p_name);
	}

	if (p_device == -1) {
		return -1;
	}

	if (!g_inputDeviceInfos[p_device].m_axisShortNames) {
		Error(0x11, "No input channel \"%s\". Line %d", p_name, g_inputMapLine, 0);
		return -1;
	}

	for (i = 0; i < g_inputDeviceInfos[p_device].m_axisCount; i++) {
		if (g_inputDeviceInfos[p_device].m_axisShortNames[i] &&
			!_strcmpi(g_inputDeviceInfos[p_device].m_axisShortNames[i], p_name)) {
			return i;
		}
	}

	ReportInputDeviceError(0x6f, p_name, g_inputDeviceInfos[p_device].m_displayName);
	return -1;
}

// Returns the button of device p_device that INPUT.MAP names p_name, a number or a short name,
// or -1.
// FUNCTION: MW2 0x100797de
MechS32 FindInputButton(MechS32 p_device, MechChar* p_name)
{
	MechS32 i;

	if (isdigit(*p_name)) {
		return atoi(p_name);
	}

	if (p_device == -1) {
		return -1;
	}

	if (!g_inputDeviceInfos[p_device].m_buttonShortNames) {
		Error(0x11, "No input channel \"%s\". Line %d", p_name, g_inputMapLine, 0);
		return -1;
	}

	for (i = 0; i < g_inputDeviceInfos[p_device].m_buttonCount; i++) {
		if (g_inputDeviceInfos[p_device].m_buttonShortNames[i] &&
			!_strcmpi(g_inputDeviceInfos[p_device].m_buttonShortNames[i], p_name)) {
			return i;
		}
	}

	ReportInputDeviceError(0x6f, p_name, g_inputDeviceInfos[p_device].m_displayName);
	return -1;
}

// Returns the sink INPUT.MAP calls p_name, or NULL.
// FUNCTION: MW2 0x10079952
InputSink* FindInputSink(MechChar* p_name)
{
	MechS32 i;

	for (i = 0; i < sizeof(g_inputSinks) / sizeof(g_inputSinks[0]); i++) {
		if (!_strcmpi(g_inputSinks[i].m_name, p_name)) {
			return &g_inputSinks[i];
		}
	}

	return NULL;
}

// Reads the next line of INPUT.MAP that isn't blank once its comment is cut off. Returns 0 at
// the end of the file.
// FUNCTION: MW2 0x100799b7
MechS32 ReadInputMapLine(MechChar* p_buffer, MechS32 p_size, FILE* p_file)
{
	MechChar* p;
	MechS32 found;

	found = FALSE;
	for (;;) {
		if (!fgets(p_buffer, p_size, p_file)) {
			return 0;
		}

		g_inputMapLine++;
		for (p = p_buffer; *p; p++) {
			if (*p == '\n' || *p == '#') {
				*p = '\0';
				break;
			}
			else if (!isspace(*p)) {
				found = TRUE;
			}
		}

		if (found) {
			return 1;
		}
	}
}

// Returns the axis state of the axis sink p_sink, setting one up the first time: its range and
// rest position, and the outputs of its _plus, _minus, _set and _reset sinks. A _delta sink's
// keys move its position directly.
// The loop compares i and g_inputAxisCount in the other operand order, and i, suffix, sink and
// name are a stack-slot permutation.
// FUNCTION: MW2 0x10079aac
InputAxis* GetOrCreateInputAxis(InputSink* p_sink)
{
	MechChar* suffix;
	InputSink* sink;
	MechChar name[40];
	MechS32 i;

	for (i = 0; i < g_inputAxisCount; i++) {
		if (g_inputAxes[i].m_output == p_sink->m_output) {
			return &g_inputAxes[i];
		}
	}

	i = g_inputAxisCount;
	g_inputAxisCount++;
	if (g_inputAxisCount > 50) {
		Error(0x11, "Too many analog sinks: line %d", g_inputMapLine, 0);
		return NULL;
	}

	if (strstr(p_sink->m_name, "_delta")) {
		g_inputAxes[i].m_rate = &g_inputAxes[i].m_position;
	}
	else {
		g_inputAxes[i].m_rate = &g_inputAxes[i].m_ownRate;
	}

	g_inputAxes[i].m_output = p_sink->m_output;
	g_inputAxes[i].m_range = p_sink->m_max - p_sink->m_min;
	g_inputAxes[i].m_minFixed = p_sink->m_min << 16;
	g_inputAxes[i].m_outputShift = p_sink->m_outputShift;
	g_inputAxes[i].m_rest = p_sink->m_rest << 16;
	g_inputAxes[i].m_rest -= g_inputAxes[i].m_minFixed;
	g_inputAxes[i].m_rest <<= 1;
	g_inputAxes[i].m_rest /= g_inputAxes[i].m_range;
	g_inputAxes[i].m_rest -= 0x10000;
	g_inputAxes[i].m_rampShift = p_sink->m_rampShift;
	g_inputAxes[i].m_position = g_inputAxes[i].m_rest;

	strcpy(name, p_sink->m_name);
	suffix = strstr(name, "_delta");
	if (suffix) {
		*suffix = '\0';
	}
	strcat(name, "_plus");
	sink = FindInputSink(name);
	if (sink) {
		g_inputAxes[i].m_plusHeld = sink->m_output;
	}
	else {
		g_inputAxes[i].m_plusHeld = NULL;
	}

	strcpy(name, p_sink->m_name);
	suffix = strstr(name, "_delta");
	if (suffix) {
		*suffix = '\0';
	}
	strcat(name, "_minus");
	sink = FindInputSink(name);
	if (sink) {
		g_inputAxes[i].m_minusHeld = sink->m_output;
	}
	else {
		g_inputAxes[i].m_minusHeld = NULL;
	}

	strcpy(name, p_sink->m_name);
	strcat(name, "_set");
	sink = FindInputSink(name);
	if (sink) {
		g_inputAxes[i].m_setHeld = sink->m_output;
	}
	else {
		g_inputAxes[i].m_setHeld = NULL;
	}

	strcpy(name, p_sink->m_name);
	strcat(name, "_reset");
	sink = FindInputSink(name);
	if (sink) {
		g_inputAxes[i].m_resetHeld = sink->m_output;
	}
	else {
		g_inputAxes[i].m_resetHeld = NULL;
	}

	return &g_inputAxes[i];
}

// Binds channel p_channel of device p_deviceName (NULL: none, for the axis's keys) to the axis
// sink p_sink. A binding of the axis without a channel takes the channel. Returns the binding's
// index, or -1.
// Stack-slot permutation: index and i.
// FUNCTION: MW2 0x10079f71
MechS32 AddAnalogBinding(InputSink* p_sink, MechChar* p_deviceName, MechS32 p_channel)
{
	MechS32 device;
	MechS32 index;
	MechS32 i;
	MechS32 found;

	for (i = 0; i < g_analogBindingCount; i++) {
		if (g_analogBindings[i].m_axis && g_analogBindings[i].m_axis->m_output == p_sink->m_output) {
			if (!p_deviceName) {
				return i;
			}

			if (!g_analogBindings[i].m_channelValue) {
				found = FindInputDevice(p_deviceName);
				g_analogBindings[i].m_channelValue = &g_inputDeviceStates[found].m_axes[p_channel];
				g_analogBindings[i].m_device = found;
				g_analogBindings[i].m_channel = p_channel;
				return i;
			}
		}
	}

	index = g_analogBindingCount;
	g_analogBindingCount++;
	if (g_analogBindingCount > 50) {
		Error(0x11, "Too many analog controls: line %d", g_inputMapLine, 0);
		return -1;
	}

	if (p_deviceName) {
		device = FindInputDevice(p_deviceName);
	}
	else {
		device = -1;
	}

	g_analogBindings[index].m_device = device;
	g_analogBindings[index].m_channel = p_channel;
	g_analogBindings[index].m_axis = GetOrCreateInputAxis(p_sink);
	if (device >= 0) {
		g_analogBindings[index].m_channelValue = &g_inputDeviceStates[device].m_axes[p_channel];
	}
	else {
		g_analogBindings[index].m_channelValue = NULL;
	}

	if (!strncmp(p_sink->m_name, "menu_", 5)) {
		g_analogBindings[index].m_menuOnly = TRUE;
	}
	else {
		g_analogBindings[index].m_menuOnly = FALSE;
	}

	return index;
}

// Reads a binding's conditions, "+ device button" or "- device button" a line, up to the closing
// brace. Returns 0 on an error, or if the last button isn't there.
// Stack-slot permutation: device, line, sign, button and name.
// FUNCTION: MW2 0x1007a168
MechS32 ParseInputConditions(FILE* p_file, MechS32* p_count, InputCondition* p_conditions)
{
	MechS32 device;
	MechChar line[256];
	MechChar buttonName[20];
	MechChar deviceName[40];
	MechChar sign[4];
	MechS32 button;
	MechChar name[40];

	button = 0;
	*p_count = 0;
	while (ReadInputMapLine(line, 0xff, p_file)) {
		if (line[0] == '}') {
			break;
		}

		if (sscanf(line, " %1[+-] %s %s", sign, deviceName, buttonName) != 3) {
			Error(0x11, "input mapping error: line %d, read error - \"%s\"", g_inputMapLine, name, 0);
			return FALSE;
		}

		if (*p_count >= 8) {
			Error(0x11, "input mapping error: line %d, chord too big - \"%s\"", g_inputMapLine, name, 0);
			return FALSE;
		}

		device = FindInputDevice(deviceName);
		button = FindInputButton(device, buttonName);
		if (button != -1) {
			if (sign[0] == '-') {
				p_conditions[*p_count].m_negate = 1;
			}
			else {
				p_conditions[*p_count].m_negate = 0;
			}

			p_conditions[*p_count].m_word = &g_inputDeviceStates[device].m_buttons[button >> 5];
			p_conditions[*p_count].m_mask = 1 << (button & 0x1f);
			(*p_count)++;
		}
	}

	return button == -1 ? FALSE : TRUE;
}

// Returns whether all p_count conditions hold.
// FUNCTION: MW2 0x1007a330
MechS32 CheckInputConditions(MechS32 p_count, InputCondition* p_conditions)
{
	MechS32 i;

	for (i = 0; i < p_count; i++) {
		if (p_conditions[i].m_negate == 1) {
			if (*p_conditions[i].m_word & p_conditions[i].m_mask) {
				return FALSE;
			}
		}
		else {
			if (!(*p_conditions[i].m_word & p_conditions[i].m_mask)) {
				return FALSE;
			}
		}
	}

	return TRUE;
}

// Parses an INPUT.MAP file: a block per sink, "name {" and its bindings up to "}". An axis sink
// reads "+ device channel" (- inverts it) and the binding's conditions; a button sink reads its
// conditions, and a _plus or _minus button also binds its axis (or the axis's _delta sink) so its
// keys move it.
// Stack-slot permutation: index, channelName, deviceName, channel and sign.
// FUNCTION: MW2 0x1007a3d1
MechS32 ParseInputMap(FILE* p_file)
{
	MechChar line[256];
	InputSink* sink;
	MechS32 index;
	MechChar channelName[20];
	MechChar deviceName[40];
	MechS32 channel;
	MechChar name[40];
	MechChar sign[4];
	MechChar* suffix;

	while (ReadInputMapLine(line, 0xff, p_file)) {
		if (sscanf(line, "%s {", name) != 1) {
			Error(0x11, "input mapping error: line %d, read error", g_inputMapLine, 0);
			return FALSE;
		}

		sink = FindInputSink(name);
		if (!sink) {
			Error(0x11, "input mapping error: line %d, bad field \"%s\"", g_inputMapLine, name, 0);
			return FALSE;
		}

		if (sink->m_kind == c_inputSinkAxis) {
			if (!ReadInputMapLine(line, 0xff, p_file)) {
				Error(0x11, "input mapping error: line %d, unexpected EOF", g_inputMapLine, 0);
				return FALSE;
			}

			if (sscanf(line, " %1[+-] %s %s", sign, deviceName, channelName) != 3) {
				Error(0x11, "input mapping error: line %d, read error - \"%s\"", g_inputMapLine, name, 0);
				return FALSE;
			}

			channel = FindInputAxis(FindInputDevice(deviceName), channelName);
			if (channel != -1) {
				index = AddAnalogBinding(sink, deviceName, channel);
				if (index < 0) {
					return FALSE;
				}

				if (sign[0] == '-') {
					g_analogBindings[index].m_sign = -1;
				}
				else {
					g_analogBindings[index].m_sign = 1;
				}

				if (!ParseInputConditions(
						p_file,
						&g_analogBindings[index].m_conditionCount,
						g_analogBindings[index].m_conditions
					)) {
					return FALSE;
				}
			}
			else {
				while (ReadInputMapLine(line, 0xff, p_file)) {
					if (line[0] == '}') {
						break;
					}
				}
			}
		}
		else if (sink->m_kind == c_inputSinkButton) {
			index = g_discreteBindingCount;
			if (ParseInputConditions(
					p_file,
					&g_discreteBindings[index].m_conditionCount,
					g_discreteBindings[index].m_conditions
				)) {
				g_discreteBindingCount++;
				if (g_discreteBindingCount > 100) {
					Error(0x11, "Too many discrete controls: line %d", g_inputMapLine, 0);
					return FALSE;
				}

				g_discreteBindings[index].m_output = sink->m_output;
				g_discreteBindings[index].m_edge = FALSE;
				if (!strncmp(sink->m_name, "menu_", 5)) {
					g_discreteBindings[index].m_menuOnly = TRUE;
				}
				else {
					g_discreteBindings[index].m_menuOnly = FALSE;
				}

				suffix = strstr(name, "_plus");
				if (!suffix) {
					suffix = strstr(name, "_minus");
				}

				if (suffix) {
					*suffix = '\0';
					sink = FindInputSink(name);
					if (!sink) {
						strcat(name, "_delta");
						sink = FindInputSink(name);
					}

					if (sink && AddAnalogBinding(sink, NULL, 0) < 0) {
						return FALSE;
					}
				}
			}
		}
		else {
			index = g_discreteBindingCount;
			if (ParseInputConditions(
					p_file,
					&g_discreteBindings[index].m_conditionCount,
					g_discreteBindings[index].m_conditions
				)) {
				g_discreteBindingCount++;
				g_discreteBindings[index].m_output = sink->m_output;
				g_discreteBindings[index].m_edge = TRUE;
				if (!strncmp(sink->m_name, "menu_", 5)) {
					g_discreteBindings[index].m_menuOnly = TRUE;
				}
				else {
					g_discreteBindings[index].m_menuOnly = FALSE;
				}
			}
		}
	}

	return TRUE;
}

// Loads INPUT.MAP, then each present device's own map: giddi\<device name without its trailing
// digits>.std.
// Stack-slot permutation: name and i.
// FUNCTION: MW2 0x1007a97a
MechS32 LoadInputMap(void)
{
	MechChar name[256];
	MechS32 i;
	FILE* file;

	file = MechFopen("input.map", "r");
	if (!file) {
		Error(0x11, "Can't open input mapping file", 0);
		return FALSE;
	}

	ParseInputMap(file);
	fclose(file);
	for (i = 0; i < g_inputDeviceCount; i++) {
		if (g_inputDevicePresent[i]) {
			strcpy(name, "giddi");
			strcat(name, "\\");
			strcat(name, g_inputDeviceNames[i]);
			while (isdigit(name[strlen(name) - 1])) {
				name[strlen(name) - 1] = '\0';
			}

			strcat(name, ".std");
			file = MechFopen(name, "r");
			if (file) {
				ParseInputMap(file);
				fclose(file);
			}
		}
	}

	return TRUE;
}

// Loads GAMEKEY.MAP: an action name and its key sequence ("CTRL+F1") a line.
// Stack-slot permutation: keyCode, i, keys, action, file and code.
// FUNCTION: MW2 0x1007aba9
MechS32 LoadGamekeyMap(void)
{
	MechChar line[256];
	MechChar* token;
	MechS16 keyCode;
	MechU32 i;
	MechChar keys[40];
	MechChar action[40];
	FILE* file;
	MechS32 code;

	g_gameKeyByKeyCode[0x20] = 0x4f;
	g_gameKeyByKeyCode[0x171] = 0x59;
	g_inputMapLine = 0;
	file = MechFopen("gamekey.map", "r");
	if (!file) {
		Error(0x11, "Can't open gamekey mapping file", 0);
		return FALSE;
	}

	while (ReadInputMapLine(line, 0xff, file)) {
		if (sscanf(line, "%s %s", action, keys) != 2) {
			Error(0x11, "gamekey mapping error: line %d, read error", g_inputMapLine, 0);
			return FALSE;
		}

		token = strtok(keys, "+");
		if (!token) {
			Error(0x11, "gamekey mapping error: line %d, bad sequence", g_inputMapLine, 0);
			return FALSE;
		}

		keyCode = 0;
		while (token) {
			code = 0;
			for (i = 0; i < sizeof(g_gameKeyModifiers) / sizeof(g_gameKeyModifiers[0]); i++) {
				if (!_strcmpi(g_gameKeyModifiers[i].m_name, token)) {
					code = g_gameKeyModifiers[i].m_code;
					break;
				}
			}

			if (code) {
				keyCode += code;
			}
			else {
				keyCode += tolower(*token);
			}

			token = strtok(NULL, "+");
		}

		for (i = 0; i < sizeof(g_gameKeyNames) / sizeof(g_gameKeyNames[0]); i++) {
			if (!_strcmpi(g_gameKeyNames[i].m_name, action)) {
				break;
			}
		}

		if (i >= sizeof(g_gameKeyNames) / sizeof(g_gameKeyNames[0])) {
			Error(0x11, "gamekey mapping error: line %d, no gamekey %s", g_inputMapLine, action);
			return FALSE;
		}

		if (g_gameKeyByKeyCode[keyCode] && g_gameKeyNames[i].m_action != g_gameKeyByKeyCode[keyCode]) {
			Error(0x11, "gamekey mapping error: line %d, multiple defines '%s' ", g_inputMapLine, line);
			return FALSE;
		}

		g_gameKeyByKeyCode[keyCode] = g_gameKeyNames[i].m_action;
	}

	fclose(file);
	return TRUE;
}

// Runs an analog binding's axis keys for the frame: _reset centers the axis (and the device's
// channel), _set works the position back from the output, and unless something already drove the
// axis, _plus and _minus ramp it. Returns whether the axis was driven.
// Stack-slot permutation: driven, device, channel and position.
// UpdateAxisFromKeys on the Rust side (src/sim/input.rs) wraps it, ramping with _plus and _minus at
// the same rate whatever the framerate.
// FUNCTION: MW2 0x1007aecc
MechS32 UpdateAxisFromKeysC(AnalogBinding* p_binding)
{
	InputAxis* axis;
	MechS32 driven;
	MechS32 device;
	MechS32 channel;
	MechDouble position;

	axis = p_binding->m_axis;
	driven = axis->m_driven;
	if (axis->m_resetHeld && *axis->m_resetHeld) {
		if (p_binding->m_channelValue) {
			device = p_binding->m_device;
			channel = p_binding->m_channel;
			g_inputDrivers[device]->m_centerAxis(g_inputDeviceInfos[device].m_driverData, channel);
		}

		*axis->m_rate = 0;
		axis->m_position = axis->m_rest;
		driven = TRUE;
	}
	else if (axis->m_setHeld && *axis->m_setHeld) {
		position = *axis->m_output << axis->m_outputShift;
		position -= axis->m_minFixed;
		position *= 2.0;
		position /= axis->m_range;
		position -= 65536.0;
		*axis->m_rate = 0;
		axis->m_position = position;
		driven = TRUE;
	}

	if (!axis->m_driven) {
		if (axis->m_plusHeld && *axis->m_plusHeld) {
			if (*axis->m_rate < 0) {
				*axis->m_rate = 0;
			}

			*axis->m_rate += g_deltaTime * 3 << axis->m_rampShift;
			if (*axis->m_rate > 0x8000) {
				*axis->m_rate = 0x8000;
			}

			axis->m_position += *axis->m_rate;
			if (axis->m_position > 0x10000) {
				axis->m_position = 0x10000;
			}

			driven = TRUE;
		}

		if (axis->m_minusHeld && *axis->m_minusHeld) {
			if (*axis->m_rate > 0) {
				*axis->m_rate = 0;
			}

			*axis->m_rate -= g_deltaTime * 3 << axis->m_rampShift;
			if (*axis->m_rate < -0x8000) {
				*axis->m_rate = -0x8000;
			}

			axis->m_position += *axis->m_rate;
			if (axis->m_position < -0x10000) {
				axis->m_position = -0x10000;
			}

			driven = TRUE;
		}

		if (!driven) {
			*axis->m_rate = 0;
		}
	}

	axis->m_driven = driven;
	return driven;
}

// Loads INPUT.MAP and GAMEKEY.MAP and flushes the keyboard's key codes.
// FUNCTION: MW2 0x1007b177
void FirstInputs(void)
{
	LoadInputMap();
	LoadGamekeyMap();
	g_inputDrivers[g_keyboardDeviceIndex]->m_flushKeyCodes();
}

// Updates the sinks for the frame: runs each axis's _reset and _set keys, clears the button sinks,
// polls the devices (the keyboard only outside the chat), runs the bindings (only the menu_ ones
// while gameplay input is off) and reads the frame's key code. Alt+Enter toggles full screen
// instead.
// The loops compare i with the counts (and the keyboard's index) in the other operand order, and
// the locals are a stack-slot permutation.
// FUNCTION: MW2 0x1007b19b
void UpdateInputs(void)
{
	MechS32 i;
	MechS16 keyCode;
	AnalogBinding* binding;
	InputAxis* axis;
	MechS32 device;
	MechS32 channel;
	MechDouble position;
	MechS32 pressed;
	MechS32 value;

	for (i = 0; i < g_analogBindingCount; i++) {
		binding = &g_analogBindings[i];
		axis = binding->m_axis;
		if (axis->m_resetHeld && *axis->m_resetHeld) {
			if (binding->m_channelValue) {
				device = binding->m_device;
				channel = binding->m_channel;
				if (g_inputDevicePresent[device]) {
					g_inputDrivers[device]->m_centerAxis(g_inputDeviceInfos[device].m_driverData, channel);
				}
			}

			axis->m_position = axis->m_rest;
			*axis->m_rate = 0;
			binding->m_lastValue = 0;
		}
		else if (axis->m_setHeld && *axis->m_setHeld) {
			position = *axis->m_output << axis->m_outputShift;
			position -= axis->m_minFixed;
			position *= 2.0;
			position /= axis->m_range;
			position -= 65536.0;
			*axis->m_rate = 0;
			axis->m_position = position;
		}

		axis->m_driven = FALSE;
	}

	for (i = 0; i < sizeof(g_inputSinks) / sizeof(g_inputSinks[0]); i++) {
		if (g_inputSinks[i].m_kind != c_inputSinkAxis) {
			*(MechS8*) g_inputSinks[i].m_output = 0;
		}
	}

	for (i = 0; i < g_inputDeviceCount; i++) {
		if (g_inputDevicePresent[i] && (!g_chatRecipient || i != g_keyboardDeviceIndex)) {
			g_inputDrivers[i]->m_poll(
				g_inputDeviceInfos[i].m_driverData,
				g_inputDeviceStates[i].m_axes,
				g_inputDeviceStates[i].m_buttons
			);
		}
	}

	for (i = 0; i < g_discreteBindingCount; i++) {
		if (g_gameplayInputEnabled || g_discreteBindings[i].m_menuOnly) {
			pressed = CheckInputConditions(g_discreteBindings[i].m_conditionCount, g_discreteBindings[i].m_conditions);
			if (!g_discreteBindings[i].m_edge || !g_discreteBindings[i].m_previous) {
				*g_discreteBindings[i].m_output |= (MechS8) pressed;
			}

			g_discreteBindings[i].m_previous = pressed;
		}
	}

	for (i = 0; i < g_analogBindingCount; i++) {
		if (g_gameplayInputEnabled || g_analogBindings[i].m_menuOnly) {
			if (UpdateAxisFromKeys(&g_analogBindings[i])) {
			}
			else if (
				g_analogBindings[i].m_channelValue &&
				CheckInputConditions(g_analogBindings[i].m_conditionCount, g_analogBindings[i].m_conditions)
			) {
				value = *g_analogBindings[i].m_channelValue * g_analogBindings[i].m_sign;
				if (g_analogBindings[i].m_lastValue != value) {
					g_analogBindings[i].m_axis->m_position = value;
				}
				else if (*g_analogBindings[i].m_axis->m_rate == g_analogBindings[i].m_axis->m_position) {
					g_analogBindings[i].m_axis->m_position = value;
				}

				g_analogBindings[i].m_lastValue = value;
			}

			value = g_analogBindings[i].m_axis->m_position;
			if (value > 0x10000) {
				value = 0x10000;
			}

			if (value < -0x10000) {
				value = -0x10000;
			}

			value += 0x10000;
			value = g_analogBindings[i].m_axis->m_range * value;
			value /= 2;
			value += g_analogBindings[i].m_axis->m_minFixed;
			*g_analogBindings[i].m_axis->m_output = value >> g_analogBindings[i].m_axis->m_outputShift;
		}
	}

	if (g_inputDevicePresent[g_keyboardDeviceIndex]) {
		g_inputDrivers[g_keyboardDeviceIndex]->m_readKeyCode(&keyCode);
	}

	if (keyCode == 0x40d) {
		// Alt+Enter. The original called ToggleFullScreen; the Rust side handles the key now.
	}
	else {
		g_localSteering.m_keyCode = keyCode;
	}
}

// FUNCTION: MW2 0x1007b704
void CloseInputDevices(void)
{
	MechS32 i;

	i = g_inputDeviceCount;
	while (i--) {
		if (g_inputDevicePresent[i]) {
			g_inputDrivers[i]->m_closeDevice(&g_inputDeviceInfos[i]);
		}
	}
}

// FUNCTION: MW2 0x1007b768
void DisableGameplayInput(void)
{
	g_gameplayInputEnabled = FALSE;
}

// FUNCTION: MW2 0x1007b77d
void EnableGameplayInput(void)
{
	g_gameplayInputEnabled = TRUE;
}

// The game key action of p_keyCode.
// FUNCTION: MW2 0x1007b792
MechS16 LookupGameKey(MechS16 p_keyCode)
{
	return g_gameKeyByKeyCode[p_keyCode];
}

// Reports an input device error p_code in a message box: 0x6e a missing device, 0x6f a missing
// channel, 0x70 a device that doesn't open. Cancel exits the game.
// FUNCTION: MW2 0x1007b7b1
void ReportInputDeviceError(MechS32 p_code, MechChar* p_channel, MechChar* p_device)
{
	switch (p_code) {
	case 0x6e:
		sprintf(
			g_inputErrorText,
			"The input device \"%s\" does not exist or is not configured properly.\n\nTo eliminate the problem, "
			"you should check the Windows Joystick Control Panel settings, or reconfigure your Cockpit Controls "
			"in the Clan Hall without using \"%s\".\n\nPush OK to disable this device and continue, or push "
			"CANCEL to abort the mission.",
			p_device,
			p_device
		);
		break;
	case 0x6f:
		sprintf(
			g_inputErrorText,
			"The input channel \"%s\" on device \"%s\" does not exist.\n\nIf you have changed your joystick "
			"configuration, you should also reconfigure your Cockpit Controls in the Clan Hall.\n\nPush OK to "
			"disable this control and continue, or push CANCEL to abort the mission.",
			p_channel,
			p_device
		);
		break;
	case 0x70:
		sprintf(
			g_inputErrorText,
			"The input device \"%s\" is not connected properly.\n\nTo eliminate the problem, you should check "
			"that your joystick is plugged in correctly and check the Windows Control Panel settings. "
			"Alternatively, you could reconfigure your Cockpit Controls in the Clan Hall without using "
			"\"%s\".\n\nPush OK to disable this device and continue, or push CANCEL to abort the mission.",
			p_device,
			p_device
		);
		break;
	default:
		sprintf(
			g_inputErrorText,
			"An input device has caused an undefined error. Sorry, no other information is available.\nPush OK "
			"to ignore this error and continue, or push CANCEL to abort the mission."
		);
		break;
	}

	WriteToMw2Log(g_inputErrorText);
	// The original showed it in a message box whose Cancel ended the game
	MechLogError(g_inputErrorText);
}
