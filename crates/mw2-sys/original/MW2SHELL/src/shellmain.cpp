#include "shellmain.h"

#include "archivereader.h"
#include "audiosubsystem.h"
#include "briefing.h"
#include "cadettraining.h"
#include "cdcheck.h"
#include "clanhall.h"
#include "cockpitcontrols.h"
#include "credits.h"
#include "debrief.h"
#include "debugout.h"
#include "debugprint.h"
#include "decomp.h"
#include "font.h"
#include "formation.h"
#include "hallofhonor.h"
#include "keyboard.h"
#include "keyboardinput.h"
#include "mainmenu.h"
#include "mechbay.h"
#include "mechvariant.h"
#include "menudata.h"
#include "midisequence.h"
#include "missionui.h"
#include "mousestate.h"
#include "options.h"
#include "pilotrecord.h"
#include "pilotroster.h"
#include "projectarchive.h"
#include "readyroom.h"
#include "refreshmode.h"
#include "rosterscreen.h"
#include "shellglobals.h"
#include "simhandoff.h"
#include "textglyph.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

// The current screen's function (RegisterScreenFunction): ShellMain's loop calls it every frame
// with c_msgScreenFrame, and a menu command or leaving the shell with the message to move on to.
// GLOBAL: MW2SHELL 0x10062978
void (*g_screenFunction)(
	TMPackDataBase* p_database,
	MechS32* p_campaign,
	MechU8* p_pilotChosen,
	char** p_scenario,
	MechS32 p_msg
) = NULL;

// The screen a menu command opened over the current one (RegisterMenuFunction), called instead of
// g_screenFunction while set: every frame with p_active TRUE, and with FALSE to close it
// (CloseMenuFunction).
// GLOBAL: MW2SHELL 0x1006297c
void (*g_menuFunction)(MechS32 p_active) = NULL;

// GLOBAL: MW2SHELL 0x10062980
MidiSequence* g_midiBackgroundMusic = NULL;

// GLOBAL: MW2SHELL 0x10062984
MechS32 g_cursorHidden = 0;

// PlayMidiSong's state: whether the base song id is set, the base, and the song playing.
// GLOBAL: MW2SHELL 0x10062988
MechU8 g_midiSongBaseSet = 0;

// GLOBAL: MW2SHELL 0x1006298c
MechS32 g_midiSongBase = 0;

// GLOBAL: MW2SHELL 0x10062990
MechS32 g_midiSongPlaying = 0;

// GLOBAL: MW2SHELL 0x1007cc84
char* g_scenario;

// GLOBAL: MW2SHELL 0x1007cc88
MechS32 g_selectedCampaign;

// GLOBAL: MW2SHELL 0x1007cc8c
MechU8 g_pilotChosen;

void PlayMidiSong(UINT p_msg, MechS32 p_campaign);
void ParseCommandLineFlags(char* p_cmdLine);
BOOL CALLBACK LittleMoviesDialogProc(HWND p_hWnd, UINT p_msg, WPARAM p_wParam, LPARAM p_lParam);
void RunScreenFrame();
void CloseMenuFunction();

