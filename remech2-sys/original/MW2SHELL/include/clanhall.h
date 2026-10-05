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
extern MechS32 g_welcomePending;
extern MechS32 g_clanHallExitVideo;
extern MechS32 g_clanHallExitMessage;
void DrawClanHall(TMPackDataBase* p_database, MechS32 p_campaign, MechU8, size_t p_wParam);

#endif // CLANHALL_H
