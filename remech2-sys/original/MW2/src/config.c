#include "config.h"

#include "ammobin.h"
#include "anim2d.h"
#include "approxlen.h"
#include "audio.h"
#include "bargauges.h"
#include "cockpit.h"
#include "cockpitframe.h"
#include "cockpitpanel.h"
#include "damagepanel.h"
#include "environment.h"
#include "eyepoint.h"
#include "fadepal.h"
#include "files.h"
#include "gamekeys.h"
#include "gamesettings.h"
#include "hud.h"
#include "loadres.h"
#include "log.h"
#include "mech.h"
#include "mechdamage.h"
#include "mechviewpanel.h"
#include "mw2prj.h"
#include "network.h"
#include "objectanim.h"
#include "players.h"
#include "point.h"
#include "poolsizes.h"
#include "random.h"
#include "recttransition.h"
#include "reel.h"
#include "render.h"
#include "resource.h"
#include "resourcename.h"
#include "resourceref.h"
#include "screenscale.h"
#include "simmain.h"
#include "soundconfig.h"
#include "soundfx.h"
#include "speech.h"
#include "staticmem.h"
#include "statuspanels.h"
#include "targeting.h"
#include "targetpanel.h"
#include "types.h"
#include "weapondata.h"
#include "weaponpanel.h"
#include "weapons.h"
#include "weaponslot.h"

#include <stdio.h>
#include <string.h>

// The 26 cockpit panels' rectangles, in 320x200 screen coordinates (ScaleCockpitLayout scales them to the
// screen).
PANE g_cockpitPanelPanes[c_panelCount] = {
	{&g_mainPixelBuffer, 13, 10, 80, 60},     {&g_mainPixelBuffer, 260, 145, 312, 197},
	{&g_mainPixelBuffer, 258, 145, 309, 175}, {&g_mainPixelBuffer, 214, 15, 256, 23},
	{&g_mainPixelBuffer, 214, 25, 256, 33},   {&g_mainPixelBuffer, 214, 35, 256, 43},
	{&g_mainPixelBuffer, 214, 45, 256, 53},   {&g_mainPixelBuffer, 214, 55, 256, 63},
	{&g_mainPixelBuffer, 268, 15, 319, 23},   {&g_mainPixelBuffer, 268, 25, 319, 33},
	{&g_mainPixelBuffer, 268, 35, 319, 43},   {&g_mainPixelBuffer, 268, 45, 319, 53},
	{&g_mainPixelBuffer, 268, 55, 319, 63},   {&g_mainPixelBuffer, 10, 145, 62, 179},
	{&g_mainPixelBuffer, 10, 183, 70, 199},   {&g_mainPixelBuffer, 30, 70, 300, 170},
	{&g_mainPixelBuffer, 30, 70, 300, 170},   {&g_mainPixelBuffer, 55, 190, 120, 199},
	{&g_mainPixelBuffer, 275, 130, 317, 195}, {&g_mainPixelBuffer, 250, 188, 273, 199},
	{&g_mainPixelBuffer, 110, 175, 169, 189}, {&g_mainPixelBuffer, 170, 175, 209, 189},
	{&g_mainPixelBuffer, 210, 175, 249, 189}, {&g_mainPixelBuffer, 1, 74, 35, 111},
	{&g_mainPixelBuffer, 115, 1, 205, 35},    {&g_mainPixelBuffer, 10, 136, 70, 145},
};

// The panels' text positions (16.16 fractions of their rectangles).
Point g_cockpitPanelTextOrigins[c_panelCount] = {{0, 0},           {0, 0},
												 {0, 0},           {0x11ec, 0x2148},
												 {0x11ec, 0x2148}, {0x11ec, 0x2148},
												 {0x11ec, 0x2148}, {0x11ec, 0x2148},
												 {0x11ec, 0x2148}, {0x11ec, 0x2148},
												 {0x11ec, 0x2148}, {0x11ec, 0x2148},
												 {0x11ec, 0x2148}, {0, 0},
												 {0, 0},           {0, 0},
												 {0, 0},           {0, 0},
												 {0, 0xe666},      {0, 0},
												 {0, 0xa666},      {0, 0xa666},
												 {0, 0xa666},      {0, 0},
												 {0, 0},           {0, 0}};

// The transitions of panels 13 and 2.

RectTransitionState g_targetTransitionState = {0, 0, 0};

RectTransitionState g_mechViewTransitionState = {0, 0, 0};

PANE g_targetTransitionFirst = {NULL, 0x8000, 0x8000, 0x8000, 0x8000};

PANE g_targetTransitionSecond = {NULL, 0, 0, 0x10000, 0x10000};

PANE g_targetTransitionRect = {NULL, 0, 0, 0, 0};

RectTransitionDef g_targetTransitionDef =
	{0xb5, &g_targetTransitionFirst, &g_targetTransitionSecond, &g_targetTransitionRect};

RectTransition g_targetTransition = {&g_targetTransitionState, &g_targetTransitionDef};

PANE g_mechViewTransitionFirst = {NULL, 0x8000, 0x8000, 0x8000, 0x8000};

PANE g_mechViewTransitionSecond = {NULL, 0, 0, 0x10000, 0x10000};

