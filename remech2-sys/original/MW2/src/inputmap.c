/* The local player's steering, and the sinks the input layer writes it through. The unit is not
   named input.c: the CRT has an input.c (the scanf engine) whose line records reccmp confuses with
   it. */
#include "inputmap.h"

#include "decomp.h"
#include "types.h"

// The local player's steering: the outputs of the sinks.
// GLOBAL: MW2 0x100b2500
PlayerSteering g_localSteering = {0};

// The outputs of the sinks past the steering. pilot_tilt and pilot_pan are the cockpit
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

// GLOBAL: MW2 0x100b25a4
MechS32 g_gameplayInputEnabled = 1;

// The sinks the input layer writes.
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
