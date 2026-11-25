#ifndef READYROOM_H
#define READYROOM_H

#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

class AudioSample;
class ButtonMenu;

// The functions and globals of readyroom.cpp that other units use.
extern int32_t g_readyRoomExitVideo;
extern int32_t g_readyRoomExitMessage;
extern AudioSample* g_readyRoomSound;
extern ButtonMenu* g_readyRoomMenu;
extern size_t g_readyRoomMessage;
void DrawReadyRoom(TMPackDataBase* p_database, int32_t p_campaign, char** p_scenario, size_t p_wParam);

#endif // READYROOM_H