// Matches except for the stack slots of helpFile, msg and mouseY (a consistent permutation).
// FUNCTION: MW2SHELL 0x1000e670
extern "C" LRESULT CALLBACK ShellWindowProc(HWND p_hWnd, UINT p_msg, WPARAM p_wParam, LPARAM p_lParam)
{
	char helpFile[32];
	UINT msg;
	MechS32 mouseY;
	PAINTSTRUCT paint;

	if (p_msg >= WM_KEYFIRST && p_msg <= WM_KEYLAST) {
		HandleKeyboardMessages(p_msg, p_wParam, p_lParam);
		return 0;
	}

	switch (p_msg) {
	case WM_ACTIVATEAPP:
		g_windowActive = p_wParam;
		if (g_windowActive) {
			if (IsFullscreenVideoPlaying()) {
				ResumeFullscreenVideo();
			}

			if (g_midiBackgroundMusic) {
				g_midiBackgroundMusic->Start();
			}

			SetFocus(g_gameWindow);
		}
		else {
			if (IsFullscreenVideoPlaying()) {
				PauseFullscreenVideo();
			}

			if (g_midiBackgroundMusic) {
				g_midiBackgroundMusic->Stop();
			}
		}
		return 0;
	case WM_PALETTECHANGED:
		if (g_gameWindow == p_hWnd) {
			return 0;
		}
	case WM_QUERYNEWPALETTE:
		if (g_videoDriver) {
			g_videoDriver->UpdatePalette();
			if (g_drawFmv) {
				g_videoDriver->ExpandRectBySize(0, 0, 320, 200);
				g_videoDriver->DrawFmv();
			}
			else {
				g_videoDriver->ExpandRectBySize(0, 0, 640, 480);
				g_videoDriver->DrawShell();
			}
			return 1;
		}
		else {
			return 0;
		}
	case WM_PAINT:
		BeginPaint(p_hWnd, &paint);
		if (g_videoDriver) {
			if (g_drawFmv) {
				g_videoDriver->ExpandRectBySize(0, 0, 320, 200);
				g_videoDriver->DrawFmv();
			}
			else {
				g_videoDriver
					->ExpandRect(paint.rcPaint.left, paint.rcPaint.top, paint.rcPaint.right, paint.rcPaint.bottom);
				g_videoDriver->DrawShell();
			}
		}
		EndPaint(p_hWnd, &paint);
		return 0;
	case WM_NCMOUSEMOVE:
		if (g_cursorHidden) {
			while (ShowCursor(TRUE) < 0)
				;
			g_cursorHidden = FALSE;
		}
		return 0;
	case WM_MOUSEMOVE:
		if (IsFullscreenVideoPlaying() && !g_cursorHidden) {
			while (ShowCursor(FALSE) >= 0)
				;
			g_cursorHidden = TRUE;
		}

		mouseY = ((MechU32) p_lParam >> 16) & 0xffff;
		if (g_menuVisible && g_windowMode == 1 && mouseY > 2) {
			SetMenu(p_hWnd, NULL);
			g_menuVisible = FALSE;
		}
		else if (
			g_windowMode == 1 && !g_menuVisible && !IsFullscreenVideoPlaying() && GetSystemMetrics(SM_CYMENU) >= mouseY
		) {
			SetMenu(p_hWnd, g_windowMenu);
			g_menuVisible = TRUE;
			g_videoDriver->ExpandRect(0, 0, 640, 480);
			g_videoDriver->DrawShell();
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_COMMAND:
		if (IsFullscreenVideoPlaying()) {
			CloseVideo(0);
		}

		switch (LOWORD(p_wParam)) {
		case c_menuNewAllegiance:
			CloseMenuFunction();
			if (g_screenFunction) {
				g_screenFunction(g_mw2Database, &g_selectedCampaign, &g_pilotChosen, &g_scenario, c_msgMainMenu);
			}
			else {
				PostMessage(g_gameWindow, c_msgMainMenu, c_msgMainMenu, 0);
			}
			break;
		case c_menuHallOfHonor:
			CloseMenuFunction();
			EnableMenuItem(g_windowMenu, c_menuHallOfHonor, MF_GRAYED);
			DrawHallOfHonor();
			g_menuDialogOpen = TRUE;
			break;
		case c_menuQuickTips:
			CheckMenuItem(g_windowMenu, p_wParam, ~GetMenuState(g_windowMenu, p_wParam, MF_BYCOMMAND) & MF_CHECKED);
			g_quickTips = 1 - g_quickTips;
			break;
		case c_menuFleeToWindows:
			if (ShowDialog("Embrace cowardice?#Yes|No", 1) == 0) {
				CloseMenuFunction();
				g_videoDriver->ActivateFramebuffer();
				memset(g_videoDriver->m_backBuffer.m_buffer, 0, 640 * 480);
				if (g_screenFunction) {
					g_screenFunction(g_mw2Database, &g_selectedCampaign, &g_pilotChosen, &g_scenario, c_msgQuit);
				}
				else {
					PostMessage(g_gameWindow, c_msgQuit, 0, 0);
				}
				g_menuDialogOpen = TRUE;
			}
			break;
		case c_menuCombatVariables:
			CloseMenuFunction();
			EnableMenuItem(g_windowMenu, c_menuCombatVariables, MF_GRAYED);
			DrawOptions();
			g_menuDialogOpen = TRUE;
			break;
		case c_menuCockpitControls:
			CloseMenuFunction();
			EnableMenuItem(g_windowMenu, c_menuCockpitControls, MF_GRAYED);
			OpenCockpitControls();
			g_menuDialogOpen = TRUE;
			break;
		case c_menuMoviePlayback:
			DialogBoxParam(g_module, MAKEINTRESOURCE(138), g_gameWindow, (DLGPROC) LittleMoviesDialogProc, 0);
			break;
		case c_menuKeshik:
			CloseMenuFunction();
			EnableMenuItem(g_windowMenu, c_menuKeshik, MF_GRAYED);
			DrawCredits();
			g_menuDialogOpen = TRUE;
			break;
		case c_menuHelpContents:
			sprintf(helpFile, "%s", "mw2help.hlp");
			if (GetFileAttributes(helpFile) == -1) {
				sprintf(helpFile, "%s%s", g_dataDrivePath, "mw2help.hlp");
			}

			if (WinHelp(g_gameWindow, helpFile, HELP_CONTENTS, 0)) {
				g_helpRegistered = TRUE;
			}
			break;
		case c_menuTechnicalHelp:
			sprintf(helpFile, "%s", "tech.hlp");
			if (GetFileAttributes(helpFile) == -1) {
				sprintf(helpFile, "%s%s", g_dataDrivePath, "tech.hlp");
			}

			if (WinHelp(g_gameWindow, helpFile, HELP_CONTENTS, 0)) {
				g_helpRegistered = TRUE;
			}
			break;
		}
		return 0;
	case c_msgMainMenu:
		g_pilotChosen = FALSE;
		DrawMainMenu(g_mw2Database, &g_selectedCampaign);
		if (g_cursorHidden) {
			while (ShowCursor(TRUE) < 0)
				;
			g_cursorHidden = FALSE;
		}
		break;
	case c_msgTrials:
		g_selectedCampaign = 2;
		DrawMissionBriefing(g_mw2Database, &g_scenario, p_wParam);
		break;
	case c_msgClanHall:
		if (!g_pilotChosen) {
			switch (g_selectedCampaign) {
			case 0:
				BeginFullscreenVideo("aworgstr", c_msgPilotRoster, c_msgClanHall);
				break;
			case 1:
				BeginFullscreenVideo("ajfrgstr", c_msgPilotRoster, c_msgClanHall);
				break;
			}
			return 0;
		}
		else {
			if (g_cursorHidden) {
				while (ShowCursor(TRUE) < 0)
					;
				g_cursorHidden = FALSE;
			}

			DrawClanHall(g_mw2Database, g_selectedCampaign, g_pilotChosen, p_wParam);
		}
		break;
	case c_msgPilotRoster:
		if (g_cursorHidden) {
			while (ShowCursor(TRUE) < 0)
				;
			g_cursorHidden = FALSE;
		}

		DrawPilotRoster(g_mw2Database, g_selectedCampaign, &g_pilotChosen, &g_scenario);
		break;
	case c_msgArchive:
		DrawArchive(g_mw2Database, g_selectedCampaign, p_wParam);
		break;
	case c_msgCadetTraining:
		DrawCadetTraining(g_mw2Database, g_selectedCampaign, &g_scenario, p_wParam);
		break;
	case c_msgReadyRoom:
		if (g_selectedCampaign == 2) {
			DrawMissionBriefing(g_mw2Database, &g_scenario, p_wParam);
		}
		else {
			DrawReadyRoom(g_mw2Database, g_selectedCampaign, &g_scenario, p_wParam);
		}
		break;
	case c_msgMechBay:
		DrawMechBay(g_mw2Database, g_selectedCampaign, p_wParam);
		break;
	case c_msgStarConfig:
		DrawStarConfig(g_mw2Database, g_selectedCampaign);
		break;
	case c_msgBriefing:
		DrawBriefing(g_mw2Database, g_scenario, g_selectedCampaign);
		break;
	case c_msgLaunchSim:
		if (p_wParam == c_msgBriefing) {
			msg = c_msgDebrief;
		}
		else {
			msg = p_wParam;
		}

		if (g_runSim) {
			WriteSimHandoff(msg, g_selectedCampaign, g_pilotChosen, g_scenario);
			PostQuitMessage(c_msgQuitToSim);
			return 0;
		}
		else {
			PostMessage(p_hWnd, msg, c_msgLaunchSim, 0);
		}
		break;
	case c_msgDebrief:
		DrawMissionDebrief(g_mw2Database, g_selectedCampaign, &g_scenario);
		break;
	case c_msgLandingVideo:
		while (ShowCursor(FALSE) >= 0)
			;
		g_cursorHidden = TRUE;

		switch (g_selectedCampaign) {
		case 0:
			if (!PlayFullscreenVideo("mwoland", c_msgClanHall, c_msgLandingVideo)) {
				PlayFullscreenVideo("mjfland", c_msgClanHall, c_msgLandingVideo);
			}
			break;
		case 1:
			if (!PlayFullscreenVideo("mjfland", c_msgClanHall, c_msgLandingVideo)) {
				PlayFullscreenVideo("mwoland", c_msgClanHall, c_msgLandingVideo);
			}
			break;
		}
		break;
	case c_msgEndingVideo:
		while (ShowCursor(FALSE) >= 0)
			;
		g_cursorHidden = TRUE;

		switch (g_selectedCampaign) {
		case 0:
			if (!PlayFullscreenVideo("mend", c_msgClanHall, c_msgEndingVideo)) {
				PlayFullscreenVideo("mend2", c_msgClanHall, c_msgEndingVideo);
			}
			break;
		case 1:
			if (!PlayFullscreenVideo("mend2", c_msgClanHall, c_msgEndingVideo)) {
				PlayFullscreenVideo("mend", c_msgClanHall, c_msgEndingVideo);
			}
			break;
		}
		break;
	case c_msgQuit:
		WriteSimHandoff(c_msgQuit, g_selectedCampaign, g_pilotChosen, "exittos");
		PostQuitMessage(c_msgQuit);
		return 0;
	default:
		return DefWindowProc(p_hWnd, p_msg, p_wParam, p_lParam);
	}

	if (g_midiAudio) {
		PlayMidiSong(p_msg, g_selectedCampaign);
	}

	return 0;
}

// Matches except for the stack slots of itemData, unk0x14 and itemSize (a consistent
// permutation that VC++ 4.1 doesn't reproduce from this source).
// FUNCTION: MW2SHELL 0x1000f35e
extern "C" int __stdcall ShellMain(
	HINSTANCE p_hInstance,
	HINSTANCE p_hPrevInstance,
	char* p_cmdLine,
	int p_cmdShow,
	HWND p_hWnd
)
{
	void* itemData = NULL;
	MechS32 unk0x14 = c_msgQuit;
	MechS32 itemSize;
	MSG msg;
	BOOL fromSim = FALSE;

	g_primaryHeap = MechHeapCreate();
	if (g_primaryHeap == NULL) {
		MessageBox(NULL, "Insufficient memory available.", g_windowClassName, MB_ICONEXCLAMATION);
		return 0xff;
	}

	g_module = p_hInstance;
	g_gameWindow = p_hWnd;

	if (*p_cmdLine == '\0') {
		g_digitalAudio = 0;
		g_midiAudio = 0;
		g_unk0x1007123c = 0;
		g_runSim = 0;
	}
	else if (strstr(p_cmdLine, "sim") != NULL) {
		fromSim = TRUE;
	}

	ParseCommandLineFlags(p_cmdLine);
	g_dataDrivePath[0] = CdCheck();
	if (*p_cmdLine == '\0') {
		g_dataDrivePath[0] = '\0';
	}

	InitTextColorMaps();

	g_mw2Database = new TMPackDataBase(g_databaseName);
	g_audioSubsystem = new AudioSubsystem();
	g_windowWidth = 640;
	g_windowHeight = 480;
	g_videoDriver = new VideoDriver();

	g_windowMenu = LoadMenu(g_module, MAKEINTRESOURCE(0x68));
	if (g_windowMode != 1) {
		SetMenu(g_gameWindow, g_windowMenu);
	}

	if (!fromSim) {
		PlayFullscreenVideo("mintro", c_msgMainMenu, c_msgMainMenu);
	}

	g_mw2Database->GetDBItem(0x1a, &itemData, &itemSize);
	g_textFont = new Font(itemData, g_videoDriver);
	g_mw2Database->GetDBItem(0x1b, &itemData, &itemSize);
	g_titleFont = new Font(itemData, g_videoDriver);
	g_mw2Database->GetDBItem(0x1c, &itemData, &itemSize);
	g_buttonFont = new Font(itemData, g_videoDriver);
	g_mw2Database->GetDBItem(0x1e, &itemData, &itemSize);
	g_unk0x1007121c = new Font(itemData, g_videoDriver);
	g_mw2Database->GetDBItem(0x1f, &itemData, &itemSize);
	g_unk0x10071220 = new Font(itemData, g_videoDriver);
	g_defaultFont = g_textFont;
	g_mw2Database->GetDBItem(0x20, &itemData, &itemSize);
	g_bodyFont = new Font(itemData, g_videoDriver);
	g_archiveFont = g_bodyFont;
	g_textFont = g_bodyFont;
	g_defaultFont = g_textFont;

	g_mw2Database->GetDBItem(0x19, &g_unk0x10071200, &itemSize);
	g_mouseState = new MouseState(g_videoDriver, g_defaultFont, g_unk0x10071200);
	g_keyboardInput = new KeyboardInput();
	g_projectArchive = new ProjectArchive("MW2.PRJ");

	LoadPilotRoster();
	LoadDifficultyConfig();
	ReadSimHandoff(fromSim, &g_selectedCampaign, &g_pilotChosen, &g_scenario);
	ClipCursor(NULL);

	if (fromSim) {
		EnableShellMenu(g_windowMenu);
		while (ShowCursor(TRUE) < 0)
			;
		g_cursorHidden = FALSE;
	}
	else {
		while (ShowCursor(FALSE) >= 0)
			;
		g_cursorHidden = TRUE;
	}

	SendMessage(g_gameWindow, c_msgActivateShell, 0, 0);

	for (;;) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (g_windowActive) {
			g_mouseState->ReadMouseState();
			RunScreenFrame();
			if (g_drawFmv) {
				g_videoDriver->DrawFmv();
			}
			else {
				g_videoDriver->DrawShell();
			}
		}
	}

	SendMessage(g_gameWindow, c_msgActivateLauncher, 0, 0);
	SetMenu(g_gameWindow, NULL);
	g_videoDriver->ActivateFramebuffer();

	if (g_menuFunction) {
		g_menuFunction(FALSE);
	}

	if (g_screenFunction) {
		g_screenFunction(g_mw2Database, &g_selectedCampaign, &g_pilotChosen, &g_scenario, c_msgQuit);
	}

	CloseAllVideos();

	delete g_projectArchive;
	delete g_mw2Database;
	delete g_mouseState;
	delete g_keyboardInput;
	delete g_videoDriver;
	delete g_audioSubsystem;

	if (g_helpRegistered) {
		WinHelp(g_gameWindow, NULL, HELP_QUIT, 0);
	}

	MechHeapDestroy(g_primaryHeap);
	g_primaryHeap = NULL;

	if (msg.wParam == c_msgQuitToSim) {
		return 3;
	}
	else {
		return 0xff;
	}
}

