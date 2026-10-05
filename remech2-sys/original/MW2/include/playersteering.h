#ifndef PLAYERSTEERING_H
#define PLAYERSTEERING_H

#include "decomp.h"
#include "types.h"

// A player's controls, set each frame by the input layer for the local player (g_localSteering) and
// by the AI for the others. The members are the sinks of the same names (g_inputSinks); the bytes
// are buttons, which the mech's update clears once it has acted on them.
// SIZE 0x48
typedef struct PlayerSteering {
	MechS32 m_torsoTilt;              // 0x00
	MechS32 m_torsoPan;               // 0x04 — swept between ±45 degrees by SweepTorso
	MechS32 m_throttle;               // 0x08
	MechS32 m_turn;                   // 0x0c
	MechS32 m_legsPanDelta;           // 0x10 — copied into m_turn
	MechS16 m_keyCode;                // 0x14 — the local player's: the frame's typed key code
	MechS8 m_torsoTiltPlus;           // 0x16
	MechS8 m_torsoTiltMinus;          // 0x17
	MechS8 m_torsoTiltReset;          // 0x18
	MechS8 m_torsoPanPlus;            // 0x19
	MechS8 m_torsoPanMinus;           // 0x1a
	MechS8 m_torsoPanSet;             // 0x1b
	MechS8 m_torsoPanReset;           // 0x1c
	MechS8 m_jumpJetEnabled;          // 0x1d
	MechS8 m_jumpJetFireLeft;         // 0x1e
	MechS8 m_jumpJetFireRight;        // 0x1f
	MechS8 m_jumpJetFireForward;      // 0x20
	MechS8 m_jumpJetFireBackward;     // 0x21
	MechS8 m_throttlePlus;            // 0x22
	MechS8 m_throttleMinus;           // 0x23
	MechS8 m_throttleSet;             // 0x24
	MechS8 m_weaponFire;              // 0x25 — fire the selected weapon
	MechS8 m_weaponCycle;             // 0x26
	MechS8 m_weaponFireGroup;         // 0x27 — fire every weapon
	MechS8 m_weaponFireGroup1;        // 0x28
	MechS8 m_weaponFireGroup2;        // 0x29
	MechS8 m_weaponFireGroup3;        // 0x2a
	MechS8 m_weaponCycleGroup;        // 0x2b
	MechS8 m_toggleGroupFire;         // 0x2c — toggles g_singleWeaponFire
	MechS8 m_legsPanMinus;            // 0x2d
	MechS8 m_legsPanPlus;             // 0x2e
	MechS8 m_reverse;                 // 0x2f — the throttle drives the mech backwards (GAMEKEY.MAP toggles it)
	MechS8 m_advanceNav;              // 0x30
	MechS8 m_previousNav;             // 0x31
	MechS8 m_resetNav;                // 0x32
	MechS8 m_advanceTarget;           // 0x33
	MechS8 m_previousTarget;          // 0x34
	MechS8 m_resetTarget;             // 0x35
	MechS8 m_targetReticle;           // 0x36
	MechS8 m_targetFriendly;          // 0x37
	MechS8 m_nearestEnemy;            // 0x38
	MechS8 m_targetLastShot;          // 0x39
	MechS8 m_inspectTarget;           // 0x3a — inspects the target for the team when it is in range (UpdateTarget)
	MechS8 m_nextObjective;           // 0x3b — GAMEKEY.MAP's NEXT_OBJECTIVE: the next objective nav
	MechS8 m_advanceGamething;        // 0x3c
	MechS8 m_previousGamething;       // 0x3d
	MechS8 m_resetGamething;          // 0x3e
	MechS8 m_advanceGamepiece;        // 0x3f
	MechS8 m_previousGamepiece;       // 0x40
	MechS8 m_resetGamepiece;          // 0x41
	MechS8 m_autopilot;               // 0x42
	MechS8 m_selfDestruct;            // 0x43 — ManeuverKama sets it near its target
	MechS8 m_grantJumpJets;           // 0x44 — the "flygirl" cheat sets it: jump jets for a mech without them
	undefined m_unk0x45[0x48 - 0x45]; // 0x45
} PlayerSteering;

#endif // PLAYERSTEERING_H
