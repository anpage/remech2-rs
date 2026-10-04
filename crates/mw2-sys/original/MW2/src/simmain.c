#include "simmain.h"

#include "ai.h"
#include "animation.h"
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
#include "directdraw.h"
#include "dispdibmode.h"
#include "displaybackend.h"
#include "dorcs.h"
#include "effectinfo.h"
#include "environment.h"
#include "error.h"
#include "eyepoint.h"
#include "fadepal.h"
#include "gamekeys.h"
#include "gdi.h"
#include "geocache.h"
#include "gpanim.h"
#include "inputmap.h"
#include "keyboard.h"
#include "loadres.h"
#include "mainmenu.h"
#include "menu.h"
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
#include "polydraw.h"
#include "random.h"
#include "refreshmode.h"
#include "render.h"
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

#include <excpt.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

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

// GLOBAL: MW2 0x100acb60
HWND g_gameWindow = NULL;

// GLOBAL: MW2 0x100acb64
HINSTANCE g_simModule = NULL;

// GLOBAL: MW2 0x100acb68
HANDLE g_primaryHeap = NULL;

// GLOBAL: MW2 0x100acb6c
MechS32 g_gameWindowWidth = 0;

// GLOBAL: MW2 0x100acb70
MechS32 g_gameWindowHeight = 0;

// GLOBAL: MW2 0x100acb74
MechS32 g_windowActive = 0;

// GLOBAL: MW2 0x100acb78
MechS32 g_drawModeReady = 0;

// GLOBAL: MW2 0x100acb7c
undefined4 g_shouldToggleFullscreen = 0;

// GLOBAL: MW2 0x100acb80
MechS32 g_desktopWidth = 0;

// GLOBAL: MW2 0x100acb84
MechS32 g_desktopHeight = 0;

// GLOBAL: MW2 0x100acb88
MechS32 g_simPaused = 0;

// GLOBAL: MW2 0x100acb8c
MechS32 g_pauseRequested = 0;

// GLOBAL: MW2 0x100acb90
undefined4 g_windowedSwitchPending = 0;

// GLOBAL: MW2 0x100acb94
MechS32 g_mouseOutsideClientWindow = 0;

// GLOBAL: MW2 0x100acb98
MechS32 g_goLaunch = 0;

// GLOBAL: MW2 0x100e9240
MechU32 g_windowedSwitchTime;

// GLOBAL: MW2 0x100e933c
MechU32 g_windowedSwitchDeadline;

// GLOBAL: MW2 0x1012b7c0
VideoDriverChoice g_videoDriverChoice;