// Handles one pending message, waiting for one while the window is inactive. Returns 0 on
// WM_QUIT (posting it again), 1 otherwise.
// FUNCTION: MW2SHELL 0x1000fe0d
MechS32 PumpMessage()
{
	MSG msg;

	if (!g_windowActive) {
		WaitMessage();
	}

	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			PostQuitMessage(msg.wParam);
			return 0;
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 1;
}

// Starts the song of the shell message p_msg for the campaign, unless it is already playing.
// Not 100%: the stack slots of size, data and result are permuted, and the table loads'
// displacements (each table less c_msgBriefing entries) land on unrelated data, which reccmp names
// after whatever symbol is there on each side.
// FUNCTION: MW2SHELL 0x1000fe86
void PlayMidiSong(UINT p_msg, MechS32 p_campaign)
{
	MechS32 size;
	void* data;
	MechS32 result;
	MechS32 song;

	switch (p_campaign) {
	case 0:
		song = g_wolfSongs[p_msg - c_msgBriefing];
		break;
	case 1:
		song = g_jadeFalconSongs[p_msg - c_msgBriefing];
		break;
	default:
		song = g_trialsSongs[p_msg - c_msgBriefing];
		break;
	}

	if (!song) {
		return;
	}

	if (song & 0x20000000) {
		if (g_midiBackgroundMusic) {
			delete g_midiBackgroundMusic;
			g_midiBackgroundMusic = NULL;
		}
		g_midiSongPlaying = 0;
		return;
	}

	if (song & 0x10000000) {
		if (g_midiBackgroundMusic) {
			delete g_midiBackgroundMusic;
			g_midiBackgroundMusic = NULL;
		}
		g_midiSongPlaying = 0;
	}

	song &= 0xffffff;
	if (!g_midiSongBaseSet) {
		g_midiSongBaseSet = 1;
		g_midiSongBase = 8;
	}
	song += g_midiSongBase;

	if (g_midiBackgroundMusic && g_midiBackgroundMusic->IsAnySequencePlaying() && song == g_midiSongPlaying) {
		return;
	}

	if (g_midiBackgroundMusic) {
		delete g_midiBackgroundMusic;
	}
	g_midiSongPlaying = song;
	result = g_mw2Database->GetDBItemLZ(g_midiSongPlaying, &data, &size);
	if (result != 1) {
		g_midiBackgroundMusic = new MidiSequence(g_audioSubsystem, data, size);
		g_midiBackgroundMusic->Start();
	}
}

