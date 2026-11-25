#ifndef SIMHANDOFFSTATE_H
#define SIMHANDOFFSTATE_H

#include "customstar.h"
#include "types.h"

// The shell's state across a mission, kept by the Rust side before the simulator runs and read
// back after it.
struct SimHandoffState {
	uint32_t m_msg;               // 0x00 — the message to post on return
	uint32_t m_campaign;          // 0x04
	uint32_t m_pilotChosen;       // 0x08
	int32_t m_playerStarSelected; // 0x0c
	CustomStar m_playerStar;      // 0x10
	CustomStar m_enemyStar;       // 0x90
	uint32_t m_briefingMission;   // 0x110 — an index into g_briefingScenarios (missionui.cpp)
	int32_t m_pilot;              // 0x114 — an index into g_pilotRoster, -1 for none
	char m_cmdLine[0x100];        // 0x118 — the simulator's command line
};

#endif // SIMHANDOFFSTATE_H
