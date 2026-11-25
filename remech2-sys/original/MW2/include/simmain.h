#ifndef SIMMAIN_H
#define SIMMAIN_H

#include "decomp.h"
#include "heap.h"
#include "mission.h"
#include "types.h"

struct DifficultyCfg;
struct NetLaunchInfo;
struct TimedCallback;

// The functions and globals of simmain.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern const MissionLaunch* g_missionLaunch;
	extern MissionReport* g_missionReport;
	extern struct DifficultyCfg* g_difficulty;
	extern struct TimedCallback* g_detachedTasks;
	extern int32_t g_shouldQuit;
	extern int32_t g_goLaunch;
	extern int32_t g_quitStage;
	extern int32_t g_localPlayerId;
	extern int32_t g_startOnAutopilot;
	extern MechHeap* g_primaryHeap;
	extern int32_t g_gameWindowWidth;
	extern int32_t g_gameWindowHeight;
	extern int32_t g_simPaused;
	extern int32_t g_pauseRequested;
	extern int32_t g_mouseOutsideClientWindow;
	extern int32_t g_remoteWaitTime;
	extern int32_t g_drawModeReady;

	int SimMain(
		char* p_cmdLine,
		const MissionLaunch* p_launch,
		struct NetLaunchInfo* p_netLaunch,
		MissionReport* p_report
	);
	void HandleMessages(void);
	void UpdatePauseState(void);
	// Implemented on the Rust side (src/sim/window.rs)
	void SetGameResolution(void);

#ifdef __cplusplus
}
#endif

#endif // SIMMAIN_H
