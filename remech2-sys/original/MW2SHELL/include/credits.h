#ifndef CREDITS_H
#define CREDITS_H

#include "decomp.h"
#include "types.h"

class LoopingMovie;

// The functions and globals of credits.cpp that other units use.
extern char* g_creditLines[0x21f];
extern LoopingMovie* g_creditsMovie;
extern char g_creditsMovieName[0x10];
extern int32_t g_creditsFirstLine;
extern undefined g_creditsTitleColors[0x100];
extern int32_t g_creditsScrollTop;
void DrawCredits();

#endif // CREDITS_H
