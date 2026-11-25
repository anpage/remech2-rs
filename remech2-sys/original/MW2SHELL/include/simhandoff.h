#ifndef SIMHANDOFF_H
#define SIMHANDOFF_H

#include "simhandoffstate.h"
#include "types.h"

// The functions and globals of simhandoff.cpp that other units use.
extern char g_missionName[0x10];
extern SimHandoffState g_simHandoff;

// Implemented on the Rust side (src/shell/handoff.rs)
extern "C" void MechSaveSimHandoff(const SimHandoffState* p_state);
extern "C" int MechLoadSimHandoff(SimHandoffState* p_state);

void ReadSimHandoff(int32_t p_fromSim, int32_t* p_campaign, uint8_t* p_pilotChosen, char** p_scenario);
void WriteSimHandoff(uint32_t p_msg, int32_t p_campaign, uint8_t p_pilotChosen, const char* p_scenario);

#endif // SIMHANDOFF_H
