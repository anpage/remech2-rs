#ifndef READYROOM_H
#define READYROOM_H

#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

class AudioSample;
class ButtonMenu;

// The functions and globals of readyroom.cpp that other units use.
extern MechS32 g_readyRoomExitVideo;
extern MechS32 g_readyRoomExitMessage;
extern AudioSample* g_readyRoomSound;
extern ButtonMenu* g_readyRoomMenu;
extern size_t g_readyRoomMessage;
void DrawReadyRoom(TMPackDataBase* p_database, MechS32 p_campaign, char** p_scenario, size_t p_wParam);

#endif // READYROOM_H
