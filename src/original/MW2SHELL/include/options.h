#ifndef OPTIONS_H
#define OPTIONS_H

#include "types.h"

// The functions and globals of options.cpp that other units use.
void LoadDifficultyConfig();
void DrawOptions();
MechS32 ShowDialog(const char* p_text, MechS32);

#endif // OPTIONS_H
