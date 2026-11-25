#include "shellmain.h"

#include "app.h"
#include "archivereader.h"
#include "audiosubsystem.h"
#include "briefing.h"
#include "cadettraining.h"
#include "clanhall.h"
#include "credits.h"
#include "debrief.h"
#include "debugprint.h"
#include "font.h"
#include "hallofhonor.h"
#include "keyboard.h"
#include "keyboardinput.h"
#include "log.h"
#include "mainmenu.h"
#include "mechbay.h"
#include "mechvariant.h"
#include "menudata.h"
#include "messages.h"
#include "midisequence.h"
#include "missionui.h"
#include "mousestate.h"
#include "options.h"
#include "pilotroster.h"
#include "pointer.h"
#include "projectarchive.h"
#include "readyroom.h"
#include "resetglobals.h"
#include "rosterscreen.h"
#include "shellglobals.h"
#include "simhandoff.h"
#include "textglyph.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <string.h>

// The current screen's function (RegisterScreenFunction): ShellMain's loop calls it every frame
// with c_msgScreenFrame, and a menu command or leaving the shell with the message to move on to.
void (*g_screenFunction)(
	TMPackDataBase* p_database,
	int32_t* p_campaign,
	uint8_t* p_pilotChosen,
	char** p_scenario,
	int32_t p_msg
) = NULL;

// The screen a menu command opened over the current one (RegisterMenuFunction), called instead of
// g_screenFunction while set: every frame with p_active TRUE, and with FALSE to close it
// (CloseMenuFunction).
void (*g_menuFunction)(int32_t p_active) = NULL;

MidiSequence* g_midiBackgroundMusic = NULL;

int32_t g_cursorHidden = 0;

// PlayMidiSong's state: whether the base song id is set, the base, and the song playing.
uint8_t g_midiSongBaseSet = 0;

int32_t g_midiSongBase = 0;

int32_t g_midiSongPlaying = 0;

char* g_scenario;

int32_t g_selectedCampaign;

uint8_t g_pilotChosen;

void PlayMidiSong(uint32_t p_msg, int32_t p_campaign);
void RunScreenFrame();
void CloseMenuFunction();

// The shell's message handler, originally its window procedure (ShellWindowProc).
// ShellHandleMessage on the Rust side (src/shell/screens/debug.rs) wraps it, for the debug menu's
// jumps between screens and to hold back the landing and ending videos while a prompt is up.
MECH_INTPTR ShellHandleMessageC(uint32_t p_msg, size_t p_wParam, MECH_INTPTR p_lParam)
{
	uint32_t msg;

	if (p_msg >= c_mechMsgKeyFirst && p_msg <= c_mechMsgKeyLast) {
		HandleKeyboardMessages(p_msg, p_wParam, p_lParam);
		return 0;
	}

	switch (p_msg) {
	// On WM_PALETTECHANGED and WM_QUERYNEWPALETTE the original reloaded the palette and redrew
	// the whole screen.
	case c_mechMsgPaint:
		// The original redrew only the rectangle Windows asked for.
		if (g_videoDriver) {
			if (g_drawFmv) {
				g_videoDriver->ExpandRectBySize(0, 0, 320, 200);
				g_videoDriver->DrawFmv();
			}
			else {
				g_videoDriver->ExpandRect(0, 0, 640, 480);
				g_videoDriver->DrawShell();
			}
		}
		return 0;
	// On WM_NCMOUSEMOVE the original showed the cursor again if a movie had hidden it.
	case c_mechMsgMouseMove:
		if (IsFullscreenVideoPlaying() && !g_cursorHidden) {
			MechMouseShowCursor(FALSE);
			g_cursorHidden = TRUE;
		}

		// Full screen, the original showed the window's menu bar while the cursor was at the top of
		// the screen. The Rust side shows its own.
		return 0;
	case c_mechMsgDestroy:
		MechPostMessage(c_mechMsgQuit, 0, 0);
		return 0;
	case c_mechMsgCommand:
		if (IsFullscreenVideoPlaying()) {
			CloseVideo(0);
		}

		switch (p_wParam & 0xffff) {
		case c_menuNewAllegiance:
			CloseMenuFunction();
			if (g_screenFunction) {
				g_screenFunction(g_mw2Database, &g_selectedCampaign, &g_pilotChosen, &g_scenario, c_msgMainMenu);
			}
			else {
				MechPostMessage(c_msgMainMenu, c_msgMainMenu, 0);
			}
			break;
		case c_menuHallOfHonor:
			CloseMenuFunction();
			EnableShellMenuCommand(c_menuHallOfHonor, FALSE);
			DrawHallOfHonor();
			g_menuDialogOpen = TRUE;
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
					MechPostMessage(c_msgQuit, 0, 0);
				}
				g_menuDialogOpen = TRUE;
			}
			break;
		case c_menuCombatVariables:
			CloseMenuFunction();
			EnableShellMenuCommand(c_menuCombatVariables, FALSE);
			DrawOptions();
			g_menuDialogOpen = TRUE;
			break;
		case c_menuKeshik:
			CloseMenuFunction();
			EnableShellMenuCommand(c_menuKeshik, FALSE);
			DrawCredits();
			g_menuDialogOpen = TRUE;
			break;
		case c_menuHelpContents:
		case c_menuTechnicalHelp:
			// The original opened mw2help.hlp and tech.hlp in WinHelp. They do nothing for now.
			break;
		}
		return 0;
	case c_msgMainMenu:
		g_pilotChosen = FALSE;
		DrawMainMenu(g_mw2Database, &g_selectedCampaign);
		if (g_cursorHidden) {
			MechMouseShowCursor(TRUE);
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
				MechMouseShowCursor(TRUE);
				g_cursorHidden = FALSE;
			}

			DrawClanHall(g_mw2Database, g_selectedCampaign, g_pilotChosen, p_wParam);
		}
		break;
	case c_msgPilotRoster:
		if (g_cursorHidden) {
			MechMouseShowCursor(TRUE);
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
			MechPostMessage(c_mechMsgQuit, c_msgQuitToSim, 0);
			return 0;
		}
		else {
			MechPostMessage(msg, c_msgLaunchSim, 0);
		}
		break;
	case c_msgDebrief:
		DrawMissionDebrief(g_mw2Database, g_selectedCampaign, &g_scenario);
		break;
	case c_msgLandingVideo:
		MechMouseShowCursor(FALSE);
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
		MechMouseShowCursor(FALSE);
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
		MechPostMessage(c_mechMsgQuit, c_msgQuit, 0);
		return 0;
	default:
		return 0;
	}

	if (g_midiAudio) {
		PlayMidiSong(p_msg, g_selectedCampaign);
	}

	return 0;
}

