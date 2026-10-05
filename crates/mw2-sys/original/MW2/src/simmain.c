#include "simmain.h"

#include "ai.h"
#include "animation.h"
#include "app.h"
#include "audio.h"
#include "brightness.h"
#include "callbacks.h"
#include "clock.h"
#include "cmdline.h"
#include "cockpit.h"
#include "collision.h"
#include "commandmenu.h"
#include "commandpointmenu.h"
#include "compat.h"
#include "config.h"
#include "debris.h"
#include "debugprint.h"
#include "decomp.h"
#include "displaybackend.h"
#include "dorcs.h"
#include "effectinfo.h"
#include "environment.h"
#include "error.h"
#include "eyepoint.h"
#include "fadepal.h"
#include "gamekeys.h"
#include "geocache.h"
#include "gpanim.h"
#include "inputmap.h"
#include "keyboard.h"
#include "loadres.h"
#include "log.h"
#include "mainmenu.h"
#include "menu.h"
#include "messages.h"
#include "missionaudio.h"
#include "mss.h"
#include "mw2log.h"
#include "mw2prj.h"
#include "netlaunchinfo.h"
#include "network.h"
#include "objective.h"
#include "overlay.h"
#include "palette.h"
#include "palettecolor.h"
#include "pausebanner.h"
#include "perf.h"
#include "players.h"
#include "point.h"
#include "pointer.h"
#include "polydraw.h"
#include "random.h"
#include "refreshmode.h"
#include "render.h"
#include "resetglobals.h"
#include "resource.h"
#include "screenscale.h"
#include "setres.h"
#include "settings.h"
#include "shots.h"
#include "speech.h"
#include "startup.h"
#include "staticmem.h"
#include "supanim.h"
#include "targeting.h"
#include "ticks.h"
#include "timedoverlays.h"
#include "types.h"
#include "videodriverchoice.h"
#include "weapons.h"
#include "world.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

DECOMP_SIZE_ASSERT(SoundConfig, 0x3c)
DECOMP_SIZE_ASSERT(StarMission, 0x3c0a)
DECOMP_SIZE_ASSERT(MissionObjective, 0x13f)

// GLOBAL: MW2 0x100acb18
MechS32 g_shouldQuit = 0;

// GLOBAL: MW2 0x100acb1c
MechS32 g_quitStage = 0;

// GLOBAL: MW2 0x100acb20
TimedCallback* g_detachedTasks = NULL;

// GLOBAL: MW2 0x100acb24
DifficultyCfg* g_difficulty = NULL;

// GLOBAL: MW2 0x100acb28
MechS32 g_localPlayerId = 0;

// GLOBAL: MW2 0x100acb2c
MechS32 g_remoteWaitTime = 0;

// Set when the local player starts on the autopilot (FirstMech).
// GLOBAL: MW2 0x100acb34
MechS32 g_startOnAutopilot = 0;

// GLOBAL: MW2 0x100acb68
MechHeap* g_primaryHeap = NULL;

// GLOBAL: MW2 0x100acb6c
MechS32 g_gameWindowWidth = 0;

// GLOBAL: MW2 0x100acb70
MechS32 g_gameWindowHeight = 0;

// GLOBAL: MW2 0x100acb74
MechS32 g_windowActive = 0;

// GLOBAL: MW2 0x100acb78
MechS32 g_drawModeReady = 0;

// GLOBAL: MW2 0x100acb88
MechS32 g_simPaused = 0;

// GLOBAL: MW2 0x100acb8c
MechS32 g_pauseRequested = 0;

// GLOBAL: MW2 0x100acb94
MechS32 g_mouseOutsideClientWindow = 0;

// GLOBAL: MW2 0x100acb98
MechS32 g_goLaunch = 0;

// GLOBAL: MW2 0x1012b7c0
VideoDriverChoice g_videoDriverChoice;

static MECH_INTPTR SimHandleMessage(MechU32 p_msg, size_t p_wParam, MECH_INTPTR p_lParam);

