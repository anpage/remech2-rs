#ifndef OPTIONS_H
#define OPTIONS_H

#include "types.h"

// The functions and globals of options.cpp that other units use.
void LoadSoundConfig();
void LoadDifficultyConfig();
void DrawOptions();

// Shows a message box, implemented on the Rust side (src/shell/dialog.rs). p_text holds the lines
// separated by '|', then after a '#' the buttons, also separated by '|'. With two buttons it waits
// for the answer and returns 0 for the first and 1 for the second. Otherwise it returns 0 at once,
// and the message stays over the screen until its button is clicked.
// Originally the dialogs 0x80 (two buttons) and 0x81 (one), which both waited.
extern "C" MechS32 ShowDialog(const char* p_text, MechS32);

#endif // OPTIONS_H
