#ifndef SHELLGLOBALS_H
#define SHELLGLOBALS_H

#include "audiosubsystem.h"
#include "decomp.h"
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
extern Font* g_unk0x1007121c;
extern Font* g_unk0x10071220;
extern Font* g_archiveFont;
extern Font* g_bodyFont;
extern TMPackDataBase* g_mw2Database;
extern ProjectArchive* g_projectArchive;
extern MechS32 g_midiAudio;
extern MechS32 g_digitalAudio;
extern MechS32 g_unk0x1007123c;
extern MechS32 g_runSim;
extern MechU32 g_movieOpenFlags;
extern MechU8 g_drawFmv;
extern MechChar* g_rankNames[10];
extern MechChar* g_clanNames[6];
extern MechS32 g_trialsSongs[18];
extern MechS32 g_wolfSongs[18];
extern MechS32 g_jadeFalconSongs[18];
extern PilotRecord* g_currentPilot;
extern MechS32 g_newPilotRegistered;
extern PaletteColor g_savedScreenPalette[0x100];
extern SoundConfig g_soundConfig;
extern DifficultyConfig g_difficultyConfig;
extern PilotRecord g_pilotRoster[20];

#endif // SHELLGLOBALS_H
