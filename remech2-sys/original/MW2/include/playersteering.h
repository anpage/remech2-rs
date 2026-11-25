#ifndef PLAYERSTEERING_H
#define PLAYERSTEERING_H

#include "types.h"

// A player's controls, set each frame by the input layer for the local player (g_localSteering) and
// by the AI for the others. The members are the sinks of the same names (g_inputSinks); the bytes
// are buttons, which the mech's update clears once it has acted on them.
typedef struct PlayerSteering {
	int32_t m_torsoTilt;
	int32_t m_torsoPan; // swept between ±45 degrees by SweepTorso
	int32_t m_throttle;
	int32_t m_turn;
	int32_t m_legsPanDelta; // copied into m_turn
	int16_t m_keyCode;      // the local player's: the frame's typed key code
	int8_t m_torsoTiltPlus;
	int8_t m_torsoTiltMinus;
	int8_t m_torsoTiltReset;
	int8_t m_torsoPanPlus;
	int8_t m_torsoPanMinus;
	int8_t m_torsoPanSet;
	int8_t m_torsoPanReset;
	int8_t m_jumpJetEnabled;
	int8_t m_jumpJetFireLeft;
	int8_t m_jumpJetFireRight;
	int8_t m_jumpJetFireForward;
	int8_t m_jumpJetFireBackward;
	int8_t m_throttlePlus;
	int8_t m_throttleMinus;
	int8_t m_throttleSet;
	int8_t m_weaponFire; // fire the selected weapon
	int8_t m_weaponCycle;
	int8_t m_weaponFireGroup; // fire every weapon
	int8_t m_weaponFireGroup1;
	int8_t m_weaponFireGroup2;
	int8_t m_weaponFireGroup3;
	int8_t m_weaponCycleGroup;
	int8_t m_toggleGroupFire; // toggles g_singleWeaponFire
	int8_t m_legsPanMinus;
	int8_t m_legsPanPlus;
	int8_t m_reverse; // the throttle drives the mech backwards (GAMEKEY.MAP toggles it)
	int8_t m_advanceNav;
	int8_t m_previousNav;
	int8_t m_resetNav;
	int8_t m_advanceTarget;
	int8_t m_previousTarget;
	int8_t m_resetTarget;
	int8_t m_targetReticle;
	int8_t m_targetFriendly;
	int8_t m_nearestEnemy;
	int8_t m_targetLastShot;
	int8_t m_inspectTarget; // inspects the target for the team when it is in range (UpdateTarget)
	int8_t m_nextObjective; // GAMEKEY.MAP's NEXT_OBJECTIVE: the next objective nav
	int8_t m_advanceGamething;
	int8_t m_previousGamething;
	int8_t m_resetGamething;
	int8_t m_advanceGamepiece;
	int8_t m_previousGamepiece;
	int8_t m_resetGamepiece;
	int8_t m_autopilot;
	int8_t m_selfDestruct;  // ManeuverKama sets it near its target
	int8_t m_grantJumpJets; // the "flygirl" cheat sets it: jump jets for a mech without them
} PlayerSteering;

#endif // PLAYERSTEERING_H
