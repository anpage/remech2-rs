#ifndef SIMHANDOFFSTATE_H
#define SIMHANDOFFSTATE_H

#include "customstar.h"
#include "decomp.h"
#include "types.h"

// SIZE 0x218
// The shell's state across a mission, kept by the Rust side before the simulator runs and read
// back after it.
struct SimHandoffState {
	undefined4 m_msg;             // 0x00 — the message to post on return
	undefined4 m_campaign;        // 0x04
	undefined4 m_pilotChosen;     // 0x08
	MechS32 m_playerStarSelected; // 0x0c
	CustomStar m_playerStar;      // 0x10
	CustomStar m_enemyStar;       // 0x90
	undefined4 m_briefingMission; // 0x110 — an index into g_briefingScenarios (missionui.cpp)
	MechS32 m_pilot;              // 0x114 — an index into g_pilotRoster, -1 for none
	MechChar m_cmdLine[0x100];    // 0x118 — the simulator's command line
};

#endif // SIMHANDOFFSTATE_H
