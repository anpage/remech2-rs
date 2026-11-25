#ifndef CLANHALL_H
#define CLANHALL_H

#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

class AudioSample;
class ButtonMenu;

// The functions and globals of clanhall.cpp that other units use.
extern ButtonMenu* g_clanHallMenu;
extern AudioSample* g_clanHallAmbience;
extern AudioSample* g_welcomeSound;
extern int32_t g_welcomePending;
extern int32_t g_clanHallExitVideo;
extern int32_t g_clanHallExitMessage;
void DrawClanHall(TMPackDataBase* p_database, int32_t p_campaign, uint8_t, size_t p_wParam);

#endif // CLANHALL_H
