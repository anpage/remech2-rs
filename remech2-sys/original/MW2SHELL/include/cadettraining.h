#ifndef CADETTRAINING_H
#define CADETTRAINING_H

#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

class AudioSample;
class ButtonMenu;

// The functions and globals of cadettraining.cpp that other units use.
extern MechS32 g_trainerTake;
extern MechS32 g_trainerIdleCountdown;
extern MechS32 g_trainingButtonsShown;
extern MechS32 g_trainingExitVideo;
extern AudioSample* g_trainingAmbience;
extern ButtonMenu* g_cadetTrainingMenu;
extern size_t g_trainingMessage;
void DrawCadetTraining(TMPackDataBase* p_database, MechS32 p_campaign, char**, size_t p_wParam);

#endif // CADETTRAINING_H
