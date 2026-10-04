#ifndef MISSIONUI_H
#define MISSIONUI_H

#include "tmpackdatabase.h"
#include "types.h"

#include <windows.h>

// The functions and globals of missionui.cpp that other units use.
void ShellApplyMissionUiInfo(MechChar* p_scenario, MechS32 p_stars, MechS32 p_video);
MechS32 GetChassisCount();
void DrawMissionBriefing(TMPackDataBase* p_database, MechChar** p_scenario, WPARAM p_wParam);

#endif // MISSIONUI_H
