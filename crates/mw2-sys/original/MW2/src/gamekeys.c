/* The keys typed in a mission: the cheat codes (the last 15 keys typed, and a match against a code
   stored XORed with 0x1a), the chat message being typed and the game keys. */
#include "gamekeys.h"

#include "ai.h"
#include "clock.h"
#include "cockpit.h"
#include "config.h"
#include "damagepanel.h"
#include "decomp.h"
#include "displaybackend.h"
#include "dorcs.h"
#include "environment.h"
#include "eyepoint.h"
#include "hud.h"
#include "inputmap.h"
#include "mechclass.h"
#include "mechdamage.h"
#include "mechreload.h"
#include "mechviewpanel.h"
#include "menu.h"
#include "network.h"
#include "objective.h"
#include "pausebanner.h"
#include "players.h"
#include "polydraw.h"
#include "refreshmode.h"
#include "render.h"
#include "rendersettings.h"
#include "shots.h"
#include "simmain.h"
#include "soundfx.h"
#include "speech.h"
#include "statuspanels.h"
#include "targeting.h"
#include "targetpanel.h"
#include "ticks.h"
#include "timedoverlays.h"
#include "types.h"
#include "weapons.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// GLOBAL: MW2 0x100aa290
MechS32 g_missionEndTime = 0;

// GLOBAL: MW2 0x100aa294
MechS32 g_speechFlushTime = 0;

// A game-key toggle (GetSystemSetting's setting 0x40).
// GLOBAL: MW2 0x100aa298
MechS32 g_overrideShutdown = 0;

// Set by game key 0x11.
// GLOBAL: MW2 0x100aa2a0
MechS32 g_feetToTorso = 0;

// GLOBAL: MW2 0x100aa2a4
MechS32 g_mechViewMode = 1;

// GLOBAL: MW2 0x100aa2a8
MechS32 g_missionEnded = 0;

// GLOBAL: MW2 0x100aa2ac
MechS32 g_missionTimerStopped = 0;

// The "meepmeep" cheat: enables the time compression key.
// GLOBAL: MW2 0x100aa2b0
MechS32 g_timeCompressionCheat = 0;

// The length of the chat message being typed (HandleChatKey).
// GLOBAL: MW2 0x100aa2b8
MechS32 g_chatLength = 0;

// GLOBAL: MW2 0x100aa2bc
MechS32 g_missionResolved = 0;

// GLOBAL: MW2 0x100aa2c0
MechS32 g_statusMessage = 0;

// The last 15 characters typed, the newest last.
// GLOBAL: MW2 0x100e9620
MechChar g_typedKeys[0xf];

// GLOBAL: MW2 0x100ea3e4
MechS32 g_frontViewForRear;

// Appends a character key (key code type 7) to the typed keys. Returns whether it was one.
// FUNCTION: MW2 0x1005b7c0
MechS32 AppendTypedKey(MechS16 p_key)
{
	if ((p_key & 0xff00) != 0x700) {
		return FALSE;
	}

	memmove(g_typedKeys, &g_typedKeys[1], 0xe);
	g_typedKeys[0xe] = (MechChar) p_key;
	return TRUE;
}

// Returns whether the typed keys end with the code p_code, whose characters are stored XORed
// with 0x1a.
// Stack-slot permutation of n and c.
// FUNCTION: MW2 0x1005b807
MechS32 TypedCodeMatches(MechChar* p_code)
{
	MechChar* typed;
	MechU32 n;
	MechChar* c;

	n = strlen(p_code);
	typed = &g_typedKeys[0xe];
	for (c = &p_code[n - 1]; n; n--, c--, typed--) {
		if ((*c ^ 0x1a) != *typed) {
			return FALSE;
		}
	}

	return TRUE;
}