// Matches except for the stack slots of seven locals (a consistent permutation; the original
// assigns them in declaration order, which VC++ 4.1 doesn't reproduce from this source). The
// operand order of the DoFirstObjtv loop test and of the network start test follows the unit's
// symbol table: both have flipped back and forth as declarations moved between units.
// FUNCTION: MW2 0x10066a50
int SimMain(char* p_cmdLine, NetLaunchInfo* p_netLaunch)
{
	MechMessage msg;
	int result;
	MechS32 i;
	char missionName[64];
	undefined4 unk0x28;
	MechS32 unk0x24;
	MechS32 seed;
	MechS32 quitLatched;

	// The original was loaded fresh for each mission.
	ResetSimGlobals();

	quitLatched = 0;
	seed = 0;
	g_primaryHeap = MechHeapCreate();
	if (g_primaryHeap == NULL) {
		Error(9, "Insufficient memory available.");
	}

	unk0x24 = 1;
	unk0x28 = 0;
	if (getenv("MECHWARRIOR")) {
		strcpy(g_gameDir, getenv("MECHWARRIOR"));
	}

	if (LoadSndCfg("mw2snd.cfg", &g_mw2SndCfgData) == -1) {
		if (g_mw2SndCfgData == NULL) {
			Error(0x11, "%s", "mw2snd.cfg");
		}
		else {
			*g_mw2SndCfgData = g_soundConfig;
		}
	}
	else {
		g_soundConfig = *g_mw2SndCfgData;
	}

	g_displayBrightness = g_brightnessSetting = g_mw2SndCfgData->m_displayBrightness;
	g_videoDriverChoice.m_flags = 0;
	if (g_mw2SndCfgData->m_videoDriver[0]) {
		g_videoDriverChoice.m_flags |= 1;
		if (strcasecmp(g_mw2SndCfgData->m_videoDriver, "scan") == 0) {
			g_videoDriverChoice.m_name[0] = 0;
		}
		else {
			strncpy(g_videoDriverChoice.m_name, g_mw2SndCfgData->m_videoDriver, 12);
			g_videoDriverChoice.m_name[12] = 0;
		}
	}

	if (!ProcessCmdLineArgs(p_cmdLine, &unk0x28, missionName)) {
		return 0;
	}

	if (StartupCheckStub()) {
		Error(0x51, NULL);
	}

	SetGameResolution(g_videoDriverChoice.m_name);
	if (g_logFileEnabled) {
		OpenMw2Log();
	}

	InitRefreshMode(5, 0, &g_mainPixelBuffer, 640, 480, 0);
	// The original sent the launcher's window 0x41f, to have it pass its messages to SimWindowProc,
	// and then told itself the window was active.
	while (MechPeekMessage(&msg, 0, 0, TRUE)) {
	}
	MechSetMessageHandler(SimHandleMessage);
	MechSendMessage(c_mechMsgActivateApp, MechAppActive(), 0);
	if (!InitDisplayGeometry()) {
		Error(0x50, NULL);
	}

	if (p_netLaunch) {
		g_isNetworkGame = 1;
		if (p_netLaunch->m_localPlayerId == 1) {
			g_netRole = 1;
		}
		else {
			g_netRole = 2;
		}

		strncpy(missionName, p_netLaunch->m_missionName, sizeof(missionName));
		if (LoadDifficultyCfg("MW2NET.CFG", &g_difficulty) == -1 || g_difficulty == NULL) {
			Error(0x11, "%s", "MW2NET.CFG");
		}
	}
	else {
		if (LoadDifficultyCfg("mw2dif.cfg", &g_difficulty) == -1 || g_difficulty == NULL) {
			Error(0x11, "%s", "mw2dif.cfg");
		}
	}

	DebugPrint("FirstClock()\n");
	__try {
		FirstClock();
		DebugPrint("StartSupAnim()\n");
		// The original passed whether the desktop had more than 256 colors or the game was full
		// screen.
		StartSupAnim(TRUE);
		DebugPrint("FirstResource()\n");
		FirstResource();
		DebugPrint("InitStaticMem()\n");
		InitStaticMem(missionName);
		DebugPrint("generate_gammas()\n");
		InitGammaTable();
		DebugPrint("InitRandom()\n");
		InitRandom(seed);
		DebugPrint("FirstAudio()\n");
		FirstAudio();
		DebugPrint("FirstRender()\n");
		FirstRender();
		DebugPrint("FirstNetwork()\n");
		FirstNetwork(p_netLaunch);
		DebugPrint("ResetClocks()\n");
		ResetClocks();
		DebugPrint("WinMain(1): pause_timer(TRUE)");
		PauseTimer(0x80, TRUE);
		DebugPrint("FirstShots()\n");
		FirstShots();
		DebugPrint("ZeroGamethings()\n");
		ZeroGamethings();
		DebugPrint("ZeroChunx()\n");
		ZeroChunx();
		DebugPrint("LoadWorld()\n");
		LoadWorld(missionName);
		DebugPrint("CollectMissionAudio()\n");
		CollectMissionAudio();
		DebugPrint("SetRes()\n");
		SetRes();
		DebugPrint("FirstEnvironment()\n");
		FirstEnvironment();
		DebugPrint("FirstStaticCache()\n");
		FirstStaticCache();
		DebugPrint("CachePreloads()\n");
		CachePreloads();
		DebugPrint("AfterWorldLoader()\n");
		AfterWorldLoader();
		DebugPrint("FirstGPAnim()\n");
		FirstGPAnim();
		DebugPrint("FirstEyepoint()\n");
		FirstEyepoint();
		DebugPrint("FirstInputs()\n");
		FirstInputs();
		DebugPrint("FirstMenu()\n");
		FirstMenu();
		DebugPrint("RegisterMenu()...\n");
		RegisterMenu(4);
		RegisterMenu(5);
		RegisterMenu(1);
		RegisterMenu(7);
		RegisterMenu(8);
		RegisterMenu(3);
		DebugPrint("DoFirstObjtv()...\n");
		for (i = 0; i < g_objectiveCount; i++) {
			DoFirstObjtv(&g_objectiveTable[i], i);
		}

		DebugPrint("FirstClassFunctions()\n");
		FirstClassFunctions();
		DebugPrint("FirstAI()\n");
		FirstAI();
		DebugPrint("FirstExternalCtrl()\n");
		if (!FirstExternalCtrl()) {
			g_shouldQuit = 1;
			g_quitStage = 3;
		}

		DebugPrint("UpdateGeoCache()\n");
		UpdateGeoCache();
		DebugPrint("SecondRender()\n");
		SecondRender();
		DebugPrint("FirstPerfSetting()\n");
		FirstPerfSetting();
		if (g_missionTimerStopped) {
			DebugPrint("SimEntranceDbug()\n");
			SimEntranceDbug(missionName, 0);
		}

		DebugPrint("StopSupAnim()\n");
		StopSupAnim();
		// On a 256-color desktop in a window the original only loaded the mission's palette here.
		DebugPrint("StartPalettes()\n");
		StartPalettes(0);

		g_drawModeReady = 1;
		DebugPrint("InitDrawMode()\n");
		if (!InitRefreshMode(
				g_drawModeIndex,
				g_initDrawModeParam2,
				&g_mainPixelBuffer,
				g_gameWindowWidth,
				g_gameWindowHeight,
				0
			)) {
			Error(0x50, "Error profiling video modes.");
		}

		MechMouseShowCursor(FALSE);

		g_mouseOutsideClientWindow = 0;
		// The original waited here for the refresh modes to be profiled.
		g_goLaunch |= 2;

		g_careerRecord.m_outcome = 1;
		g_careerRecord.m_winner = -1;
		while (g_quitStage < 3) {
			if (g_goLaunch == 3) {
				DebugPrint("GoLaunch == GO_READY\n");
				g_goLaunch |= 0x80000000;
				DebugPrint("WinMain(2): pause_timer(false)");
				PauseTimer(0x80, FALSE);
				StartMissionMusic();
				g_statusMessage = 0;
			}
			else if (g_isNetworkGame && !(g_goLaunch & 0x80000000)) {
				if (g_remoteWaitTime == 0) {
					g_remoteWaitTime = g_realClock + 0xb5;
				}
				else if (g_remoteWaitTime < g_realClock) {
					g_statusMessage = 3;
				}
			}

			HandleMessages();
			UpdateNetwork();
			NextClock();
			UpdateInputs();
			UpdateMenuKey();
			HandleGameKeys(0, 0, 0);
			RunTimedCallbacks(&g_detachedTasks);
			if (!g_isNetworkGame || (g_goLaunch & 0x80000000)) {
				UpdateAllPlayers();
			}

			UpdateEyepoint();
			UpdateAllShots();
			UpdateEffects();
			UpdateLocalPlayer();
			LateUpdateAllPlayers();
			UpdateDebris();
			// While the refresh modes were being profiled the original drew and timed frames here
			// (ProfileRefreshModes).
			if (g_goLaunch & 0x80000000) {
				UpdateGeoCache();
			}

			AdvanceAnimations();
			UpdatePaletteFade();
			ApplyPendingPalette();
			if ((g_windowActive ? g_currentDisplayBackend->m_acquireFramebuffer() : -1) == 0) {
				g_renderSettings.m_frameDrawCallback();
				DrawLocalPlayer();
				UpdateMenus();
				DrawTimedOverlays();
				DrawDebugOverlays();
				if (g_simPaused && g_pauseRequested) {
					DrawPausedBanner();
				}
			}

			Blit();
			if (g_goLaunch & 0x80000000) {
				DoAudio();
				AdvanceSpeechQueue();
			}

			UpdateTimeOfDay();
			UpdateObjectives();
			LoopCdMusic();
			UpdatePauseState();
			if (g_shouldQuit && !quitLatched) {
				g_quitStage++;
				quitLatched = 1;
				g_drawModeReady = 0;
			}

			g_goLaunch |= 2;
		}

		FadeToEndPalette(g_careerRecord.m_outcome & 4);
		if ((g_windowActive ? g_currentDisplayBackend->m_acquireFramebuffer() : -1) == 0) {
			VFX_pane_wipe(&g_currentPane, 0);
		}

		DebugPrint("Calling Blit()\n");
		Blit();
		while (MechPeekMessage(&msg, 0, 0, TRUE)) {
		}

		// The original sent the launcher's window 0x41e, to have it handle its own messages again.
		MechSetMessageHandler(NULL);
		SaveCareerRecord();
		ShutdownAllPlayers();
		ShutdownNetwork();
		ShutdownAudio();
		FreeMenus();
		StopTimers();
		ShutdownMw2Prj();
		ShutdownRender();
		CloseInputDevices();
		if (g_logFileEnabled) {
			CloseMw2Log();
		}

		DebugPrint("Calling EndTheMission()\n");
		EndTheMission1();
		EndTheMission2();
	}
	__finally {
		if (AbnormalTermination() && g_ticksTimerInitialized) {
			MechSetMessageHandler(NULL);
			MechLogError("Attempting to shutdown from an unknown fatal error.");
			AIL_shutdown();
		}
	}

	MechHeapDestroy(g_primaryHeap);
	g_primaryHeap = NULL;
	MechMouseShowCursor(TRUE);

	result = g_fledToWindows ? 0xff : 0;
	return result;
}

