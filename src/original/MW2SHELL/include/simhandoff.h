#ifndef SIMHANDOFF_H
#define SIMHANDOFF_H

#include "simhandoffstate.h"
#include "types.h"

#include <windows.h>

// The functions and globals of simhandoff.cpp that other units use.
extern MechChar g_missionName[0x10];
extern SimHandoffState g_simHandoff;

void ReadSimHandoff(BOOL p_fromSim, MechS32* p_campaign, MechU8* p_pilotChosen, char** p_scenario);
void WriteSimHandoff(UINT p_msg, MechS32 p_campaign, MechU8 p_pilotChosen, const char* p_scenario);

#endif // SIMHANDOFF_H