PANE g_mechViewTransitionRect = {NULL, 0, 0, 0, 0};

RectTransitionDef g_mechViewTransitionDef =
	{0xb5, &g_mechViewTransitionFirst, &g_mechViewTransitionSecond, &g_mechViewTransitionRect};

RectTransition g_mechViewTransition = {&g_mechViewTransitionState, &g_mechViewTransitionDef};

// The panels' transitions.
RectTransition* g_cockpitPanelTransitions[c_panelCount] = {
	NULL,
	NULL,
	&g_mechViewTransition,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&g_targetTransition,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

// Set when UpdateCockpit should run PunchInAutoHeading on its next frame.
int32_t g_punchInAutoHeadingRequested = 0;

int32_t g_hitFadePending = 0;

// The clock times the panels light up at on startup (CockpitPanel::m_lightUpTime): the weapon panels in turn.
uint32_t g_cockpitPanelLightUpTimes[c_panelCount] = {0x16a, 0xb5,  0xb5,  0x21f, 0x23d, 0x25b, 0x279, 0x297, 0x32e,
													 0x310, 0x2f2, 0x2d4, 0x2b5, 0xb5,  0xb5,  0xb5,  0xb5,  0xb5,
													 0xb5,  0xb5,  0xb5,  0xb5,  0xb5,  0x0,   0x0,   0x0};

// The local mech's state (Mech::m_powerState) when PlayCockpitWarnings last ran.
int32_t g_lastWarningPowerState = 0;

int32_t g_lockedTonePlayed = 0;

int32_t g_lockingTonePlayed = 0;

int32_t g_hitFadeCount = 0;

// The game directory (the MECHWARRIOR environment variable).
char g_gameDir[256] = {0};

// The path BuildGamePath returns.
char g_gamePath[0x50];

// The local player's heading and torso twist, in whole degrees (UpdateCockpit).

int32_t g_torsoTwistDegrees;

int32_t g_headingDegrees;

// The 26 cockpit panels InitCockpitPanels allocates.
CockpitPanel* g_cockpitPanels[c_panelCount];

// Which of the panels are enabled when they are set up.
int32_t g_cockpitPanelEnabled[c_panelCount];

int32_t g_cockpitPowerState;

// The three values of the HUD layout (LoadHudFile).
int32_t g_hudLayoutValues[3];

// Loads eight sounds ahead of their use.
void PreloadCockpitSounds(void)
{
	int32_t ids[8];
	uint32_t i;

	ids[0] = 0xbd;
	ids[1] = 0xf7;
	ids[2] = 0xdb;
	ids[3] = 0xf0;
	ids[4] = 0xf6;
	ids[5] = 0xcf;
	ids[6] = 0xce;
	ids[7] = 0xfe;
	for (i = 0; i < 8; i++) {
		PreloadResource(ids[i], g_resourceTypeTags[c_resTagSnds]);
	}
}

// Lays out p_mech's weapons on the weapon panels: the left panels (3 to 7) take the weapons on
// hardpoints 5, 3 and 7, the right ones (8 to 12) those on 4, 1 and 6, and the weapons on 2 and 0
// fill the rest, alternating. Then reorders the weapons to match, left at even indices and right
// at odd ones, renumbering their bins, and clears the slots left empty.
void LayoutWeaponPanels(Mech* p_mech)
{
	int32_t rightStart;
	WeaponSlot* slot;
	int32_t panel;
	int32_t i;
	WeaponSlot* dst;
	WeaponSlot* weapons;
	int32_t left;
	int32_t j;
	int32_t index;
	int32_t alternate;
	AmmoBin* bin;
	int32_t right;

	alternate = true;
	for (i = 0; i < 10; i++) {
		p_mech->m_weapons[i].m_index = i;
	}

	slot = p_mech->m_weapons;
	rightStart = 8;
	for (i = 0; i < 10; i++) {
		if (slot->m_hardpoint == 4 || slot->m_hardpoint == 1 || slot->m_hardpoint == 6) {
			rightStart++;
		}

		if (i != 10) {
			slot++;
		}
	}

	slot = p_mech->m_weapons;
	left = 3;
	for (i = 0; i < 10; i++) {
		if (slot->m_hardpoint == 5 || slot->m_hardpoint == 3 || slot->m_hardpoint == 7) {
			g_cockpitPanels[left]->m_setName(g_cockpitPanels[left], g_weaponDefs[slot->m_type].m_name);
			g_cockpitPanels[left]->m_setWeapon(g_cockpitPanels[left], i);
			g_cockpitPanels[left]->m_drawStartup = DrawWeaponPanelStartup;
			g_cockpitPanels[left]->m_draw = DrawWeaponPanel;
			left++;
		}

		if (left == 8) {
			left = rightStart;
		}

		if (i != 10) {
			slot++;
		}
	}

	slot = p_mech->m_weapons;
	right = 8;
	for (i = 0; i < 10; i++) {
		if (slot->m_hardpoint == 4 || slot->m_hardpoint == 1 || slot->m_hardpoint == 6) {
			g_cockpitPanels[right]->m_setName(g_cockpitPanels[right], g_weaponDefs[slot->m_type].m_name);
			g_cockpitPanels[right]->m_setWeapon(g_cockpitPanels[right], i);
			g_cockpitPanels[right]->m_drawStartup = DrawWeaponPanelStartup;
			g_cockpitPanels[right]->m_draw = DrawWeaponPanel;
			right++;
		}

		if (right > 12) {
			right = left;
		}

		if (i != 10) {
			slot++;
		}
	}

	slot = p_mech->m_weapons;
	panel = left;
	for (i = 0; i < 10; i++) {
		if (slot->m_hardpoint == 2 || slot->m_hardpoint == 0) {
			if (alternate && right > left) {
				panel = right;
				if (panel == 13) {
					panel = left;
					left++;
				}
				else {
					right++;
				}

				alternate = false;
			}
			else {
				panel = left;
				if (panel == 8) {
					panel = right;
					right++;
				}
				else {
					left++;
				}

				alternate = true;
			}

			g_cockpitPanels[panel]->m_setName(g_cockpitPanels[panel], g_weaponDefs[slot->m_type].m_name);
			g_cockpitPanels[panel]->m_setWeapon(g_cockpitPanels[panel], i);
			g_cockpitPanels[panel]->m_drawStartup = DrawWeaponPanelStartup;
			g_cockpitPanels[panel]->m_draw = DrawWeaponPanel;
			panel++;
		}

		if (i != 10) {
			slot++;
		}
	}

	weapons = MechHeapAllocZeroed(g_primaryHeap, 10 * sizeof(WeaponSlot));
	dst = weapons;
	for (i = 3; i <= 7; i++) {
		if (g_cockpitPanels[i]->m_weapon != -1) {
			slot = &p_mech->m_weapons[g_cockpitPanels[i]->m_weapon];
			*dst = *slot;
			if (i != 7) {
				dst += 2;
			}
		}
	}

	dst = weapons + 1;
	for (i = 8; i <= 12; i++) {
		if (g_cockpitPanels[i]->m_weapon != -1) {
			slot = &p_mech->m_weapons[g_cockpitPanels[i]->m_weapon];
			*dst = *slot;
			if (i != 12) {
				dst += 2;
			}
		}
	}

	index = 0;
	for (i = 3; i <= 7; i++) {
		if (g_cockpitPanels[i]->m_weapon != -1) {
			slot = &p_mech->m_weapons[g_cockpitPanels[i]->m_weapon];
			for (j = 0; j < slot->m_binCount; j++) {
				bin = &((AmmoBin*) p_mech->m_ammoBins)[slot->m_bins[j]];
				bin->m_weapon = index;
			}
		}

		g_cockpitPanels[i]->m_weapon = index;
		index += 2;
	}

	index = 1;
	for (i = 8; i <= 12; i++) {
		if (g_cockpitPanels[i]->m_weapon != -1) {
			slot = &p_mech->m_weapons[g_cockpitPanels[i]->m_weapon];
			for (j = 0; j < slot->m_binCount; j++) {
				bin = &((AmmoBin*) p_mech->m_ammoBins)[slot->m_bins[j]];
				bin->m_weapon = index;
			}

			g_cockpitPanels[i]->m_weapon = index;
			index += 2;
		}
	}

	memcpy(p_mech->m_weapons, weapons, 10 * sizeof(WeaponSlot));
	MechHeapFree(g_primaryHeap, weapons);
	for (i = 0; i < 10; i++) {
		slot = &p_mech->m_weapons[i];
		if (!slot->m_type && !slot->m_ammo) {
			slot->m_type = -1;
		}
	}
}

// Scales the panels' rectangles, text positions and transition rectangles to the screen.
void ScaleCockpitLayout(void)
{
	PANE* rect;
	RectTransition* transition;
	int32_t i;
	PANE* target;

	for (i = 0; i < c_panelCount; i++) {
		target = &g_cockpitPanelPanes[i];
		ScaleRectFromLowRes(target, target);
		ScaleRectToScreen(&g_mainPixelBuffer, target, target);
		ScalePointToFrame(target, &g_cockpitPanelTextOrigins[i], &g_cockpitPanelTextOrigins[i]);
		transition = g_cockpitPanelTransitions[i];
		if (transition) {
			rect = transition->m_def->m_first;
			rect->m_window = &g_mainPixelBuffer;
			ScaleRectToFrame(target, rect, rect);
			rect = transition->m_def->m_second;
			rect->m_window = &g_mainPixelBuffer;
			ScaleRectToFrame(target, rect, rect);
			rect = transition->m_def->m_out;
			rect->m_window = &g_mainPixelBuffer;
		}
	}
}

// Creates the 26 cockpit panels, all enabled but the second and (in network games) the
// fourteenth, lays out the local mech's weapon panels (LayoutWeaponPanels) and installs the panels'
// handlers.
void InitCockpitPanels(void)
{
	Mech* mech;
	int32_t i;

	for (i = 0; i < c_panelCount; i++) {
		g_cockpitPanelEnabled[i] = true;
	}

	g_cockpitPanelEnabled[1] = false;
	if (!g_difficulty->m_radar) {
		g_cockpitPanelEnabled[c_panelTarget] = false;
	}

	mech = g_players[g_localPlayerId]->m_mech;
	for (i = 0; i < c_panelCount; i++) {
		g_cockpitPanels[i] = MechHeapAlloc(g_primaryHeap, sizeof(CockpitPanel));
		InitCockpitPanel(g_cockpitPanels[i]);
		g_cockpitPanels[i]->m_setLightUpTime(g_cockpitPanels[i], g_cockpitPanelLightUpTimes[i]);
		g_cockpitPanels[i]->m_setTarget(g_cockpitPanels[i], &g_cockpitPanelPanes[i]);
		g_cockpitPanels[i]->m_setTextOrigin(g_cockpitPanels[i], &g_cockpitPanelTextOrigins[i]);
		g_cockpitPanels[i]->m_setTransition(g_cockpitPanels[i], g_cockpitPanelTransitions[i]);
		if (g_cockpitPanelEnabled[i]) {
			g_cockpitPanels[i]->m_enable(g_cockpitPanels[i]);
		}
		else {
			g_cockpitPanels[i]->m_disable(g_cockpitPanels[i]);
		}
	}

	LayoutWeaponPanels(mech);
	g_cockpitPanels[c_panelMechView]->m_draw = DrawMechViewPanel;
	g_cockpitPanels[c_panelMechView]->m_drawStatic = DrawMechViewStatic;
	g_cockpitPanels[c_panelMechView]->m_drawStartup = DrawMechViewStartup;
	g_cockpitPanels[c_panelMechView]->m_drawShutdown = DrawMechViewShutdown;
	g_cockpitPanels[c_panelTarget]->m_draw = DrawTargetPanel;
	g_cockpitPanels[c_panelTarget]->m_drawStatic = DrawTargetStatic;
	g_cockpitPanels[c_panelTargetText]->m_draw = DrawTargetPanelText;
	g_cockpitPanels[c_panelTarget]->m_drawStartup = DrawTargetPanelStartup;
	g_cockpitPanels[c_panelTarget]->m_drawShutdown = DrawTargetPanelShutdown;
	g_cockpitPanels[c_panelObjectives]->m_draw = DrawObjectivesPanel;
	g_cockpitPanels[c_panelNetwork]->m_draw = DrawNetworkPanel;
	g_cockpitPanels[c_panelAutopilot]->m_draw = DrawAutopilotPanel;
	g_cockpitPanels[c_panelSpeed]->m_draw = DrawSpeedPanel;
	g_cockpitPanels[c_panelMasc]->m_draw = DrawMascPanel;
	g_cockpitPanels[c_panelHeat]->m_draw = DrawHeatPanel;
	g_cockpitPanels[c_panelHeatRate]->m_draw = DrawHeatRatePanel;
	g_cockpitPanels[c_panelJets]->m_draw = DrawJetsPanel;
	g_cockpitPanels[c_panelKills]->m_draw = DrawKillsPanel;
	ScaleBarGauges();
	InitDamagePanel();
	g_eyeHeightOffset = &mech->m_cockpitHeight;
	g_eyeTwist = &mech->m_torsoTwist.m_value;
	InitHudGauges();
	if (g_difficulty->m_radar) {
		InitCockpitViews();
	}
}

// Lays out the local mech's weapon panels again and resets every panel to its settings.
void ResetCockpitPanels(void)
{
	Mech* mech;
	int32_t i;

	mech = g_players[g_localPlayerId]->m_mech;
	LayoutWeaponPanels(mech);
	for (i = 0; i < c_panelCount; i++) {
		g_cockpitPanels[i]->m_setLightUpTime(g_cockpitPanels[i], g_cockpitPanelLightUpTimes[i]);
		g_cockpitPanels[i]->m_setTransition(g_cockpitPanels[i], g_cockpitPanelTransitions[i]);
		g_cockpitPanels[i]->m_setDamage(g_cockpitPanels[i], 0);
		if (g_cockpitPanelEnabled[i]) {
			g_cockpitPanels[i]->m_enable(g_cockpitPanels[i]);
		}
	}
}

// Updates the cockpit for the local mech's frame: the heading and the torso twist the panels show,
// the target's bearing relative to both and its distance, a one-time warning sound, and the
// panels' handlers for the view mode (Mech::m_powerState: 1, 2 or the rest).
void UpdateCockpit(Mech* p_mech)
{
	int32_t pitch;
	int32_t twistBearing;
	int32_t i;
	int32_t distance;
	int32_t bearing;

	if (p_mech->m_player->m_index != g_localPlayerId) {
		return;
	}

	g_cockpitPowerState = p_mech->m_powerState;
	if (g_cockpitPowerState != 2 && g_cockpitPanels[c_panelNetwork]->m_draw) {
		g_cockpitPanels[c_panelNetwork]->m_draw(g_cockpitPanels[c_panelNetwork]);
	}

	if (g_cockpitPowerState == 4 || g_cockpitPowerState == 5) {
		return;
	}

	if (!g_showHud) {
		return;
	}

	g_headingDegrees = ((p_mech->m_player->m_heading >> 16) % 360 % 360 + 360) % 360;
	g_torsoTwistDegrees = (p_mech->m_torsoTwist.m_value >> 16) % 360 % 360;
	pitch = (p_mech->m_player->m_targetInfo.m_pitch + p_mech->m_torsoPitch.m_value) % 0x1680000;
	bearing = (p_mech->m_player->m_targetInfo.m_heading >> 16) % 360 - g_headingDegrees;
	if (bearing > 180) {
		bearing -= 360;
	}
	else if (bearing < -180) {
		bearing += 360;
	}

	twistBearing = bearing - g_torsoTwistDegrees;
	if (twistBearing > 180) {
		twistBearing -= 360;
	}
	else if (twistBearing < -180) {
		twistBearing += 360;
	}

	distance = ApproximateVectorLength(
		p_mech->m_player->m_targetInfo.m_position.m_x - p_mech->m_player->m_position.m_x,
		p_mech->m_player->m_targetInfo.m_position.m_y - p_mech->m_player->m_position.m_y,
		p_mech->m_player->m_targetInfo.m_position.m_z - p_mech->m_player->m_position.m_z
	);
	if (g_punchInAutoHeadingRequested) {
		if (g_cockpitPowerState == 2) {
			PunchInAutoHeading(p_mech);
		}

		g_punchInAutoHeadingRequested = 0;
	}

	if (g_overrideShutdown && g_cockpitPowerState != 3 && (p_mech->m_flags & 4) && !(p_mech->m_flags & 8)) {
		PlaySoundEffect(0xcd, 100, 0x40, 5, 0x32);
		PlayCockpitSound(2, -1);
		p_mech->m_flags |= 8;
		g_overrideShutdown = 0;
	}
	else {
		g_overrideShutdown = 0;
	}

	PlayCockpitWarnings(p_mech);
	switch (g_cockpitPowerState) {
	case 2:
		if (g_difficulty->m_radar) {
			RunMapView();
		}

		for (i = 0; i < c_panelCount; i++) {
			if (g_cockpitPanels[i]->m_draw) {
				g_cockpitPanels[i]->m_draw(g_cockpitPanels[i]);
			}
		}

		DrawHud(
			p_mech,
			g_headingDegrees,
			g_torsoTwistDegrees,
			bearing,
			twistBearing,
			pitch,
			distance,
			g_cockpitEyeSteady
		);
		break;
	case 1:
		if (g_difficulty->m_radar) {
			PowerUpMapView();
		}

		for (i = 0; i < c_panelCount; i++) {
			if (g_cockpitPanels[i]->m_drawStartup) {
				g_cockpitPanels[i]->m_drawStartup(g_cockpitPanels[i]);
			}
		}

		break;
	default:
		if (g_difficulty->m_radar) {
			PowerDownMapView();
		}

		for (i = 0; i < c_panelCount; i++) {
			if (g_cockpitPanels[i]->m_drawShutdown) {
				g_cockpitPanels[i]->m_drawShutdown(g_cockpitPanels[i]);
			}
		}

		break;
	}
}

// Shuts the cockpit panels down: ResetMapView with the radar on (DifficultyCfg::m_radar),
// each panel's m_shutdown hook, then every 2D animation.
void ShutdownCockpitPanels(void)
{
	int32_t i;

	if (g_difficulty->m_radar) {
		ResetMapView();
	}

	for (i = 0; i < c_panelCount; i++) {
		if (g_cockpitPanels[i]->m_shutdown) {
			g_cockpitPanels[i]->m_shutdown(g_cockpitPanels[i]);
		}
	}

	FreeAnim2ds(-1);
}

// Knocks the cockpit panels about when the local player's mech is hit: each panel has a two
// (p_heavy: five) in ten chance of stepping its damage.
void DamageCockpitPanels(Mech* p_mech, int32_t p_heavy)
{
	int32_t chance;
	int32_t i;

	if (p_mech->m_player->m_index != g_localPlayerId) {
		return;
	}

	if (p_heavy) {
		chance = 5;
	}
	else {
		chance = 2;
	}

	for (i = 0; i < c_panelCount; i++) {
		if (RandomIntBelow(10) < chance) {
			g_cockpitPanels[i]->m_setDamage(g_cockpitPanels[i], g_cockpitPanels[i]->m_damage + 1);
		}
	}
}

// Plays the cockpit's warning sounds for the local mech as its state and flags change.
void PlayCockpitWarnings(Mech* p_mech)
{
	if (g_hitFadePending) {
		g_hitFadeCount++;
		FlashZappedPaletteLevel(g_hitFadeCount * 3);
		g_hitFadePending = 0;
	}

	if (g_inCockpitView) {
		if (p_mech->m_flags & 0x80) {
			if (!g_lockedTonePlayed && p_mech->m_weapons[p_mech->m_selectedWeapon].m_state == 1 &&
				p_mech->m_powerState == 2) {
				g_lockedTonePlayed = 1;
				PlaySoundEffect(0xfe, 100, 0x5f, 5, 0x32);
			}
		}
		else {
			g_lockedTonePlayed = 0;
			if (p_mech->m_flags & 0x40) {
				if (!g_lockingTonePlayed && p_mech->m_powerState == 2) {
					g_lockingTonePlayed = 1;
					PlaySoundEffect(0xcf, 100, 0x1f, 5, 0x32);
				}
			}
			else {
				g_lockingTonePlayed = 0;
			}
		}

		if (p_mech->m_powerState != g_lastWarningPowerState) {
			switch (p_mech->m_powerState) {
			case 2:
				PlaySoundEffect(0xce, 100, 0x2f, 5, 0x32);
				break;
			case 0:
			case 4:
				PlaySoundEffect(0xf6, 100, 0x40, 5, 0x32);
				break;
			default:
				break;
			}
		}
	}

	g_lastWarningPowerState = p_mech->m_powerState;
}

void DrawPanelAnim(PANE* p_target, int32_t p_index, int32_t p_x, int32_t p_y)
{
	DrawAnim2d(p_target, p_index, p_x, p_y);
}

// Loads the mech geometry resource p_ref: seven values, of which the third to fifth are unused.
int32_t LoadMgdFile(
	ResourceRef* p_ref,
	int32_t* p_height,
	int32_t* p_cockpitHeight,
	int32_t* p_maxTorsoTwist,
	int32_t* p_radius
)
{
	int32_t size;
	int32_t* data;
	int32_t* cursor;

	data = LoadResourceByRef(
		p_ref,
		g_resourceTypeTags[c_resTagMgeo],
		g_resourceTypeExtensions[c_resExtMgi],
		5,
		&size,
		NULL
	);
	if (!data) {
		MechLogErrorf("symlog.txt: Couldn't load ID=%s Type=%s\n", p_ref->m_name, g_resourceTypeTags[c_resTagMgeo]);
		return false;
	}

	cursor = data;
	*p_height = *cursor;
	cursor++;
	*p_cockpitHeight = *cursor;
	cursor += 4;
	*p_maxTorsoTwist = *cursor;
	cursor++;
	*p_radius = *cursor;
	if (p_ref->m_id == -1) {
		MechHeapFree(g_primaryHeap, data);
	}
	else {
		UnlockCachedResource(p_ref->m_id, g_resourceTypeTags[c_resTagMgeo]);
	}

	return true;
}

// Loads the animation file p_ref: up to 32 animations, numbered from the current base
// (GetAnimBase), into g_reels.
int32_t LoadReels(ResourceRef* p_ref)
{
	int32_t ids[32];
	int32_t size;
	int32_t kind;
	uint8_t* frames;
	int32_t base;
	int32_t offset;
	int32_t count;
	int32_t frameCount;
	int32_t i;
	uint8_t* data;
	int32_t stride;
	uint8_t* end;
	int32_t index;

	offset = 0;
	stride = sizeof(int32_t);
	data = LoadResourceByRef(
		p_ref,
		g_resourceTypeTags[c_resTagAnim],
		g_resourceTypeExtensions[c_resExt3di],
		2,
		&size,
		&g_staticPoolTags[6]
	);
	if (!data) {
		MechLogErrorf("symlog.txt: Couldn't load ID=%s Type=%s\n", p_ref->m_name, g_resourceTypeTags[c_resTagAnim]);
		return false;
	}

	count = *(int32_t*) data;
	frameCount = *(int32_t*) (data + 4);
	offset = stride * 2;
	base = GetAnimBase();
	for (i = 0; i < count; i++) {
		index = *(int32_t*) (data + offset);
		offset += stride;
		kind = *(int32_t*) (data + offset);
		offset += stride;
		if (index >= 0x20) {
			return false;
		}

		index += base;
		if (index >= 0x780) {
			return false;
		}

		frames = data + offset;
		offset += frameCount * sizeof(int32_t);
		g_reels[index] = StaticPoolAlloc(sizeof(Reel), g_staticPoolTags[5]);
		if (!g_reels[index]) {
			return false;
		}

		g_reels[index]->m_amounts = (int32_t*) frames;
		g_reels[index]->m_kind = kind;
		g_reels[index]->m_frameCount = frameCount;
		if (p_ref->m_id == -1) {
			g_reels[index]->m_byName = 1;
		}
		else {
			g_reels[index]->m_byName = 0;
		}

		ids[i] = index;
	}

	end = data + offset;
	for (i = 0; i < count; i++) {
		g_reels[ids[i]]->m_events = (ReelEvent*) end;
	}

	return true;
}

// Loads the cockpit layout resource p_ref: the gauge positions (g_hudGaugePositions), three values
// (g_hudLayoutValues) and fifteen rectangles in percent of the screen (g_outlinePartRects), any of
// them out of range cleared.
int32_t LoadHudFile(ResourceRef* p_ref)
{
	int32_t size;
	int32_t value;
	int32_t i;
	int32_t* data;
	int32_t* cursor;
	int32_t right;
	int32_t bottom;
	int32_t left;
	int32_t top;

	data = LoadResourceByRef(
		p_ref,
		g_resourceTypeTags[c_resTagHud],
		g_resourceTypeExtensions[c_resExtHdi],
		4,
		&size,
		NULL
	);
	if (!data) {
		MechLogErrorf("symlog.txt: Couldn't load ID=%s Type=%s\n", p_ref->m_name, g_resourceTypeTags[c_resTagHud]);
		return false;
	}

	cursor = data;
	for (i = 0; i < 5; i++) {
		g_hudGaugePositions[i].m_x = *cursor;
		cursor++;
		g_hudGaugePositions[i].m_y = *cursor;
		cursor++;
	}

	for (i = 0; i < 3; i++) {
		value = *cursor;
		cursor++;
		g_hudLayoutValues[i] = value;
	}

	for (i = 0; i < 15; i++) {
		left = *cursor;
		cursor++;
		top = *cursor;
		cursor++;
		right = *cursor;
		cursor++;
		bottom = *cursor;
		cursor++;
		if (left < 0 || left > 100 || top < 0 || top > 100 || right < 0 || right > 100 || bottom < 0 || bottom > 100) {
			left = top = right = bottom = 0;
		}

		g_outlinePartRects[i].m_x0 = left;
		g_outlinePartRects[i].m_y0 = top;
		g_outlinePartRects[i].m_x1 = right;
		g_outlinePartRects[i].m_y1 = bottom;
	}

	if (p_ref->m_id == -1) {
		MechHeapFree(g_primaryHeap, data);
	}
	else {
		UnlockCachedResource(p_ref->m_id, g_resourceTypeTags[c_resTagHud]);
	}

	return true;
}

// Loads the cockpit layout resource p_ref: five rectangles into p_gauges and fifteen into p_panels
int32_t LoadCptFile(ResourceRef* p_ref, PANE* p_gauges, PANE* p_panels)
{
	int32_t size;
	PANE* target;
	int32_t i;
	CockpitFrame* frame;
	void* data;

	if (!p_gauges || !p_panels) {
		return false;
	}

	data = LoadResourceByRef(
		p_ref,
		g_resourceTypeTags[c_resTagCpit],
		g_resourceTypeExtensions[c_resExtCpi],
		3,
		&size,
		NULL
	);
	if (!data) {
		MechLogErrorf("symlog.txt: Couldn't load ID=%s Type=%s\n", p_ref->m_name, g_resourceTypeTags[c_resTagCpit]);
		return false;
	}

	target = p_gauges;
	frame = data;
	for (i = 0; i < 5; i++) {
		target->m_x0 = frame->m_x;
		target->m_y0 = frame->m_y;
		target->m_x1 = frame->m_x + frame->m_width - 1;
		target->m_y1 = frame->m_y + frame->m_height - 1;
		frame++;
		target++;
	}

	target = p_panels;
	for (i = 0; i < 15; i++) {
		target->m_x0 = frame->m_x;
		target->m_y0 = frame->m_y;
		target->m_x1 = frame->m_x + frame->m_width - 1;
		target->m_y1 = frame->m_y + frame->m_height - 1;
		frame++;
		target++;
	}

	if (p_ref->m_id == -1) {
		MechHeapFree(g_primaryHeap, data);
	}
	else {
		UnlockCachedResource(p_ref->m_id, g_resourceTypeTags[c_resTagCpit]);
	}

	return true;
}

// Writes the screen as a picture: a 0x20-byte header from TABL resource 15, the palette, then
// the main pixel buffer. Returns whether it could.
int32_t WriteScreenPicture(char* p_path, void* p_palette)
{
	void* header;
	int32_t file;
	uint8_t* pixels;

	header = NULL;
	file = MechOpen(p_path, c_mechOpenCreate);
	if (file == -1) {
		return false;
	}

	header = LoadCachedResource(g_mw2PrjHandle, 15, g_resourceTypeTags[c_resTagTable]);
	if (header == NULL) {
		MechClose(file);
		return false;
	}

	MechWrite(file, header, 0x20);
	MechWrite(file, p_palette, 0x300);
	pixels = g_mainPixelBuffer.m_buffer;
	MechWrite(file, pixels, g_screenPixelCount);
	MechClose(file);
	UnlockCachedResource(15, g_resourceTypeTags[c_resTagTable]);
	return true;
}

// Reads a whole file into memory from the heap, or from a static pool when p_poolTag is given.
// Returns the open file, or -1 (logging the path to symlog.txt when it can't be opened).
int32_t LoadFile(char* p_path, int32_t* p_size, void** p_data, uint32_t* p_poolTag)
{
	int32_t file;

	file = MechOpen(p_path, c_mechOpenRead);
	if (file == -1) {
		MechLogErrorf("symlog.txt: Couldn't load ID=%s\n", p_path);
		return -1;
	}

	*p_size = MechFileLength(file);
	if (p_poolTag == NULL) {
		*p_data = MechHeapAlloc(g_primaryHeap, *p_size);
	}
	else {
		*p_data = StaticPoolAlloc(*p_size, *p_poolTag);
	}

	if (*p_data == NULL) {
		MechClose(file);
		return -1;
	}

	if (MechRead(file, *p_data, *p_size) != *p_size) {
		if (p_poolTag == NULL) {
			MechHeapFree(g_primaryHeap, *p_data);
		}
		MechClose(file);
		return -1;
	}

	return file;
}

// Reads a game file into memory. Returns the file (closed), or -1.
int32_t ReadGameFile(char* p_name, void** p_data)
{
	int32_t size;
	int32_t file;
	void* data;

	*p_data = NULL;
	file = LoadFile(BuildGamePath(p_name), &size, &data, NULL);
	if (file != -1) {
		MechClose(file);
		*p_data = data;
	}

	return file;
}

// Builds the difficulty settings from the player’s (p_name NULL) or from file p_name, then overrides some of them in
// network games. Returns 1 or -1 if there is no block.
int32_t LoadDifficultyCfg(char* p_name, DifficultyCfg** p_cfg)
{
	int32_t size;
	int32_t file;
	void* data;

	*p_cfg = MechHeapAllocZeroed(g_primaryHeap, sizeof(DifficultyCfg));
	if (*p_cfg == NULL) {
		return -1;
	}

	DifficultySettings settings;
	if (p_name == NULL) {
		MechGetDifficultySettings(&settings);
		(*p_cfg)->m_unlimitedAmmo = settings.m_unlimitedAmmo;
		(*p_cfg)->m_invulnerable = settings.m_invulnerable;
		(*p_cfg)->m_splashDamage = settings.m_splashDamage;
		(*p_cfg)->m_collisionDamage = settings.m_collisionDamage;
		(*p_cfg)->m_heatTracking = settings.m_heatTracking;
		(*p_cfg)->m_enemySkill = settings.m_enemySkill;
	}
	else {
		file = LoadFile(BuildGamePath(p_name), &size, &data, NULL);
		if (file != -1) {
			memcpy(*p_cfg, data, size);
			MechClose(file);
		}
	}

	if (g_isNetworkGame) {
		(*p_cfg)->m_enemySkill = 2;
		(*p_cfg)->m_invulnerable = 0;
	}
	else {
		(*p_cfg)->m_gravity = 0;
		(*p_cfg)->m_timeOfDay = 0;
		(*p_cfg)->m_temperature = 0;
		(*p_cfg)->m_radar = 1;
	}

	if (g_isNetworkGame > 1) {
		(*p_cfg)->m_heatTracking = 1;
		(*p_cfg)->m_unlimitedAmmo = 0;
		(*p_cfg)->m_splashDamage = 1;
		(*p_cfg)->m_collisionDamage = 1;
	}

	return 1;
}

// Fills p_cfg from g_soundConfig's defaults and the player's sound and display settings. The
// speech and music flags follow their volumes.
void LoadSndCfg(SoundConfig* p_cfg)
{
	SoundSettings sound;
	DisplaySettings display;

	MechGetSoundSettings(&sound);
	MechGetDisplaySettings(&display);
	*p_cfg = g_soundConfig;
	p_cfg->m_effectsVolume = sound.m_effectsVolume;
	p_cfg->m_voiceVolume = sound.m_voiceVolume;
	p_cfg->m_midiVolume = sound.m_musicVolume;
	if (p_cfg->m_voiceVolume) {
		p_cfg->m_simFlags |= 2;
	}
	else {
		p_cfg->m_simFlags &= ~2;
	}
	if (p_cfg->m_midiVolume) {
		p_cfg->m_simFlags |= 8 | 4;
	}
	else {
		p_cfg->m_simFlags &= ~(8 | 4);
	}
	p_cfg->m_objectTextmaps = display.m_objectTextmaps;
	p_cfg->m_terrainTextmaps = display.m_terrainTextmaps;
	p_cfg->m_displayDetail = display.m_highDetail;
	p_cfg->m_objectDensity = display.m_highObjectDensity;
	p_cfg->m_explosionChunks = display.m_explosionChunks;
	p_cfg->m_displayBrightness = display.m_brightness;
}

void SaveSndCfg(const SoundConfig* p_cfg)
{
	SoundSettings sound;
	DisplaySettings display;

	sound.m_effectsVolume = p_cfg->m_effectsVolume;
	sound.m_voiceVolume = p_cfg->m_voiceVolume;
	sound.m_musicVolume = p_cfg->m_midiVolume;
	display.m_objectTextmaps = p_cfg->m_objectTextmaps;
	display.m_terrainTextmaps = p_cfg->m_terrainTextmaps;
	display.m_highDetail = p_cfg->m_displayDetail;
	display.m_highObjectDensity = p_cfg->m_objectDensity;
	display.m_explosionChunks = p_cfg->m_explosionChunks;
	display.m_brightness = p_cfg->m_displayBrightness;
	MechSetSoundSettings(&sound);
	MechSetDisplaySettings(&display);
}

// Returns the path of a game file: in g_gameDir unless the name has a directory already.
char* BuildGamePath(char* p_name)
{
	int32_t i;

	for (i = 0; i < 0x50; i++) {
		g_gamePath[i] = 0;
	}

	if (g_gameDir[0] && !strchr(p_name, '\\') && !strchr(p_name, '/')) {
		snprintf(g_gamePath, sizeof(g_gamePath), "%s\\%s", g_gameDir, p_name);
	}
	else {
		strcpy(g_gamePath, p_name);
	}

	return g_gamePath;
}

// Returns the path of a file in g_gameDir.
char* BuildGameDirPath(char* p_name)
{
	int32_t i;

	for (i = 0; i < 0x50; i++) {
		g_gamePath[i] = 0;
	}

	if (g_gameDir[0]) {
		snprintf(g_gamePath, sizeof(g_gamePath), "%s\\%s", g_gameDir, p_name);
	}
	else {
		strcpy(g_gamePath, p_name);
	}

	return g_gamePath;
}