// Takes a typed key for the cheat codes: when the last keys typed spell one (TypedCodeMatches; the codes
// are stored XORed with 0x1a), toggles or runs its cheat and says so.
// Stack-slot permutation: target and victim.
// FUNCTION: MW2 0x1005b883
void HandleCheatInput(MechS16 p_key)
{
	MechS32 target;
	MechS32 victim;

	if (!AppendTypedKey(p_key)) {
		return;
	}

	if (TypedCodeMatches("xvuhx")) { // "blorb"
		if (!g_difficulty->m_invulnerable) {
			g_difficulty->m_invulnerable = 1;
			ShowInGameMessage("Invulnerability ON", 1, 0x16a, 0x32);
		}
		else {
			g_difficulty->m_invulnerable = 0;
			ShowInGameMessage("Invulnerability OFF", 1, 0x16a, 0x32);
		}
	}
	else if (TypedCodeMatches("ys{")) { // "cia"
		if (!g_difficulty->m_unlimitedAmmo) {
			g_difficulty->m_unlimitedAmmo = 1;
			ShowInGameMessage("Unlimited Ammo ON", 1, 0x16a, 0x32);
		}
		else {
			g_difficulty->m_unlimitedAmmo = 0;
			ShowInGameMessage("Unlimited Ammo OFF", 1, 0x16a, 0x32);
		}
	}
	else if (TypedCodeMatches("yuv~wsi\x7fh")) { // "coldmiser"
		if (!g_difficulty->m_heatTracking) {
			g_difficulty->m_heatTracking = 1;
			ShowInGameMessage("Heat tracking ON", 1, 0x16a, 0x32);
		}
		else {
			g_difficulty->m_heatTracking = 0;
			ShowInGameMessage("Heat tracking OFF", 1, 0x16a, 0x32);
		}
	}
	else if (TypedCodeMatches("~\x7fs")) { // "dei"
		ShowInGameMessage("F E I F", 1, 0x16a, 0x32);
	}
	else if (TypedCodeMatches("~uhyi")) { // "dorcs"
		ShowDorcs();
		ShowInGameMessage("You asked for it!", 1, 0x16a, 0x32);
	}
	else if (TypedCodeMatches("\x7ftuv{}{c")) { // "enolagay"
		target = GetLocalTargetGamePiece();
		if (target >= 0) {
			StartNuke(g_players[target]);
		}
	}
	else if (TypedCodeMatches("|vc}shv")) { // "flygirl"
		g_localSteering.m_grantJumpJets = 1;
		ShowInGameMessage("Jumpjets", 1, 0x16a, 0x32);
	}
	else if (TypedCodeMatches("|hutn")) { // "front"
		if (!g_frontViewForRear) {
			g_frontViewForRear = 1;
			ShowInGameMessage("forward view instead of rear ON", 1, 0x16a, 0x32);
		}
		else {
			g_frontViewForRear = 0;
			ShowInGameMessage("forward view instead of rear OFF", 1, 0x16a, 0x32);
		}
	}
	else if (TypedCodeMatches("|oyq")) { // "fuck"
		ShowInGameMessage("Freebirth vulgarity will not be tolerated!", 1, 0x16a, 0x32);
	}
	else if (TypedCodeMatches("}{tq\x7fw")) { // "gankem"
		victim = GetLocalTargetGamePiece();
		if (victim >= 0) {
			KillMech(g_localPlayerId, g_players[victim]->m_mech);
		}
	}
	else if (TypedCodeMatches("r{t}{huot~")) { // "hangaround"
		if (!g_missionTimerStopped) {
			g_missionTimerStopped = 1;
		}
		else {
			g_missionTimerStopped = 0;
		}

		if (g_missionTimerStopped) {
			ShowInGameMessage("That's better........", 1, 0xb5, 0x32);
		}
		else {
			ShowInGameMessage("Now how do I win!", 1, 0xb5, 0x32);
		}
	}
	else if (TypedCodeMatches("sy{tnr{yqsn")) { // "icanthackit"
		g_forceMissionSuccess = 1;
	}
	else if (TypedCodeMatches("s~q|{")) { // "idkfa"
		ShowInGameMessage("This ain't DOOM, Bub.", 1, 0x16a, 0x50);
		RunGameKey(0x3b);
	}
	else if (TypedCodeMatches("v{sh~u")) { // "lairdo"
		if (!g_lairdoCheat) {
			g_lairdoCheat = 1;
		}
		else {
			g_lairdoCheat = 0;
		}

		ShowInGameMessage("ATTENTION ENEMIES: Don't mess with the blimp.", 1, 0xb5, 0x32);
	}
	else if (TypedCodeMatches("w\x7f\x7fjw\x7f\x7fj")) { // "meepmeep"
		if (!g_timeCompressionCheat) {
			g_timeCompressionCheat = 1;
		}
		else {
			g_timeCompressionCheat = 0;
		}

		if (g_timeCompressionCheat) {
			ShowInGameMessage("Time Compression key enabled", 1, 0xb5, 0x32);
		}
		else {
			ShowInGameMessage("Time Compression key disabled", 1, 0xb5, 0x32);
		}
	}
	else if (TypedCodeMatches("wsyr\x7fvst")) { // "michelin"
		if (!g_frameOutlineParts) {
			g_frameOutlineParts = 1;
		}
		else {
			g_frameOutlineParts = 0;
		}

		if (!g_showBoundingSpheres) {
			g_showBoundingSpheres = 1;
		}
		else {
			g_showBoundingSpheres = 0;
		}

		ShowInGameMessage("bounding spheres", 1, 0x16a, 0x32);
	}
	else if (TypedCodeMatches("ws}rncwuoi\x7f")) { // "mightymouse"
		if (!g_infiniteJumpFuel) {
			g_infiniteJumpFuel = 1;
			ShowInGameMessage("Infinite Jumpjet juice ON", 1, 0x16a, 0x32);
		}
		else {
			g_infiniteJumpFuel = 0;
			ShowInGameMessage("Infinite Jumpjet juice OFF", 1, 0x16a, 0x32);
		}
	}
	else if (TypedCodeMatches("irsn")) { // "shit"
		ShowInGameMessage("Freebirth vulgarity will not be tolerated!", 1, 0x16a, 0x32);
	}
	else if (TypedCodeMatches("nstq\x7fhx\x7fvv")) { // "tinkerbell"
		SetViewMode(c_viewFreeEye);
		g_mapFollowsFreeEye = 1;
		ShowInGameMessage("Free-eye mode ON", 1, 0x16a, 0x32);
	}
	else if (TypedCodeMatches("bh{c")) { // "xray"
		g_renderSettings.m_wireframe = 2;
		g_renderSettings.m_wireframeColors = 0;
		ShowInGameMessage("X-Ray vision enabled", 1, 0x16a, 0x32);
	}
	else if (TypedCodeMatches("`w{q")) { // "zmak"
		if (!g_timeExpansionEnabled) {
			g_timeExpansionEnabled = 1;
		}
		else {
			g_timeExpansionEnabled = 0;
		}

		if (g_timeExpansionEnabled) {
			ShowInGameMessage("Time expansion enabled", 1, 0x16a, 0x32);
		}
		else {
			ShowInGameMessage("Time expansion disabled", 1, 0x16a, 0x32);
		}
	}
}