// The simulator's message handler, originally its window procedure (SimWindowProc).
// FUNCTION: MW2 0x10067757
static MECH_INTPTR SimHandleMessage(MechU32 p_msg, size_t p_wParam, MECH_INTPTR p_lParam)
{
	if (p_msg >= c_mechMsgKeyFirst && p_msg <= c_mechMsgKeyLast) {
		HandleKeyboardMessages(p_msg, p_wParam, p_lParam);
		return 0;
	}

	switch (p_msg) {
	case c_mechMsgActivateApp:
		if (g_shouldQuit) {
			break;
		}

		g_windowActive = p_wParam;
		if (g_windowActive == TRUE) {
			KeyboardClearKeyStates();
		}

		// The original also restored or minimized the window here when it was full screen, and on a
		// 640x480 desktop left and returned to DirectDraw's full screen mode or showed and hid
		// DisplayDib's window.
		if (g_currentDisplayBackend) {
			g_currentDisplayBackend->m_setPalette(0, 0x100, g_paletteColors, 1);
		}
		return 0;
	case c_mechMsgPaint:
		if (g_drawModeReady && g_windowMode == c_windowModeWindowed) {
			g_currentRefreshMode->m_flip();
			return 0;
		}
		else {
			break;
		}
	// On WM_NCMOUSEMOVE in a window the original showed the cursor and set
	// g_mouseOutsideClientWindow.
	case c_mechMsgMouseMove:
		if (g_mouseOutsideClientWindow) {
			if (!g_simPaused || GetMenuSlotState(4) == 1) {
				MechMouseShowCursor(FALSE);
				g_mouseOutsideClientWindow = FALSE;
			}
		}
		return 0;
	// For three seconds after DirectDraw switched to a window, the original's WM_WINDOWPOSCHANGING
	// held the window at its windowed position and size.
	// On WM_QUERYNEWPALETTE the original reloaded the palette.
	case c_mechMsgClose:
		g_shouldQuit = TRUE;
		g_quitStage += 2;
		return 0;
	case c_mechMsgDestroy:
		MechPostMessage(c_mechMsgQuit, 0, 0);
		return 0;
		break;
	}

	return 0;
}

