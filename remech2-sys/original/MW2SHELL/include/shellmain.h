#ifndef SHELLMAIN_H
#define SHELLMAIN_H

#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

class MidiSequence;

// The shell's messages. Its message handler opens the screen of each; the screen functions get
// them as p_msg (c_msgScreenFrame on every frame) and the screen they come from as p_wParam.
enum ShellMessage {
	c_msgQuitToSim = 0x401,   // ShellMain's exit code: run the simulator
	c_msgQuit = 0x402,        // leave the game; also ShellMain's exit code
	c_msgReaderBack = 0x403,  // an ArchiveReader's result: BACK on its first page
	c_msgScreenFrame = 0x404, // the screen functions' per-frame call
	c_msgReaderExit = 0x405,  // an ArchiveReader's result: EXIT
	c_msgBriefing = 0x406,    // the first message with a song (PlayMidiSong)
	c_msgClanHall = 0x407,    // or the registration video, before a pilot is chosen
	c_msgDebrief = 0x409,
	c_msgArchive = 0x40b,
	c_msgTrials = 0x40d, // the Trials of Grievance's mission briefing
	c_msgMainMenu = 0x40e,
	c_msgMechBay = 0x40f,
	c_msgLaunchSim = 0x410,
	c_msgReadyRoom = 0x411,
	c_msgPilotRoster = 0x412,
	c_msgStarConfig = 0x413,
	c_msgCadetTraining = 0x414,
	c_msgLandingVideo = 0x415, // a clan's landing video, before its clan hall
	c_msgEndingVideo = 0x416   // the campaign's ending video
};

// The shell menu's commands (menu 104)
enum ShellMenuCommand {
	c_menuNewAllegiance = 40001,
	c_menuHallOfHonor = 40002,
	c_menuFleeToWindows = 40003,
	c_menuCockpitControls = 40011,
	c_menuHelpContents = 40012, // "Codes and Procedures"
	c_menuKeshik = 40082,       // the credits
	c_menuCombatVariables = 40084,
	c_menuTechnicalHelp = 40085
};

// The functions and globals of shellmain.cpp that other units use.
extern void (*g_screenFunction)(TMPackDataBase*, int32_t*, uint8_t*, char**, int32_t);
extern char* g_scenario;
extern int32_t g_selectedCampaign;
extern uint8_t g_pilotChosen;
extern void (*g_menuFunction)(int32_t p_active);
extern MidiSequence* g_midiBackgroundMusic;
extern int32_t g_cursorHidden;
extern uint8_t g_midiSongBaseSet;
extern int32_t g_midiSongBase;
extern int32_t g_midiSongPlaying;
extern uint8_t g_menuCommandEnabled[5];

extern "C" int ShellMain(char* p_cmdLine);
// Implemented on the Rust side (src/shell/screens/debug.rs), around ShellHandleMessageC
extern "C" intptr_t ShellHandleMessage(uint32_t p_msg, size_t p_wParam, intptr_t p_lParam);
intptr_t ShellHandleMessageC(uint32_t p_msg, size_t p_wParam, intptr_t p_lParam);
void CloseMenuFunction();
int32_t PumpMessage();
extern "C" int32_t IsShellMenuCommandEnabled(int32_t p_command);
void EnableShellMenuCommand(int32_t p_command, int32_t p_enabled);
void EnableShellMenu();
void DisableShellMenu();
void RegisterScreenFunction(void (*p_callback)(TMPackDataBase*, int32_t*, uint8_t*, char**, int32_t));
void UnregisterScreenFunction(void (*p_callback)(TMPackDataBase*, int32_t*, uint8_t*, char**, int32_t));
void RegisterMenuFunction(void (*p_callback)(int32_t p_active));
void UnregisterMenuFunction(void (*p_callback)(int32_t p_active));

#endif // SHELLMAIN_H