// Turns a typed character into the one its key gives with shift held (the US layout).
// FUNCTION: MW2 0x1005bf7c
void ShiftCharacter(MechChar* p_char)
{
	MechChar shifted[16] = {'<', '_', '>', '?', ')', '!', '@', '#', '$', '%', '^', '&', '*', '(', ':', ':'};

	if (*p_char == '\'') {
		*p_char = '"';
	}
	else if (*p_char >= ',' && *p_char <= ';') {
		*p_char = shifted[*p_char - ','];
	}
	else if (*p_char >= '[' && *p_char <= ']') {
		*p_char += 0x20;
	}

	if (*p_char == '`') {
		*p_char = '~';
	}
}

// Takes a key while a chat message is being typed (g_chatRecipient is the recipient): Backspace
// edits, Enter sends it (to everyone from -1), Esc cancels, F-keys e and f pick the team (-3) or
// everyone (-2), and printable characters are added up to 40. Returns whether it took the key.
// Stack-slot permutation; g_localPlayerId == g_chatRecipient compares in the other operand order.
// FUNCTION: MW2 0x1005c057
MechS32 HandleChatKey(MechU32 p_keyCode)
{
	MechChar text[80];
	MechChar c;
	MechS32 to;

	if (g_localPlayerId == g_chatRecipient) {
		to = 0;
	}
	else {
		to = g_chatRecipient;
	}

	if (p_keyCode & 0x400) {
		return FALSE;
	}

	c = (MechChar) p_keyCode;
	if (c == '\b') {
		if (g_chatLength > 0) {
			g_chatLength--;
		}

		g_chatMessage[g_chatLength] = '\0';
		return TRUE;
	}

	switch (c) {
	case 0x1b:
		g_chatRecipient = 0;
		memset(g_chatMessage, 0, 40);
		g_chatLength = 0;
		return TRUE;
	case '\r':
		if (g_chatRecipient > 0) {
			g_chatRecipient = 0;
		}
		else if (g_chatRecipient == -1) {
			to = -1;
			g_chatRecipient = 0;
		}
	}

	if (g_chatLength > 39) {
		return TRUE;
	}

	if (p_keyCode & 0x100) {
		if (g_chatRecipient == -1) {
			switch (c) {
			case 'f':
				to = -2;
				g_chatRecipient = 0;
				break;
			case 'e':
				to = -3;
				g_chatRecipient = 0;
				break;
			default:
				return FALSE;
			}
		}
		else {
			return FALSE;
		}
	}

	if (p_keyCode & 0x200) {
		c = toupper(c);
		ShiftCharacter(&c);
	}

	if (g_chatRecipient == 0) {
		SendChatMsg(to, g_chatMessage);
		sprintf(text, "%s: %s", g_players[g_localPlayerId]->m_name, g_chatMessage);
		ShowInGameMessage(text, 1, 0x2d4, 0x32);
		memset(g_chatMessage, 0, 40);
		g_chatLength = 0;
		return TRUE;
	}

	if (c < ' ' || c > '~') {
		return FALSE;
	}

	g_chatMessage[g_chatLength] = c;
	g_chatLength++;
	return TRUE;
}

