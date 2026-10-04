#ifndef SIMHANDOFF_H
#define SIMHANDOFF_H

#include "simhandoffstate.h"
#include "types.h"

// The functions and globals of simhandoff.cpp that other units use.
extern MechChar g_missionName[0x10];
extern SimHandoffState g_simHandoff;

void ReadSimHandoff(MechS32 p_fromSim, MechS32* p_campaign, MechU8* p_pilotChosen, char** p_scenario);
void WriteSimHandoff(MechU32 p_msg, MechS32 p_campaign, MechU8 p_pilotChosen, const char* p_scenario);

#endif // SIMHANDOFF_H