// Matches except for the stack slots of seven locals (a consistent permutation; the original
// assigns them in declaration order, which VC++ 4.1 doesn't reproduce from this source). The
// operand order of the DoFirstObjtv loop test and of the network start test follows the unit's
// symbol table: both have flipped back and forth as declarations moved between units.
// FUNCTION: MW2 0x10066a50
int __stdcall SimMain(
	HINSTANCE p_module,
	undefined4 p_unk0x0c,
	LPSTR p_cmdLine,
	NetLaunchInfo* p_netLaunch,
	undefined4 p_isNetGameUnused,
	HWND p_hWnd
)
{
	MSG msg;
	void* palette;
	MechS32 hasPalette;
	int result;
	MechS32 i;
	char missionName[64];
	undefined4 unk0x28;
	MechS32 unk0x24;
	MechS32 seed;
	MechS32 quitLatched;

	quitLatched = 0;
	seed = 0;
	g_gameWindow = p_hWnd;
	g_simModule = p_module;
	g_desktopWidth = GetSystemMetrics(SM_CXSCREEN);
	g_desktopHeight = GetSystemMetrics(SM_CYSCREEN);
	g_primaryHeap = HeapCreate(HEAP_NO_SERIALIZE, 1000000, 0);
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
		if (_stricmp(g_mw2SndCfgData->m_videoDriver, "scan") == 0) {
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
	SendMessage(g_gameWindow, 0x41f, 0, 0);
	SendMessage(g_gameWindow, WM_ACTIVATEAPP, TRUE, 0);
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
		StartSupAnim(GetDeviceCaps(GetDC(g_gameWindow), NUMCOLORS) == -1 || g_windowMode == c_windowModeFullscreen);
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
		if (GetDeviceCaps(GetDC(g_gameWindow), NUMCOLORS) == -1 || g_windowMode == c_windowModeFullscreen) {
			DebugPrint("StartPalettes()\n");
			StartPalettes(0);
		}
		else {
			hasPalette = g_paletteResourceIds[0x10] != -1;
			if (hasPalette) {
				palette = LoadCachedResource(g_mw2PrjHandle, hasPalette, g_resourceTypeTags[c_resTagPal], 0);
				if (palette) {
					g_currentDisplayBackend->m_setPalette(0, 0x100, palette, 1);
				}
			}
		}

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

		while (ShowCursor(FALSE) >= 0) {
		}

		g_mouseOutsideClientWindow = 0;
		if (!g_refreshModeFallback) {
			g_goLaunch |= 2;
		}

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
			if (g_refreshModeFallback) {
				while (g_refreshModeFallback) {
					if (g_currentDisplayBackend->m_id == 0) {
						DdrawFill(0, 0, g_gameWindowWidth, g_gameWindowHeight, g_groundColor);
					}

					if ((g_windowActive ? g_currentDisplayBackend->m_acquireFramebuffer() : -1) == 0) {
						g_renderSettings.m_frameDrawCallback();
						DrawLocalPlayer();
						UpdateMenus();
						DrawTimedOverlays();
						DrawDebugOverlays();
					}

					Blit();
					ProfileRefreshModes();
				}
			}

			if (g_goLaunch & 0x80000000) {
				UpdateGeoCache();
			}

			AdvanceAnimations();
			UpdatePaletteFade();
			ApplyPendingPalette();
			if (g_windowActive && g_currentDisplayBackend->m_id == 0) {
				DdrawFill(0, 0, g_gameWindowWidth, g_gameWindowHeight, g_groundColor);
			}

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
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		}

		SendMessage(g_gameWindow, 0x41e, 0, 0);
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
			SendMessage(g_gameWindow, 0x41e, 0, 0);
			MessageBox(NULL, "Attempting to shutdown from an unknown fatal error.", "MECHWARRIOR 2", MB_ICONHAND);
			AIL_shutdown();
		}
	}

	HeapDestroy(g_primaryHeap);
	g_primaryHeap = NULL;
	while (ShowCursor(TRUE) < 1) {
	}

	result = g_fledToWindows ? 0xff : 0;
	return result;
}

