#ifndef PLAYERSTEERING_H
#define PLAYERSTEERING_H

#include "types.h"

// A player's controls, set each frame by the input layer for the local player (g_localSteering) and
// by the AI for the others. The members are the sinks of the same names (g_inputSinks); the bytes
// are buttons, which the mech's update clears once it has acted on them.
typedef struct PlayerSteering {
	int32_t m_torsoTilt;          // 0x00
	int32_t m_torsoPan;           // 0x04 — swept between ±45 degrees by SweepTorso
	int32_t m_throttle;           // 0x08
	int32_t m_turn;               // 0x0c
	int32_t m_legsPanDelta;       // 0x10 — copied into m_turn
	int16_t m_keyCode;            // 0x14 — the local player's: the frame's typed key code
	int8_t m_torsoTiltPlus;       // 0x16
	int8_t m_torsoTiltMinus;      // 0x17
	int8_t m_torsoTiltReset;      // 0x18
	int8_t m_torsoPanPlus;        // 0x19
	int8_t m_torsoPanMinus;       // 0x1a
	int8_t m_torsoPanSet;         // 0x1b
	int8_t m_torsoPanReset;       // 0x1c
	int8_t m_jumpJetEnabled;      // 0x1d
	int8_t m_jumpJetFireLeft;     // 0x1e
	int8_t m_jumpJetFireRight;    // 0x1f
	int8_t m_jumpJetFireForward;  // 0x20
	int8_t m_jumpJetFireBackward; // 0x21
	int8_t m_throttlePlus;        // 0x22
	int8_t m_throttleMinus;       // 0x23
	int8_t m_throttleSet;         // 0x24
	int8_t m_weaponFire;          // 0x25 — fire the selected weapon
	int8_t m_weaponCycle;         // 0x26
	int8_t m_weaponFireGroup;     // 0x27 — fire every weapon
	int8_t m_weaponFireGroup1;    // 0x28
	int8_t m_weaponFireGroup2;    // 0x29
	int8_t m_weaponFireGroup3;    // 0x2a
	int8_t m_weaponCycleGroup;    // 0x2b
	int8_t m_toggleGroupFire;     // 0x2c — toggles g_singleWeaponFire
	int8_t m_legsPanMinus;        // 0x2d
	int8_t m_legsPanPlus;         // 0x2e
	int8_t m_reverse;             // 0x2f — the throttle drives the mech backwards (GAMEKEY.MAP toggles it)
	int8_t m_advanceNav;          // 0x30
	int8_t m_previousNav;         // 0x31
	int8_t m_resetNav;            // 0x32
	int8_t m_advanceTarget;       // 0x33
	int8_t m_previousTarget;      // 0x34
	int8_t m_resetTarget;         // 0x35
	int8_t m_targetReticle;       // 0x36
	int8_t m_targetFriendly;      // 0x37
	int8_t m_nearestEnemy;        // 0x38
	int8_t m_targetLastShot;      // 0x39
	int8_t m_inspectTarget;       // 0x3a — inspects the target for the team when it is in range (UpdateTarget)
	int8_t m_nextObjective;       // 0x3b — GAMEKEY.MAP's NEXT_OBJECTIVE: the next objective nav
	int8_t m_advanceGamething;    // 0x3c
	int8_t m_previousGamething;   // 0x3d
	int8_t m_resetGamething;      // 0x3e
	int8_t m_advanceGamepiece;    // 0x3f
	int8_t m_previousGamepiece;   // 0x40
	int8_t m_resetGamepiece;      // 0x41
	int8_t m_autopilot;           // 0x42
	int8_t m_selfDestruct;        // 0x43 — ManeuverKama sets it near its target
	int8_t m_grantJumpJets;       // 0x44 — the "flygirl" cheat sets it: jump jets for a mech without them
} PlayerSteering;

#endif // PLAYERSTEERING_H
