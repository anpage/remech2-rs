#ifndef READYROOM_H
#define READYROOM_H

#include "tmpackdatabase.h"
#include "types.h"

#include <windows.h>

// The functions and globals of readyroom.cpp that other units use.
void DrawReadyRoom(TMPackDataBase* p_database, MechS32 p_campaign, char** p_scenario, WPARAM p_wParam);

#endif // READYROOM_H
