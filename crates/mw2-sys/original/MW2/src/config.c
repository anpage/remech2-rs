#include "config.h"

#include "ammobin.h"
#include "anim2d.h"
#include "approxlen.h"
#include "bargauges.h"
#include "cockpit.h"
#include "cockpitframe.h"
#include "cockpitpanel.h"
#include "damagepanel.h"
#include "decomp.h"
#include "environment.h"
#include "eyepoint.h"
#include "fadepal.h"
#include "files.h"
#include "gamekeys.h"
#include "hud.h"
#include "loadres.h"
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
#include "screenshot.h"
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

DECOMP_SIZE_ASSERT(DifficultyCfg, 0x17)
DECOMP_SIZE_ASSERT(Reel, 0x14)
DECOMP_SIZE_ASSERT(CockpitFrame, 0x8)

// The 26 cockpit panels' rectangles, in 320x200 screen coordinates (ScaleCockpitLayout scales them to the
// screen).
// GLOBAL: MW2 0x100adf58
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
// GLOBAL: MW2 0x100ae160
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

// GLOBAL: MW2 0x100ae230
RectTransitionState g_targetTransitionState = {0, 0, 0};

// GLOBAL: MW2 0x100ae240
RectTransitionState g_mechViewTransitionState = {0, 0, 0};

// GLOBAL: MW2 0x100ae250
PANE g_targetTransitionFirst = {NULL, 0x8000, 0x8000, 0x8000, 0x8000};

// GLOBAL: MW2 0x100ae268
PANE g_targetTransitionSecond = {NULL, 0, 0, 0x10000, 0x10000};

// GLOBAL: MW2 0x100ae280
PANE g_targetTransitionRect = {NULL, 0, 0, 0, 0};

// GLOBAL: MW2 0x100ae298
RectTransitionDef g_targetTransitionDef =
	{0xb5, &g_targetTransitionFirst, &g_targetTransitionSecond, &g_targetTransitionRect};

// GLOBAL: MW2 0x100ae2a8
RectTransition g_targetTransition = {&g_targetTransitionState, &g_targetTransitionDef};

// GLOBAL: MW2 0x100ae2b0
PANE g_mechViewTransitionFirst = {NULL, 0x8000, 0x8000, 0x8000, 0x8000};

// GLOBAL: MW2 0x100ae2c8
PANE g_mechViewTransitionSecond = {NULL, 0, 0, 0x10000, 0x10000};

// GLOBAL: MW2 0x100ae2e0
PANE g_mechViewTransitionRect = {NULL, 0, 0, 0, 0};

// GLOBAL: MW2 0x100ae2f8
RectTransitionDef g_mechViewTransitionDef =
	{0xb5, &g_mechViewTransitionFirst, &g_mechViewTransitionSecond, &g_mechViewTransitionRect};

// GLOBAL: MW2 0x100ae308
RectTransition g_mechViewTransition = {&g_mechViewTransitionState, &g_mechViewTransitionDef};

// The panels' transitions.
// GLOBAL: MW2 0x100ae310
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
// GLOBAL: MW2 0x100ae37c
MechS32 g_punchInAutoHeadingRequested = 0;

// GLOBAL: MW2 0x100ae380
MechS32 g_hitFadePending = 0;

// The clock times the panels light up at on startup (CockpitPanel::m_lightUpTime): the weapon panels in turn.
// GLOBAL: MW2 0x100ae388
undefined4 g_cockpitPanelLightUpTimes[c_panelCount] = {0x16a, 0xb5,  0xb5,  0x21f, 0x23d, 0x25b, 0x279, 0x297, 0x32e,
													   0x310, 0x2f2, 0x2d4, 0x2b5, 0xb5,  0xb5,  0xb5,  0xb5,  0xb5,
													   0xb5,  0xb5,  0xb5,  0xb5,  0xb5,  0x0,   0x0,   0x0};

// The local mech's state (Mech::m_powerState) when PlayCockpitWarnings last ran.
// GLOBAL: MW2 0x100ae3f0
MechS32 g_lastWarningPowerState = 0;