// The original was WinMain-shaped, with the module, the command line and the launcher's window.
extern "C" int ShellMain(char* p_cmdLine)
{
	void* itemData = NULL;
	int32_t itemSize;
	MechMessage msg;
	int32_t fromSim = FALSE;

	// The original was loaded fresh each time.
	ResetShellGlobals();

	g_primaryHeap = MechHeapCreate();
	if (g_primaryHeap == NULL) {
		MechLogError("Insufficient memory available.");
		return 0xff;
	}

	// Drop what the sim or an earlier shell run left queued. What the start-up below posts, like
	// ReadSimHandoff's message, is kept.
	while (MechPeekMessage(&msg, 0, 0, TRUE)) {
	}

	if (*p_cmdLine == '\0') {
		g_digitalAudio = 0;
		g_midiAudio = 0;
		g_runSim = 0;
	}
	else if (strstr(p_cmdLine, "sim") != NULL) {
		fromSim = TRUE;
	}

	InitTextColorMaps();

	g_mw2Database = new TMPackDataBase(g_databaseName);
	// The original left the saved volumes unread until the options screen was opened.
	LoadSoundConfig();
	LoadDifficultyConfig();
	g_audioSubsystem = new AudioSubsystem();
	g_windowWidth = 640;
	g_windowHeight = 480;
	g_videoDriver = new VideoDriver();

	// The original loaded the window's menu (menu 104) here, and showed it unless full screen.

	if (!fromSim) {
		PlayFullscreenVideo("mintro", c_msgMainMenu, c_msgMainMenu);
	}

	g_mw2Database->GetDBItem(0x1a, &itemData, &itemSize);
	g_textFont = new Font(itemData, g_videoDriver);
	g_mw2Database->GetDBItem(0x1b, &itemData, &itemSize);
	g_titleFont = new Font(itemData, g_videoDriver);
	g_mw2Database->GetDBItem(0x1c, &itemData, &itemSize);
	g_buttonFont = new Font(itemData, g_videoDriver);
	g_defaultFont = g_textFont;
	g_mw2Database->GetDBItem(0x20, &itemData, &itemSize);
	g_bodyFont = new Font(itemData, g_videoDriver);
	g_archiveFont = g_bodyFont;
	g_textFont = g_bodyFont;
	g_defaultFont = g_textFont;

	g_mw2Database->GetDBItem(0x19, &g_cursorShape, &itemSize);
	g_mouseState = new MouseState(g_videoDriver, g_defaultFont, g_cursorShape);
	g_keyboardInput = new KeyboardInput();
	g_projectArchive = new ProjectArchive("MW2.PRJ");

	LoadPilotRoster();
	LoadDifficultyConfig();
	ReadSimHandoff(fromSim, &g_selectedCampaign, &g_pilotChosen, &g_scenario);
	MechMouseGrab(FALSE);

	if (fromSim) {
		EnableShellMenu();
		MechMouseShowCursor(TRUE);
		g_cursorHidden = FALSE;
	}
	else {
		MechMouseShowCursor(FALSE);
		g_cursorHidden = TRUE;
	}

	// The original sent the launcher's window c_msgActivateShell, to have it pass its messages to ShellWindowProc.
	MechSetMessageHandler(ShellHandleMessage);

	for (;;) {
		MechAppPump();
		if (MechPeekMessage(&msg, 0, 0, TRUE)) {
			if (msg.m_message == c_mechMsgQuit) {
				break;
			}

			MechSendMessage(msg.m_message, msg.m_wParam, msg.m_lParam);
		}

		g_mouseState->ReadMouseState();
		RunScreenFrame();
		if (g_drawFmv) {
			g_videoDriver->DrawFmv();
		}
		else {
			g_videoDriver->DrawShell();
		}
	}

	// The original sent the launcher's window c_msgActivateLauncher, to have it handle its own
	// messages again.
	MechSetMessageHandler(NULL);
	g_videoDriver->ActivateFramebuffer();

	if (g_menuFunction) {
		g_menuFunction(FALSE);
	}

	if (g_screenFunction) {
		g_screenFunction(g_mw2Database, &g_selectedCampaign, &g_pilotChosen, &g_scenario, c_msgQuit);
	}

	CloseAllVideos();

	// The original left the song to FreeLibrary. It has to go before the audio subsystem it plays
	// on, and before the heap its data is in.
	if (g_midiBackgroundMusic) {
		delete g_midiBackgroundMusic;
		g_midiBackgroundMusic = NULL;
	}

	delete g_projectArchive;
	delete g_mw2Database;
	delete g_mouseState;
	delete g_keyboardInput;
	delete g_videoDriver;
	delete g_audioSubsystem;

	MechHeapDestroy(g_primaryHeap);
	g_primaryHeap = NULL;

	if (msg.m_wParam == c_msgQuitToSim) {
		return 3;
	}
	else {
		return 0xff;
	}
}