// FUNCTION: MW2SHELL 0x10010137
void ParseCommandLineFlags(char* p_cmdLine)
{
	char* token;

	token = strtok(p_cmdLine, " ");
	while (token != NULL) {
		if (token[0] == '-') {
			switch (toupper(token[1])) {
			case 'X':
				if (token[2] == '=') {
					switch (toupper(token[3])) {
					case 'F':
						SetDebugOutputMode(4);
						break;
					case 'S':
						SetDebugOutputMode(2);
						break;
					case 'M':
						SetDebugOutputMode(1);
						break;
					}
				}
				break;
			}
		}
		token = strtok(NULL, " ");
	}
}

// FUNCTION: MW2SHELL 0x1001023c
void EnableShellMenu(HMENU p_menu)
{
	MechS32 result;

	result = EnableMenuItem(p_menu, 1, MF_BYPOSITION | MF_ENABLED);
	result = EnableMenuItem(p_menu, c_menuNewAllegiance, MF_ENABLED);
	result = EnableMenuItem(p_menu, c_menuHallOfHonor, MF_ENABLED);
	result = EnableMenuItem(p_menu, c_menuQuickTips, MF_ENABLED);
	CheckMenuItem(p_menu, c_menuQuickTips, g_quickTips ? MF_CHECKED : MF_UNCHECKED);
	result = EnableMenuItem(p_menu, c_menuCombatVariables, MF_ENABLED);
	result = EnableMenuItem(p_menu, c_menuCockpitControls, MF_ENABLED);
	result = EnableMenuItem(p_menu, c_menuMoviePlayback, MF_ENABLED);
	result = EnableMenuItem(p_menu, c_menuKeshik, MF_ENABLED);
	result = DrawMenuBar(g_gameWindow);
}