// GLOBAL: MW2 0x100ae3f4
MechS32 g_lockedTonePlayed = 0;

// GLOBAL: MW2 0x100ae3f8
MechS32 g_lockingTonePlayed = 0;

// GLOBAL: MW2 0x100ae3fc
MechS32 g_hitFadeCount = 0;

// The game directory (the MECHWARRIOR environment variable).
// GLOBAL: MW2 0x100ae400
MechChar g_gameDir[256] = {0};

// The number of the next screenshot SaveScreenshot saves.
// GLOBAL: MW2 0x100ae500
MechS32 g_screenshotCount = 0;

// The path BuildGamePath returns.
// GLOBAL: MW2 0x100bef58
MechChar g_gamePath[0x50];

// The local player's heading and torso twist, in whole degrees (UpdateCockpit).

// GLOBAL: MW2 0x100c326c
MechS32 g_torsoTwistDegrees;

// GLOBAL: MW2 0x100c3270
MechS32 g_headingDegrees;

// The 26 cockpit panels InitCockpitPanels allocates.
// GLOBAL: MW2 0x100c3280
CockpitPanel* g_cockpitPanels[c_panelCount];

// Which of the panels are enabled when they are set up.
// GLOBAL: MW2 0x100c32f0
MechS32 g_cockpitPanelEnabled[c_panelCount];

// GLOBAL: MW2 0x100c3358
MechS32 g_cockpitPowerState;

// The three values of the HUD layout (LoadHudFile).
// GLOBAL: MW2 0x10109c30
MechS32 g_hudLayoutValues[3];

