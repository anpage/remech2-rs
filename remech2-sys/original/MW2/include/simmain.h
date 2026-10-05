#ifndef SIMMAIN_H
#define SIMMAIN_H

#include "decomp.h"
#include "heap.h"
#include "types.h"

struct DifficultyCfg;
struct NetLaunchInfo;
struct TimedCallback;
struct VideoDriverChoice;

// The functions and globals of simmain.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern struct DifficultyCfg* g_difficulty;
	extern struct TimedCallback* g_detachedTasks;
	extern MechS32 g_shouldQuit;
	extern MechS32 g_goLaunch;
	extern MechS32 g_quitStage;
	extern MechS32 g_localPlayerId;
	extern MechS32 g_startOnAutopilot;
	extern struct VideoDriverChoice g_videoDriverChoice;
	extern MechHeap* g_primaryHeap;
	extern MechS32 g_gameWindowWidth;
	extern MechS32 g_gameWindowHeight;
	extern MechS32 g_windowActive;
	extern MechS32 g_simPaused;
	extern MechS32 g_pauseRequested;
	extern MechS32 g_mouseOutsideClientWindow;
	extern MechS32 g_remoteWaitTime;
	extern MechS32 g_drawModeReady;

	int SimMain(char* p_cmdLine, struct NetLaunchInfo* p_netLaunch);
	void HandleMessages(void);
	void UpdatePauseState(void);
	// Implemented on the Rust side (src/sim/window.rs)
	void SetGameResolution(char* p_driverName);

#ifdef __cplusplus
}
#endif

#endif // SIMMAIN_H