// Operand order: the timer test's time > g_windowedSwitchDeadline loads g_windowedSwitchDeadline
// first in the original. The test's comparisons follow the unit's symbol table: the other two flipped when
// the shell's Miles declarations joined mss.h and back when the unit's declarations moved into
// headers.
// FUNCTION: MW2 0x10067757
LRESULT CALLBACK SimWindowProc(HWND p_hWnd, UINT p_msg, WPARAM p_wParam, LPARAM p_lParam)
{
	WINDOWPOS* windowPos;

	if (p_msg >= WM_KEYFIRST && p_msg <= WM_KEYLAST) {
		HandleKeyboardMessages(p_msg, p_wParam, p_lParam);
		return 0;
	}

	switch (p_msg) {
	case WM_ACTIVATEAPP:
		if (g_shouldQuit) {
			break;
		}

		g_windowActive = p_wParam;
		if (g_windowActive == TRUE) {
			KeyboardClearKeyStates();
			if (g_desktopWidth <= 640 && g_desktopHeight <= 480) {
				if (g_shouldToggleFullscreen == TRUE) {
					ToggleFullScreen();
					g_shouldToggleFullscreen = FALSE;
					ShowWindow(g_gameWindow, SW_RESTORE);
				}
				else if (g_currentDisplayBackend && g_currentDisplayBackend->m_id == 1) {
					ShowWindow(g_gameWindow, SW_SHOWNOACTIVATE);
				}
			}
			else {
				if (g_windowMode == c_windowModeFullscreen) {
					ShowWindow(g_gameWindow, SW_RESTORE);
				}
			}
		}
		else {
			if (g_desktopWidth <= 640 && g_desktopHeight <= 480) {
				if (g_currentDisplayBackend && g_currentDisplayBackend->m_id == 0 &&
					g_shouldToggleFullscreen == FALSE) {
					ShowWindow(g_gameWindow, SW_MINIMIZE);
					g_shouldToggleFullscreen = TRUE;
					ToggleFullScreen();
				}
				else if (g_currentDisplayBackend && g_currentDisplayBackend->m_id == 1) {
					ShowWindow(g_gameWindow, SW_HIDE);
				}
			}
			else {
				if (g_windowMode == c_windowModeFullscreen) {
					ShowWindow(g_gameWindow, SW_MINIMIZE);
				}
			}
		}

		if (g_currentDisplayBackend) {
			g_currentDisplayBackend->m_setPalette(0, 0x100, g_paletteColors, 1);
		}
		return 0;
	case WM_PAINT:
		if (g_drawModeReady && g_windowMode == c_windowModeWindowed) {
			g_currentRefreshMode->m_flip();
			ValidateRect(p_hWnd, NULL);
			return 0;
		}
		else {
			break;
		}
	case WM_NCMOUSEMOVE:
		if (g_mouseOutsideClientWindow == FALSE && g_windowMode == c_windowModeWindowed) {
			while (ShowCursor(TRUE) < 0) {
			}
			g_mouseOutsideClientWindow = TRUE;
		}
		break;
	case WM_MOUSEMOVE:
		if (g_mouseOutsideClientWindow) {
			if (!g_simPaused || GetMenuSlotState(4) == 1) {
				while (ShowCursor(FALSE) >= 0) {
				}
				g_mouseOutsideClientWindow = FALSE;
			}
		}
		return 0;
	case WM_WINDOWPOSCHANGING:
		windowPos = (WINDOWPOS*) p_lParam;
		if (g_windowedSwitchPending) {
			LONG time;

			time = GetMessageTime();
			if (time > g_windowedSwitchDeadline &&
				(g_windowedSwitchDeadline > g_windowedSwitchTime || time < g_windowedSwitchTime)) {
				g_windowedSwitchPending = FALSE;
			}
			else {
				windowPos->x = g_windowedRect.left;
				windowPos->y = g_windowedRect.top;
				windowPos->cx = g_windowedRect.right;
				windowPos->cy = g_windowedRect.bottom;
			}
			return 0;
		}
		else {
			break;
		}
	case WM_QUERYNEWPALETTE:
		if (g_currentDisplayBackend) {
			g_currentDisplayBackend->m_setPalette(0, 0x100, g_paletteColors, 1);
			return 1;
		}
		else {
			return 0;
		}
	case WM_CLOSE:
		g_shouldQuit = TRUE;
		g_quitStage += 2;
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		break;
	}

	return DefWindowProc(p_hWnd, p_msg, p_wParam, p_lParam);
}

// FUNCTION: MW2 0x10067bbc
void HandleMessages(void)
{
	MSG msg;

	if (!g_windowActive) {
		WaitMessage();
	}

	if (!g_shouldQuit && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		while (!g_mouseOutsideClientWindow && msg.message >= WM_MOUSEFIRST && msg.message <= WM_MBUTTONDBLCLK) {
			PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		}

		if (msg.hwnd != NULL && msg.message == WM_QUIT) {
			g_shouldQuit = TRUE;
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
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
			while (ShowCursor(FALSE) >= 0) {
			}

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
			while (ShowCursor(TRUE) < 0) {
			}

			g_mouseOutsideClientWindow = TRUE;
		}

		DebugPrint("WinMain(4): pause_timer(TRUE)");
		PauseTimer(0x80, TRUE);
		PauseAudio();
		DisableGameplayInput();
		g_simPaused = TRUE;
	}
}

// FUNCTION: MW2 0x10067e23
void SetGameResolution(char* p_driverName)
{
	if (_strcmpi(p_driverName, "MCGA.DLL") == 0) {
		g_gameWindowWidth = 320;
		g_gameWindowHeight = 200;
	}
	else if (_strcmpi(p_driverName, "VESA480.DLL") == 0) {
		g_gameWindowWidth = 640;
		g_gameWindowHeight = 480;
	}
	else if (_strcmpi(p_driverName, "VESA768.DLL") == 0) {
		g_gameWindowWidth = 1024;
		g_gameWindowHeight = 768;
	}
	else {
		g_gameWindowWidth = 320;
		g_gameWindowHeight = 200;
	}
}
