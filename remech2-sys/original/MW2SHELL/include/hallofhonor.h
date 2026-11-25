#ifndef HALLOFHONOR_H
#define HALLOFHONOR_H

#include "types.h"

class LoopingMovie;

// The functions and globals of hallofhonor.cpp that other units use.
extern LoopingMovie* g_hallOfHonorMovie;
extern char g_hallOfHonorText[0x20];
void DrawHallOfHonor();

#endif // HALLOFHONOR_H