// FUNCTION: MW2SHELL 0x10010320
void DisableShellMenu(HMENU p_menu)
{
	MechS32 result;

	result = EnableMenuItem(p_menu, 1, MF_BYPOSITION | MF_GRAYED);
	result = EnableMenuItem(p_menu, c_menuNewAllegiance, MF_GRAYED);
	result = EnableMenuItem(p_menu, c_menuHallOfHonor, MF_GRAYED);
	result = EnableMenuItem(p_menu, c_menuQuickTips, MF_GRAYED);
	result = EnableMenuItem(p_menu, c_menuCombatVariables, MF_GRAYED);
	result = EnableMenuItem(p_menu, c_menuCockpitControls, MF_GRAYED);
	result = EnableMenuItem(p_menu, c_menuMoviePlayback, MF_GRAYED);
	result = EnableMenuItem(p_menu, c_menuKeshik, MF_GRAYED);
	result = DrawMenuBar(g_gameWindow);
}

// FUNCTION: MW2SHELL 0x1001067f
BOOL CALLBACK OkDialogProc(HWND p_hDlg, UINT p_msg, WPARAM p_wParam, LPARAM)
{
	MechS32 id;

	switch (p_msg) {
	case WM_INITDIALOG:
		SetFocus(GetDlgItem(p_hDlg, IDOK));
		return FALSE;
	case WM_COMMAND:
		id = LOWORD(p_wParam);
		switch (id) {
		case IDOK:
			EndDialog(p_hDlg, 0);
			break;
		}

		return TRUE;
	}

	return FALSE;
}

