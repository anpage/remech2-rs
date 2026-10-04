#ifndef ROSTERSCREEN_H
#define ROSTERSCREEN_H

#include "tmpackdatabase.h"
#include "types.h"

// The functions and globals of rosterscreen.cpp that other units use.
void DrawPilotRoster(TMPackDataBase* p_database, MechS32 p_campaign, MechU8* p_pilotChosen, char**);

#endif // ROSTERSCREEN_H