// Handles one pending message. Returns 0 on c_mechMsgQuit (posting it again), 1 otherwise.
int32_t PumpMessage()
{
	MechMessage msg;

	MechAppPump();

	if (MechPeekMessage(&msg, 0, 0, TRUE)) {
		if (msg.m_message == c_mechMsgQuit) {
			MechPostMessage(c_mechMsgQuit, msg.m_wParam, 0);
			return 0;
		}
		else {
			MechSendMessage(msg.m_message, msg.m_wParam, msg.m_lParam);
		}
	}

	return 1;
}

// Starts the song of the shell message p_msg for the campaign, unless it is already playing.
void PlayMidiSong(uint32_t p_msg, int32_t p_campaign)
{
	int32_t size;
	void* data;
	int32_t result;
	int32_t song;

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

// The commands EnableShellMenu and DisableShellMenu switch, and whether each is enabled. The
// original kept this in the window's menu, where the shell grayed the items; the Rust side draws
// its menu bar from it.
static const int32_t g_menuCommands[] = {
	c_menuNewAllegiance,
	c_menuHallOfHonor,
	c_menuCombatVariables,
	c_menuCockpitControls,
	c_menuKeshik,
};

uint8_t g_menuCommandEnabled[sizeof(g_menuCommands) / sizeof(g_menuCommands[0])] = {1, 1, 1, 1, 1};

// Whether a menu command can be chosen. The commands that are never grayed always can.
extern "C" int32_t IsShellMenuCommandEnabled(int32_t p_command)
{
	uint32_t i;

	for (i = 0; i < sizeof(g_menuCommands) / sizeof(g_menuCommands[0]); i++) {
		if (g_menuCommands[i] == p_command) {
			return g_menuCommandEnabled[i];
		}
	}

	return TRUE;
}

// Originally EnableMenuItem on the window's menu.
void EnableShellMenuCommand(int32_t p_command, int32_t p_enabled)
{
	uint32_t i;

	for (i = 0; i < sizeof(g_menuCommands) / sizeof(g_menuCommands[0]); i++) {
		if (g_menuCommands[i] == p_command) {
			g_menuCommandEnabled[i] = p_enabled != 0;
		}
	}
}

// The original also enabled the Options menu itself.
void EnableShellMenu()
{
	memset(g_menuCommandEnabled, 1, sizeof(g_menuCommandEnabled));
}

void DisableShellMenu()
{
	memset(g_menuCommandEnabled, 0, sizeof(g_menuCommandEnabled));
}

void RegisterScreenFunction(void (*p_callback)(TMPackDataBase*, int32_t*, uint8_t*, char**, int32_t))
{
	g_screenFunction = p_callback;
}

void UnregisterScreenFunction(void (*p_callback)(TMPackDataBase*, int32_t*, uint8_t*, char**, int32_t))
{
	g_screenFunction = p_callback;
	if (g_screenFunction != NULL) {
		g_screenFunction = NULL;
	}
	else {
		DebugPrint("UnregisterScreenFunction: pointer mismatch!\n");
	}
}

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

void RegisterMenuFunction(void (*p_callback)(int32_t p_active))
{
	g_menuFunction = p_callback;
}

void UnregisterMenuFunction(void (*p_callback)(int32_t p_active))
{
	g_menuFunction = p_callback;
	if (g_menuFunction != NULL) {
		g_menuFunction = NULL;
	}
	else {
		DebugPrint("UnregisterMenuFunction: pointer mismatch!\n");
	}
}

void CloseMenuFunction()
{
	if (g_menuFunction != NULL) {
		g_menuFunction(FALSE);
	}
}
