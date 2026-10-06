#ifndef OPTIONS_H
#define OPTIONS_H

#include "palettecolor.h"
#include "screenfield.h"
#include "textglyph.h"
#include "types.h"

class AudioSample;
class LoopingMovie;

// The functions and globals of options.cpp that other units use.
extern LoopingMovie* g_optionsMovie;
extern MechChar g_optionsMovieName[0x10];
extern AudioSample* g_volumeTestSample;
extern PaletteColor g_savedPalette[0x100];
extern void* g_sliderImages;
extern MechChar* g_skillNames[3];
extern ScreenField g_optionFields[18];
void LoadSoundConfig();
void LoadDifficultyConfig();
void SaveDifficultyConfig();
void SaveSoundConfig();
void DrawOptions();
// The resolution and widescreen rows' ScreenField functions, implemented on the Rust side
// (src/shell/screens/settings.rs)
extern "C" TextGlyph* DrawResolutionOption(ScreenField* p_option);
extern "C" void ToggleVesaDriver(ScreenField* p_option);
extern "C" TextGlyph* DrawWidescreenOption(ScreenField* p_option);
extern "C" void ToggleWidescreen(ScreenField* p_option);
// Draws p_label's text (m_data) against its right edge, for a row the screen's art has no label for
extern "C" TextGlyph* DrawOptionLabel(ScreenField* p_label);

// Shows a message box, implemented on the Rust side (src/shell/dialog.rs). p_text holds the lines
// separated by '|', then after a '#' the buttons, also separated by '|'. With two buttons it waits
// for the answer and returns 0 for the first and 1 for the second. Otherwise it returns 0 at once,
// and the message stays over the screen until its button is clicked.
// Originally the dialogs 0x80 (two buttons) and 0x81 (one), which both waited.
extern "C" MechS32 ShowDialog(const char* p_text, MechS32);

#endif // OPTIONS_H