// FUNCTION: MW2 0x10067bbc
void HandleMessages(void)
{
	MechMessage msg;

	if (!g_windowActive) {
		MechAppWait();
	}
	else {
		MechAppPump();
	}

	if (!g_shouldQuit && MechPeekMessage(&msg, 0, 0, TRUE)) {
		// The original skipped every mouse message here, and without looking at whether there was
		// another message to take.
		while (!g_mouseOutsideClientWindow && msg.m_message == c_mechMsgMouseMove) {
			if (!MechPeekMessage(&msg, 0, 0, TRUE)) {
				return;
			}
		}

		if (msg.m_message == c_mechMsgQuit) {
			g_shouldQuit = TRUE;
		}
		else {
			MechSendMessage(msg.m_message, msg.m_wParam, msg.m_lParam);
		}
	}
}

// FUNCTION: MW2 0x10067c79
void UpdatePauseState(void)
{
	if (g_pauseRequested) {
		if (g_simPaused && g_localSteering.m_keyCode) {
			PlayResumeSound();
			g_localSteering.m_keyCode = 0;
			g_pauseRequested = FALSE;
		}
		else if (g_netRole || GetMenuSlotState(4)) {
			g_pauseRequested = FALSE;
		}
	}

	if (!GetMenuSlotState(4) && g_windowActive && !g_pauseRequested) {
		if (g_simPaused) {
			MechMouseShowCursor(FALSE);

			g_mouseOutsideClientWindow = FALSE;
			DebugPrint("WinMain(3): pause_timer(false)");
			PauseTimer(0x80, FALSE);
			ResumeAudio();
			EnableGameplayInput();
			g_simPaused = FALSE;
		}
	}
	else if (!g_simPaused && !g_netRole) {
		if (g_pauseRequested) {
			PlayPauseSound();
			g_localSteering.m_keyCode = 0;
		}

		if (!GetMenuSlotState(4) && g_windowMode != c_windowModeFullscreen) {
			MechMouseShowCursor(TRUE);

			g_mouseOutsideClientWindow = TRUE;
		}

		DebugPrint("WinMain(4): pause_timer(TRUE)");
		PauseTimer(0x80, TRUE);
		PauseAudio();
		DisableGameplayInput();
		g_simPaused = TRUE;
	}
}

// SetGameResolution is implemented on the Rust side (src/sim/window.rs). The original's low
// resolution was 320x200, shown at 4:3 with tall pixels.
