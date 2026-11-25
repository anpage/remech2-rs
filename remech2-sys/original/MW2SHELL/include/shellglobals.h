#ifndef SHELLGLOBALS_H
#define SHELLGLOBALS_H

#include "audiosubsystem.h"
#include "difficultyconfig.h"
#include "font.h"
#include "keyboardinput.h"
#include "mousestate.h"
#include "palettecolor.h"
#include "pilotrecord.h"
#include "projectarchive.h"
#include "soundconfig.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "videodriver.h"

// The functions and globals of shellglobals.cpp that other units use.
extern KeyboardInput* g_keyboardInput;
extern AudioSubsystem* g_audioSubsystem;
extern void* g_cursorShape;
extern MouseState* g_mouseState;
extern VideoDriver* g_videoDriver;
extern Font* g_defaultFont;
extern Font* g_textFont;
extern Font* g_titleFont;
extern Font* g_buttonFont;
extern Font* g_archiveFont;
extern Font* g_bodyFont;
extern TMPackDataBase* g_mw2Database;
extern ProjectArchive* g_projectArchive;
extern int32_t g_midiAudio;
extern int32_t g_digitalAudio;
extern int32_t g_runSim;
extern uint32_t g_movieOpenFlags;
extern uint8_t g_drawFmv;
extern char* g_rankNames[10];
extern char* g_clanNames[6];
extern int32_t g_trialsSongs[18];
extern int32_t g_wolfSongs[18];
extern int32_t g_jadeFalconSongs[18];
extern PilotRecord* g_currentPilot;
extern int32_t g_newPilotRegistered;
extern PaletteColor g_savedScreenPalette[0x100];
extern SoundConfig g_soundConfig;
extern DifficultyConfig g_difficultyConfig;
extern PilotRecord g_pilotRoster[20];

#endif // SHELLGLOBALS_H
