#ifndef SIMMAIN_H
#define SIMMAIN_H

#include "decomp.h"
#include "types.h"

#include <windows.h>

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
	extern HANDLE g_primaryHeap;
	extern MechS32 g_gameWindowWidth;
	extern MechS32 g_gameWindowHeight;
	extern MechS32 g_windowActive;
	extern MechS32 g_simPaused;
	extern MechS32 g_pauseRequested;
	extern MechS32 g_mouseOutsideClientWindow;
	extern HWND g_gameWindow;
	extern undefined4 g_windowedSwitchPending;
	extern undefined4 g_shouldToggleFullscreen;
	extern MechS32 g_desktopWidth;
	extern MechS32 g_desktopHeight;
	extern MechU32 g_windowedSwitchTime;
	extern MechU32 g_windowedSwitchDeadline;

	int __stdcall SimMain(
		HINSTANCE p_module,
		undefined4 p_unk0x0c,
		LPSTR p_cmdLine,
		struct NetLaunchInfo* p_netLaunch,
		undefined4 p_isNetGameUnused,
		HWND p_hWnd
	);
	void HandleMessages(void);
	void UpdatePauseState(void);
	void SetGameResolution(char* p_driverName);

#ifdef __cplusplus
}
#endif

#endif // SIMMAIN_H