// Runs the frame's game keys: after the mission ends, waits a while before offering to quit (or,
// with DifficultyCfg::m_regenerate, to regenerate); otherwise passes the key code (from the local
// steering, through the cheat codes, the chat message and LookupGameKey) to RunGameKey.
// FUNCTION: MW2 0x1005c2e1
void HandleGameKeys(MechS32 p_unk0x00, MechS32 p_unk0x04, MechU16 p_key)
{
	if (g_simPaused) {
		return;
	}

	if (g_speechFlushTime > 0 && g_currentClock > g_speechFlushTime) {
		FlushSpeechQueue(1);
		g_speechFlushTime = 0;
	}

	if (!g_missionEndTime && !g_missionEnded) {
		if (g_missionResolved || g_fledToWindows) {
			g_statusMessage = 0;
			g_missionEnded = 1;
			g_speechFlushTime = g_currentClock + 0x108;
			g_missionEndTime = g_currentClock + 0x71c;
			ShowInGameMessage("Press CTRL-Q to exit...", 1, 0x1536, 100);
		}
		else if (g_localMechLost) {
			g_chatRecipient = 0;
			CycleTrackedPlayer(0, 1);
			g_renderSettings.m_wireframe = 0;
			SetInfrared(0, 0);
			g_speechFlushTime = g_currentClock + 0x108;
			if (!g_netRole) {
				g_missionEnded = 1;
				g_missionEndTime = g_currentClock + 0x71c;
				ShowInGameMessage("Press CTRL-Q to exit...", 1, 0x1536, 100);
			}
			else {
				g_missionEndTime = g_currentClock + 0x43e;
			}
		}
	}
	else if (g_missionEndTime > 0 && g_currentClock > g_missionEndTime) {
		g_missionEnded = 1;
		if (!g_netRole || g_missionResolved || g_fledToWindows) {
			g_shouldQuit = 1;
			g_quitStage = 0x29a;
		}
		else if (!g_difficulty->m_regenerate) {
			g_missionEndTime = -1;
			ShowInGameMessage("Press SPACEBAR to advance viewpoint, or CTRL-Q to exit.", 1, 0x58610, 100);
			g_statusMessage = 1;
			g_spectating = 1;
		}
		else {
			g_missionEndTime = 0;
			g_statusMessage = 2;
		}
	}

	p_key = g_localSteering.m_keyCode;
	if (!g_isNetworkGame) {
		HandleCheatInput(p_key);
	}

	if (g_chatRecipient && HandleChatKey(p_key)) {
		return;
	}

	p_key = LookupGameKey(p_key);
	if (!p_key) {
		return;
	}

	if (g_missionEnded) {
		if (!g_netRole || g_missionResolved || g_fledToWindows) {
			if (p_key == 0x59) {
				g_shouldQuit = 1;
				g_quitStage = 0x29a;
				return;
			}
		}
		else if (g_difficulty->m_regenerate) {
			switch (p_key) {
			case 0x59:
				g_shouldQuit = 1;
				g_quitStage = 0x29a;
				return;
			case 0x33:
				RequestMenu(4);
				break;
			case 0x4f:
				g_renderSettings.m_wireframe = 0;
				SetInfrared(0, 0);
				ReloadPlayerMech(g_localPlayerId, 0, 0, 0);
				SetViewMode(c_viewCockpit);
				g_sinkPilotTiltReset = 1;
				g_sinkPilotPanReset = 1;
				g_sinkZoomFactorReset = 1;
				g_speechLocked = 0;
				g_speechFlushTime = 0;
				g_statusMessage = 0;
				g_missionEndTime = 0;
				g_missionEnded = 0;
				g_missionResolved = 0;
				g_spectating = 0;
				break;
			}
		}
		else if (g_spectating) {
			switch (p_key) {
			case 0x59:
				g_shouldQuit = 1;
				g_quitStage = 0x29a;
				return;
			case 0x33:
				RequestMenu(4);
				break;
			case 0x4f:
				CycleTrackedPlayer(1, 0);
				break;
			}
		}
	}
	else {
		RunGameKey(p_key);
	}
}