// The Movie Playback dialog's (138) choices: 320 x 200 or stretched to 640 x 480
enum {
	c_idLittleMovies = 1000,
	c_idBigMovies = 1001
};

// FUNCTION: MW2SHELL 0x10010724
BOOL CALLBACK LittleMoviesDialogProc(HWND p_hWnd, UINT p_msg, WPARAM p_wParam, LPARAM)
{
	UINT command;

	switch (p_msg) {
	case WM_INITDIALOG:
		if (g_littleMovies != 0) {
			CheckDlgButton(p_hWnd, c_idLittleMovies, BST_CHECKED);
		}
		else {
			CheckDlgButton(p_hWnd, c_idBigMovies, BST_CHECKED);
		}
		SetFocus(GetDlgItem(p_hWnd, IDOK));
		return FALSE;
	case WM_COMMAND:
		command = LOWORD(p_wParam);
		switch (command) {
		case c_idLittleMovies:
			if (IsDlgButtonChecked(p_hWnd, c_idLittleMovies) == BST_CHECKED) {
				CheckDlgButton(p_hWnd, c_idBigMovies, BST_UNCHECKED);
			}
			else {
				CheckDlgButton(p_hWnd, c_idBigMovies, BST_CHECKED);
			}
			break;
		case c_idBigMovies:
			if (IsDlgButtonChecked(p_hWnd, c_idBigMovies) == BST_CHECKED) {
				CheckDlgButton(p_hWnd, c_idLittleMovies, BST_UNCHECKED);
			}
			else {
				CheckDlgButton(p_hWnd, c_idLittleMovies, BST_CHECKED);
			}
			break;
		case IDOK:
			if (IsDlgButtonChecked(p_hWnd, c_idLittleMovies) == BST_CHECKED) {
				g_littleMovies = 1;
			}
			else {
				g_littleMovies = 0;
			}
			// fall through to EndDialog
		case IDCANCEL:
			EndDialog(p_hWnd, 0);
			break;
		}
		return TRUE;
	}

	return FALSE;
}