// Loads eight sounds ahead of their use.
// FUNCTION: MW2 0x1006f480
void PreloadCockpitSounds(void)
{
	MechS32 ids[8];
	MechU32 i;

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
// The locals are a stack-slot permutation, and right > left takes its operands in the other
// order.
// FUNCTION: MW2 0x1006f4fa
void LayoutWeaponPanels(Mech* p_mech)
{
	MechS32 rightStart;
	WeaponSlot* slot;
	MechS32 panel;
	MechS32 i;
	WeaponSlot* dst;
	WeaponSlot* weapons;
	MechS32 left;
	MechS32 j;
	MechS32 index;
	MechS32 alternate;
	AmmoBin* bin;
	MechS32 right;

	alternate = TRUE;
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

				alternate = FALSE;
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

				alternate = TRUE;
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
// Stack-slot permutation: rect, transition, i and target.
// FUNCTION: MW2 0x1006fba3
void ScaleCockpitLayout(void)
{
	PANE* rect;
	RectTransition* transition;
	MechS32 i;
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
// FUNCTION: MW2 0x1006fca5
void InitCockpitPanels(void)
{
	Mech* mech;
	MechS32 i;

	for (i = 0; i < c_panelCount; i++) {
		g_cockpitPanelEnabled[i] = TRUE;
	}

	g_cockpitPanelEnabled[1] = FALSE;
	if (!g_difficulty->m_radar) {
		g_cockpitPanelEnabled[c_panelTarget] = FALSE;
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
// FUNCTION: MW2 0x1006ff7b
void ResetCockpitPanels(void)
{
	Mech* mech;
	MechS32 i;

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
// Stack-slot permutation: pitch, twistBearing, i, distance and bearing.
// FUNCTION: MW2 0x1007005a
void UpdateCockpit(Mech* p_mech)
{
	MechS32 pitch;
	MechS32 twistBearing;
	MechS32 i;
	MechS32 distance;
	MechS32 bearing;

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
// FUNCTION: MW2 0x100704c1
void ShutdownCockpitPanels(void)
{
	MechS32 i;

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
// FUNCTION: MW2 0x1007053d
void DamageCockpitPanels(Mech* p_mech, MechS32 p_heavy)
{
	MechS32 chance;
	MechS32 i;

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
// FUNCTION: MW2 0x100705dd
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

// FUNCTION: MW2 0x1007079d
void DrawPanelAnim(PANE* p_target, MechS32 p_index, MechS32 p_x, MechS32 p_y)
{
	DrawAnim2d(p_target, p_index, p_x, p_y);
}

// Loads seven values from resource p_ref.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x100707c0
MechS32 LoadMgdFile(
	ResourceRef* p_ref,
	MechS32* p_height,
	MechS32* p_cockpitHeight,
	MechS32* p_unk0x0c,
	MechS32* p_unk0x10,
	MechS32* p_unk0x14,
	MechS32* p_maxTorsoTwist,
	MechS32* p_radius
)
{
	MechS32 size;
	MechS32* data;
	MechS32* cursor;
	FILE* file;

	data = LoadResourceByRef(
		p_ref,
		g_resourceTypeTags[c_resTagMgeo],
		g_resourceTypeExtensions[c_resExtMgi],
		5,
		&size,
		NULL
	);
	if (!data) {
		file = MechFopen("symlog.txt", "a");
		if (file) {
			fprintf(file, "Couldn't load ID=%s Type=%s\n", p_ref->m_name, g_resourceTypeTags[c_resTagMgeo]);
			fclose(file);
		}

		return FALSE;
	}

	cursor = data;
	*p_height = *cursor;
	cursor++;
	*p_cockpitHeight = *cursor;
	cursor++;
	*p_unk0x0c = *cursor;
	cursor++;
	*p_unk0x10 = *cursor;
	cursor++;
	*p_unk0x14 = *cursor;
	cursor++;
	*p_maxTorsoTwist = *cursor;
	cursor++;
	*p_radius = *cursor;
	if (p_ref->m_id == -1) {
		MechHeapFree(g_primaryHeap, data);
	}
	else {
		UnlockCachedResource(p_ref->m_id, g_resourceTypeTags[c_resTagMgeo]);
	}

	return TRUE;
}

// Loads the animation file p_ref: up to 32 animations, numbered from the current base
// (GetAnimBase), into g_reels.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x100708f4
MechS32 LoadReels(ResourceRef* p_ref)
{
	MechS32 ids[32];
	MechS32 size;
	MechS32 unk0x08;
	MechU8* frames;
	MechS32 base;
	MechS32 offset;
	MechS32 count;
	MechS32 frameCount;
	MechS32 i;
	MechU8* data;
	MechS32 stride;
	MechU8* end;
	MechS32 index;
	FILE* file;

	offset = 0;
	stride = sizeof(MechS32);
	data = LoadResourceByRef(
		p_ref,
		g_resourceTypeTags[c_resTagAnim],
		g_resourceTypeExtensions[c_resExt3di],
		2,
		&size,
		&g_staticPoolTags[6]
	);
	if (!data) {
		file = MechFopen("symlog.txt", "a");
		if (file) {
			fprintf(file, "Couldn't load ID=%s Type=%s\n", p_ref->m_name, g_resourceTypeTags[c_resTagAnim]);
		}

		fclose(file);
		return FALSE;
	}

	count = *(MechS32*) data;
	frameCount = *(MechS32*) (data + 4);
	offset = stride * 2;
	base = GetAnimBase();
	for (i = 0; i < count; i++) {
		index = *(MechS32*) (data + offset);
		offset += stride;
		unk0x08 = *(MechS32*) (data + offset);
		offset += stride;
		if (index >= 0x20) {
			return FALSE;
		}

		index += base;
		if (index >= 0x780) {
			return FALSE;
		}

		frames = data + offset;
		offset += frameCount * sizeof(MechS32);
		g_reels[index] = StaticPoolAlloc(sizeof(Reel), g_staticPoolTags[5]);
		if (!g_reels[index]) {
			return FALSE;
		}

		g_reels[index]->m_amounts = (MechS32*) frames;
		g_reels[index]->m_kind = unk0x08;
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

	return TRUE;
}

// Loads the cockpit layout resource p_ref: the gauge positions (g_hudGaugePositions), three values
// (g_hudLayoutValues) and fifteen rectangles in percent of the screen (g_outlinePartRects), any of
// them out of range cleared.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10070bda
MechS32 LoadHudFile(ResourceRef* p_ref)
{
	MechS32 size;
	MechS32 value;
	MechS32 i;
	MechS32* data;
	MechS32* cursor;
	MechS32 right;
	MechS32 bottom;
	MechS32 left;
	MechS32 top;
	FILE* file;

	data = LoadResourceByRef(
		p_ref,
		g_resourceTypeTags[c_resTagHud],
		g_resourceTypeExtensions[c_resExtHdi],
		4,
		&size,
		NULL
	);
	if (!data) {
		file = MechFopen("symlog.txt", "a");
		if (file) {
			fprintf(file, "Couldn't load ID=%s Type=%s\n", p_ref->m_name, g_resourceTypeTags[c_resTagHud]);
		}

		fclose(file);
		return FALSE;
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

	return TRUE;
}

// Loads the cockpit layout resource p_ref: five rectangles into p_gauges, fifteen into p_panels
// and a point.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10070e22
MechS32 LoadCptFile(ResourceRef* p_ref, PANE* p_gauges, PANE* p_panels, Point* p_point)
{
	MechS32 size;
	PANE* target;
	MechS32 i;
	CockpitFrame* frame;
	void* data;
	MechS16* value;
	FILE* file;

	if (!p_gauges || !p_panels || !p_point) {
		return FALSE;
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
		file = MechFopen("symlog.txt", "a");
		if (file) {
			fprintf(file, "Couldn't load ID=%s Type=%s\n", p_ref->m_name, g_resourceTypeTags[c_resTagCpit]);
		}

		fclose(file);
		return FALSE;
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

	value = (MechS16*) frame;
	p_point->m_x = *value;
	value++;
	p_point->m_y = *value;
	value++;
	if (p_ref->m_id == -1) {
		MechHeapFree(g_primaryHeap, data);
	}
	else {
		UnlockCachedResource(p_ref->m_id, g_resourceTypeTags[c_resTagCpit]);
	}

	return TRUE;
}

// Writes the screen as a picture: a 0x20-byte header from TABL resource 15, the palette, then
// the main pixel buffer. Returns whether it could.
// Stack-slot permutation: header, file and pixels.
// FUNCTION: MW2 0x10071026
MechS32 WriteScreenPicture(MechChar* p_path, void* p_palette)
{
	void* header;
	MechS32 file;
	undefined* pixels;

	header = NULL;
	file = MechOpen(p_path, c_mechOpenCreate);
	if (file == -1) {
		return FALSE;
	}

	header = LoadCachedResource(g_mw2PrjHandle, 15, g_resourceTypeTags[c_resTagTable], 1);
	if (header == NULL) {
		MechClose(file);
		return FALSE;
	}

	MechWrite(file, header, 0x20);
	MechWrite(file, p_palette, 0x300);
	pixels = g_mainPixelBuffer.m_buffer;
	MechWrite(file, pixels, g_screenPixelCount);
	MechClose(file);
	UnlockCachedResource(15, g_resourceTypeTags[c_resTagTable]);
	return TRUE;
}

// Reads a whole file into memory from the heap, or from a static pool when p_poolTag is given.
// Returns the open file, or -1 (logging the path to symlog.txt when it can't be opened).
// Stack-slot permutation: log and file.
// FUNCTION: MW2 0x10071108
MechS32 LoadFile(MechChar* p_path, MechS32* p_size, void** p_data, MechU32* p_poolTag)
{
	FILE* log;
	MechS32 file;

	file = MechOpen(p_path, c_mechOpenRead);
	if (file == -1) {
		log = MechFopen("symlog.txt", "a");
		if (log) {
			fprintf(log, "Couldn't load ID=%s\n", p_path);
		}
		fclose(log);
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
// Stack-slot permutation: file and data.
// FUNCTION: MW2 0x10071251
MechS32 ReadGameFile(MechChar* p_name, void** p_data)
{
	MechS32 size;
	MechS32 file;
	void* data;

	*p_data = NULL;
	file = LoadFile(BuildGamePath(p_name), &size, &data, NULL);
	if (file != -1) {
		MechClose(file);
		*p_data = data;
	}

	return file;
}

// FUNCTION: MW2 0x100712b0
MechS32 WriteCareerRecordFile(MechChar* p_name, void* p_data)
{
	MechS32 file;
	MechS32 result;

	file = MechOpen(BuildGamePath(p_name), c_mechOpenCreate);
	if (file != -1) {
		MechWrite(file, p_data, 0xd6);
		MechClose(file);
		result = 0;
	}
	else {
		result = -1;
	}

	return result;
}

// Reads the difficulty settings into a new block, then overrides some of them in network games.
// Returns 1, or -1 if there is no block.
// Stack-slot permutation: file and data.
// FUNCTION: MW2 0x10071326
MechS32 LoadDifficultyCfg(MechChar* p_name, DifficultyCfg** p_cfg)
{
	MechS32 size;
	MechS32 file;
	void* data;

	*p_cfg = MechHeapAllocZeroed(g_primaryHeap, sizeof(DifficultyCfg));
	file = LoadFile(BuildGamePath(p_name), &size, &data, NULL);
	if (file != -1) {
		memcpy(*p_cfg, data, size);
	}
	else if (*p_cfg == NULL) {
		return -1;
	}

	MechClose(file);
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

// FUNCTION: MW2 0x10071440
MechS32 SaveDifficultyCfg(MechChar* p_name, DifficultyCfg* p_cfg)
{
	MechS32 file;
	MechS32 result;

	file = MechOpen(BuildGamePath(p_name), c_mechOpenCreate);
	if (file != -1) {
		MechWrite(file, p_cfg, sizeof(DifficultyCfg));
		MechClose(file);
		result = 0;
	}
	else {
		result = -1;
	}

	return result;
}

// Reads the sound settings, or allocates cleared ones. Returns 1, or -1 if it couldn't read them.
// Stack-slot permutation: file and data.
// FUNCTION: MW2 0x100714b3
MechS32 LoadSndCfg(MechChar* p_name, SoundConfig** p_cfg)
{
	MechS32 size;
	MechS32 file;
	void* data;

	file = LoadFile(BuildGamePath(p_name), &size, &data, NULL);
	if (file != -1) {
		*p_cfg = data;
	}
	else {
		*p_cfg = MechHeapAllocZeroed(g_primaryHeap, sizeof(SoundConfig));
		return -1;
	}

	MechClose(file);
	return 1;
}

// FUNCTION: MW2 0x1007152f
MechS32 SaveSndCfg(MechChar* p_name, SoundConfig* p_cfg)
{
	MechS32 file;
	MechS32 result;

	file = MechOpen(BuildGamePath(p_name), c_mechOpenCreate);
	if (file != -1) {
		MechWrite(file, p_cfg, sizeof(SoundConfig));
		MechClose(file);
		result = 0;
	}
	else {
		result = -1;
	}

	return result;
}

// Saves the screen as the next of mw2NNNN.gif, up to 1000 of them.
// FUNCTION: MW2 0x100715a2
void SaveScreenshot(void)
{
	MechS32 count;
	PANE target;
	MechChar name[16];

	target.m_window = &g_mainPixelBuffer;
	target.m_x0 = 0;
	target.m_y0 = 0;
	target.m_x1 = g_screenWidthMinus1;
	target.m_y1 = g_screenHeightMinus1;
	if (g_screenshotCount < 1000) {
		count = g_screenshotCount++;
		snprintf(name, sizeof(name), "mw2%04d.gif", count);
		ScreenshotBegin(name);
		ScreenshotWritePalette();
		ScreenshotWriteImage(&target);
		ScreenshotEnd();
	}
}

// Returns the path of a game file: in g_gameDir unless the name has a directory already.
// FUNCTION: MW2 0x1007162a
MechChar* BuildGamePath(MechChar* p_name)
{
	MechS32 i;

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
// FUNCTION: MW2 0x100716ec
MechChar* BuildGameDirPath(MechChar* p_name)
{
	MechS32 i;

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