// Performs game key p_key's action: the cockpit's detail and views, the throttle steps (0x1a-0x23),
// the controls the local steering takes (a flag set for the frame), the menus, the chat
// recipients, ejecting (0x3b) and the screenshot (0x5b). Any other key goes to HandleDebugKey while
// the mission timer is stopped.
// Stack-slot permutation: mech, step and text.
// FUNCTION: MW2 0x1005c78a
void RunGameKey(MechS32 p_key)
{
	Mech* mech;
	MechS32 step;
	MechChar text[40];

	step = -1;
	mech = g_players[g_localPlayerId]->m_mech;
	switch (p_key) {
	case 0x0:
		break;
	case 0x59:
		g_shouldQuit = 1;
		g_quitStage += 2;
		break;
	case 0x2:
		CycleMechViewMode();
		if (g_players[g_localPlayerId]->m_flags & 0x2000) {
			PlaySoundEffect(0xdc, 100, 0x40, 0, 0x32);
		}
		break;
	case 0x3:
		if (g_mechViewMode == 2) {
			g_mechViewMode = 0;
		}
		else {
			g_mechViewMode = 2;
		}

		if (g_players[g_localPlayerId]->m_flags & 0x2000) {
			PlaySoundEffect(0xdc, 100, 0x40, 1, 0x32);
		}
		break;
	case 0x4:
		if (g_mechViewMode == 3) {
			g_mechViewMode = 0;
		}
		else {
			g_mechViewMode = 3;
		}

		if (g_players[g_localPlayerId]->m_flags & 0x2000) {
			PlaySoundEffect(0xdc, 100, 0x40, 2, 0x32);
		}
		break;
	case 0x5:
		if (g_mechViewMode == 4) {
			g_mechViewMode = 0;
		}
		else {
			g_mechViewMode = 4;
		}

		if (g_players[g_localPlayerId]->m_flags & 0x2000) {
			PlaySoundEffect(0xdc, 100, 0x40, 3, 0x32);
		}
		break;
	case 0x6:
		if (g_mechViewMode == 5) {
			g_mechViewMode = 0;
		}
		else {
			g_mechViewMode = 5;
		}

		if (g_players[g_localPlayerId]->m_flags & 0x2000) {
			PlaySoundEffect(0xdc, 100, 0x40, 4, 0x32);
		}
		break;
	case 0x7:
		if (g_mechViewMode == 1) {
			g_mechViewMode = 0;
		}
		else {
			g_mechViewMode = 1;
		}

		if (g_players[g_localPlayerId]->m_flags & 0x2000) {
			PlaySoundEffect(0xdc, 100, 0x40, 6, 0x32);
		}
		break;
	case 0x8:
		g_targetPanelMode++;
		if (g_targetPanelMode > 2) {
			g_targetPanelMode = 0;
		}

		if (g_players[g_localPlayerId]->m_flags & 0x2000) {
			PlaySoundEffect(0xdc, 100, 0x40, 7, 0x32);
		}
		break;
	case 0x9:
		if (!(g_players[g_localPlayerId]->m_flags & 2)) {
			if (GetViewMode() || IsSatelliteView()) {
				LeaveSatelliteView();
				SetViewMode(c_viewCockpit);
				g_sinkPilotTiltReset = 1;
				g_sinkPilotPanReset = 1;
				g_sinkZoomFactorReset = 1;
			}
			else {
				SetViewMode(c_viewTrack);
				CycleTrackedPlayer(0, 1);
			}
		}
		break;
	case 0xe:
		if (TrackLastShot()) {
			SetViewMode(c_viewOrdinance);
		}
		break;
	case 0x11:
		g_feetToTorso = 1;
		break;
	case 0x12:
		g_localSteering.m_inspectTarget = 1;
		break;
	case 0x13:
		if (!g_showHud) {
			g_showHud = 1;
		}
		else {
			g_showHud = 0;
		}

		if (g_players[g_localPlayerId]->m_flags & 0x2000) {
			PlaySoundEffect(0xdc, 100, 0x40, 8, 0x32);
		}
		break;
	case 0x14:
		g_jettisonAmmoRequested = 1;
		break;
	case 0x17:
		g_sinkZoomFactorReset = 1;
		break;
	case 0x1a:
	case 0x1b:
	case 0x1c:
	case 0x1d:
	case 0x1e:
	case 0x1f:
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
		step = p_key - 0x1a;
		break;
	case 0x24:
		g_localSteering.m_advanceGamething = 1;
		break;
	case 0x25:
		g_localSteering.m_previousGamething = 1;
		break;
	case 0x26:
		g_localSteering.m_resetGamething = 1;
		break;
	case 0x27:
		ResetTargeting();
		break;
	case 0x28:
		g_localSteering.m_advanceNav = 1;
		break;
	case 0x29:
		g_localSteering.m_previousNav = 1;
		break;
	case 0x2a:
		g_localSteering.m_resetNav = 1;
		break;
	case 0x2b:
		g_localSteering.m_advanceGamepiece = 1;
		break;
	case 0x2c:
		g_localSteering.m_previousGamepiece = 1;
		break;
	case 0x2d:
		g_localSteering.m_resetGamepiece = 1;
		break;
	case 0x2e:
		CycleCockpitView();
		break;
	case 0x2f:
		ZoomMapView(1);
		break;
	case 0x30:
		ZoomMapView(2);
		break;
	case 0x31:
		ZoomMapView(0);
		break;
	case 0x32:
		ToggleSatelliteView();
		break;
	case 0x33:
		RequestMenu(4);
		break;
	case 0x34:
		ToggleMenu(5);
		break;
	case 0x42:
		g_showObjectives = 0;
		if (g_isNetworkGame) {
			g_chatRecipient = -1;
		}
		else {
			ToggleMenu(1);
		}
		break;
	case 0x43:
		g_showObjectives = 0;
		if (g_isNetworkGame) {
			g_chatRecipient = 1;
		}
		else {
			ToggleMenu(7);
		}
		break;
	case 0x44:
		g_showObjectives = 0;
		if (g_isNetworkGame) {
			g_chatRecipient = 2;
		}
		else {
			ToggleMenu(8);
		}
		break;
	case 0x45:
		g_showObjectives = 0;
		if (g_isNetworkGame) {
			g_chatRecipient = 3;
		}
		break;
	case 0x46:
		g_showObjectives = 0;
		if (g_isNetworkGame) {
			g_chatRecipient = 4;
		}
		break;
	case 0x47:
		g_showObjectives = 0;
		if (g_isNetworkGame) {
			g_chatRecipient = 5;
		}
		break;
	case 0x48:
		g_showObjectives = 0;
		if (g_isNetworkGame) {
			g_chatRecipient = 6;
		}
		break;
	case 0x49:
		g_showObjectives = 0;
		if (g_isNetworkGame) {
			g_chatRecipient = 7;
		}
		break;
	case 0x3b:
		EjectPlayer(g_players[g_localPlayerId]->m_mech, 1);
		break;
	case 0x3c:
		if (g_players[g_localPlayerId]->m_flags & 0x2000) {
			if (g_autoEject) {
				g_autoEject = 0;
				sprintf(text, "Automatic ejection OFF");
				ShowInGameMessage(text, 1, 0x16a, 0x32);
			}
			else {
				g_autoEject = 1;
				sprintf(text, "Automatic ejection ON");
				ShowInGameMessage(text, 1, 0x16a, 0x32);
			}
		}
		break;
	case 0x3d:
		if (mech->m_powerState == 3) {
			g_powerRequest = 1;
		}
		else {
			g_powerRequest = -1;
		}
		break;
	case 0x3e:
		if (mech->m_powerState == 3) {
			g_powerRequest = 1;
		}
		else {
			if (mech->m_powerState == 7) {
				break;
			}

			g_powerRequest = -1;
		}
		break;
	case 0x3f:
		if (!g_localSteering.m_reverse) {
			g_localSteering.m_reverse = 1;
		}
		else {
			g_localSteering.m_reverse = 0;
		}
		break;
	case 0x40:
		g_overrideShutdown = 1;
		break;
	case 0xa7:
		if (g_renderSettings.m_wireframe != 1 && (g_players[g_localPlayerId]->m_flags & 0x2000)) {
			g_renderSettings.m_wireframe = 1;
			g_renderSettings.m_wireframeColors = 0;
			PlayCockpitSound(0x1b, 1);
		}
		else {
			g_renderSettings.m_wireframe = 0;
		}
		break;
	case 0xa6:
		SetInfrared(0, !g_infraredOn);
		break;
	case 0x41:
		g_chatRecipient = 0;
		if (!g_showObjectives) {
			g_showObjectives = 1;
		}
		else {
			g_showObjectives = 0;
		}
		break;
	case 0x4a:
		g_localSteering.m_autopilot = 1;
		break;
	case 0x4b:
		g_localSteering.m_toggleGroupFire = 1;
		break;
	case 0x4c:
		g_localSteering.m_advanceTarget = 1;
		break;
	case 0x4d:
		g_localSteering.m_previousTarget = 1;
		break;
	case 0x4e:
		g_localSteering.m_resetTarget = 1;
		break;
	case 0x50:
		g_localSteering.m_nearestEnemy = 1;
		break;
	case 0x53:
		g_localSteering.m_targetLastShot = 1;
		break;
	case 0x51:
		g_localSteering.m_targetFriendly = 1;
		break;
	case 0x54:
		g_localSteering.m_nextObjective = 1;
		break;
	case 0x52:
		g_localSteering.m_targetReticle = 1;
		break;
	case 0x55:
		g_punchInAutoHeadingRequested = 1;
		break;
	case 0x56:
		g_toggleMascRequested = 1;
		break;
	case 0x57:
		g_localSteering.m_selfDestruct = 1;
		break;
	case 0x1:
		g_localSteering.m_torsoTiltReset = 1;
		g_localSteering.m_torsoPanReset = 1;
		g_sinkPilotTiltReset = 1;
		g_sinkPilotPanReset = 1;
		g_sinkEyepointTiltReset = 1;
		g_sinkEyepointPanReset = 1;
		break;
	case 0x58:
		g_pauseRequested = 1;
		break;
	case 0x97:
		SetSelectedWeaponGroup(0);
		break;
	case 0x98:
		SetSelectedWeaponGroup(1);
		break;
	case 0x99:
		SetSelectedWeaponGroup(2);
		break;
	case 0x9a:
		g_localSteering.m_weaponCycleGroup = 1;
		break;
	case 0x9b:
		g_localSteering.m_weaponFireGroup = 1;
		break;
	case 0x9c:
		g_localSteering.m_weaponFireGroup1 = 1;
		break;
	case 0x9d:
		g_localSteering.m_weaponFireGroup2 = 1;
		break;
	case 0x9e:
		g_localSteering.m_weaponFireGroup3 = 1;
		break;
	case 0x92:
		if (!g_timeCompressionCheat) {
			break;
		}

		if (!g_timeCompressionEnabled) {
			g_timeCompressionEnabled = 1;
		}
		else {
			g_timeCompressionEnabled = 0;
		}

		if (g_timeCompressionEnabled) {
			sprintf(text, "Time compression enabled");
			ShowInGameMessage(text, 1, 0xb50, 0x32);
		}
		else {
			sprintf(text, "Time compression disabled");
			ShowInGameMessage(text, 1, 0x16a, 0x32);
		}
		break;
	case 0x5b:
		PauseTimer(0x80, 1);
		g_windowActive ? g_currentDisplayBackend->m_acquireFramebuffer() : -1;
		SaveScreenshot();
		PauseTimer(0x80, 0);
		sprintf(text, "GIF saved - MW2000?.GIF");
		ShowInGameMessage(text, 1, 0x16a, 0x32);
		break;
	default:
		if (g_missionTimerStopped) {
			HandleDebugKey(p_key);
		}
		break;
	}

	if (step != -1) {
		g_localSteering.m_throttle = step * 113;
		g_localSteering.m_throttleSet = 1;
	}
}