// FUNCTION: MW2SHELL 0x100108e5
void RegisterScreenFunction(void (*p_callback)(TMPackDataBase*, MechS32*, MechU8*, char**, MechS32))
{
	g_screenFunction = p_callback;
}

// FUNCTION: MW2SHELL 0x100108fd
void UnregisterScreenFunction(void (*p_callback)(TMPackDataBase*, MechS32*, MechU8*, char**, MechS32))
{
	g_screenFunction = p_callback;
	if (g_screenFunction != NULL) {
		g_screenFunction = NULL;
	}
	else {
		DebugPrint("UnregisterScreenFunction: pointer mismatch!\n");
	}
}

// FUNCTION: MW2SHELL 0x1001093e
void RunScreenFrame()
{
	if (g_menuFunction != NULL) {
		g_menuFunction(TRUE);
	}
	else {
		if (g_screenFunction != NULL) {
			g_screenFunction(g_mw2Database, &g_selectedCampaign, &g_pilotChosen, &g_scenario, c_msgScreenFrame);
			UpdateVideos();
		}
	}
}

// FUNCTION: MW2SHELL 0x100109a0
void RegisterMenuFunction(void (*p_callback)(MechS32 p_active))
{
	g_menuFunction = p_callback;
}

// FUNCTION: MW2SHELL 0x100109b8
void UnregisterMenuFunction(void (*p_callback)(MechS32 p_active))
{
	g_menuFunction = p_callback;
	if (g_menuFunction != NULL) {
		g_menuFunction = NULL;
	}
	else {
		DebugPrint("UnregisterMenuFunction: pointer mismatch!\n");
	}
}

// FUNCTION: MW2SHELL 0x100109f9
void CloseMenuFunction()
{
	if (g_menuFunction != NULL) {
		g_menuFunction(FALSE);
	}
}
