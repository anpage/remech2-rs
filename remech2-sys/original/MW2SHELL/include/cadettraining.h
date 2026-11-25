#ifndef CADETTRAINING_H
#define CADETTRAINING_H

#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

class AudioSample;
class ButtonMenu;

// The functions and globals of cadettraining.cpp that other units use.
extern int32_t g_trainerTake;
extern int32_t g_trainerIdleCountdown;
extern int32_t g_trainingButtonsShown;
extern int32_t g_trainingExitVideo;
extern AudioSample* g_trainingAmbience;
extern ButtonMenu* g_cadetTrainingMenu;
extern size_t g_trainingMessage;
void DrawCadetTraining(TMPackDataBase* p_database, int32_t p_campaign, char**, size_t p_wParam);

#endif // CADETTRAINING_H
