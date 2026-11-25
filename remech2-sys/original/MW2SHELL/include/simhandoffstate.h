#ifndef SIMHANDOFFSTATE_H
#define SIMHANDOFFSTATE_H

#include "customstar.h"
#include "types.h"

// The shell's state across a mission, kept by the Rust side before the simulator runs and read
// back after it.
struct SimHandoffState {
	uint32_t m_msg; // the message to post on return
	uint32_t m_campaign;
	uint32_t m_pilotChosen;
	int32_t m_playerStarSelected;
	CustomStar m_playerStar;
	CustomStar m_enemyStar;
	uint32_t m_briefingMission; // an index into g_briefingScenarios (missionui.cpp)
	int32_t m_pilot;            // an index into g_pilotRoster, -1 for none
	char m_cmdLine[0x100];      // the simulator's command line
};

#endif // SIMHANDOFFSTATE_H
