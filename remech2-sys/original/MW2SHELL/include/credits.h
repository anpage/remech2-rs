#ifndef CREDITS_H
#define CREDITS_H

#include "decomp.h"
#include "types.h"

class LoopingMovie;

// The functions and globals of credits.cpp that other units use.
extern MechChar* g_creditLines[0x21f];
extern LoopingMovie* g_creditsMovie;
extern MechChar g_creditsMovieName[0x10];
extern MechS32 g_creditsFirstLine;
extern undefined g_creditsTitleColors[0x100];
extern MechS32 g_creditsScrollTop;
void DrawCredits();

#endif // CREDITS_H
