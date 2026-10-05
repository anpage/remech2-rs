/* The "paused" banner and the pause and resume sounds. */
#include "pausebanner.h"

#include "ai.h"
#include "audio.h"
#include "config.h"
#include "debugbreak.h"
#include "decomp.h"
#include "environment.h"
#include "eyepoint.h"
#include "faceshade.h"
#include "loadres.h"
#include "mw2prj.h"
#include "network.h"
#include "objective.h"
#include "overlay.h"
#include "polydraw.h"
#include "render.h"
#include "rendersettings.h"
#include "screenscale.h"
#include "setres.h"
#include "simmain.h"
#include "soundfx.h"
#include "targeting.h"
#include "ticks.h"
#include "timedoverlays.h"
#include "types.h"
#include "view.h"

#include <stdio.h>

// The debug keys' selections (HandleDebugKey): a number, and a mech section.

// GLOBAL: MW2 0x100a15d0
MechS32 g_debugSlot = -1;

// GLOBAL: MW2 0x100a15d4
MechS32 g_debugSection = -1;

// The banner's rectangle, in 16.16 fractions of the screen until the first draw scales it.
// GLOBAL: MW2 0x100a15e0
PANE g_pausedBannerRect = {NULL, 0, 0x3333, 0x10000, 0x6666};

// GLOBAL: MW2 0x100a15f4
MechS32 g_pausedBannerUnscaled = 1;

// FUNCTION: MW2 0x10009e50
void DrawPausedBanner(void)
{
	void* shape;

	shape = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 0x5e, g_resourceTypeTags[c_resTagShp], 0);
	if (shape) {
		if (g_pausedBannerUnscaled) {
			g_pausedBannerRect.m_window = &g_mainPixelBuffer;
			ScaleRectToScreen(&g_mainPixelBuffer, &g_pausedBannerRect, &g_pausedBannerRect);
			FitRectToShape(&g_pausedBannerRect, &g_pausedBannerRect, shape, 0);
			g_pausedBannerUnscaled = 0;
		}

		VFX_shape_draw(&g_pausedBannerRect, shape, 0, 0, 0);
	}
}

// FUNCTION: MW2 0x10009ef1
void PlayPauseSound(void)
{
	PlaySoundOnce(0xc6, 100, 0x40, RandomSampleRate());
}

// FUNCTION: MW2 0x10009f13
void PlayResumeSound(void)
{
	PlaySoundOnce(0xf1, 0x32, 0x40, RandomSampleRate());
}

// Pauses the clock and the audio, outside a network game.
// FUNCTION: MW2 0x10009f35
void PauseGame(void)
{
	if (!g_netRole) {
		PauseTimer(0x80, 1);
		PauseAudio();
	}
}

// Resumes the clock and the audio, outside a network game.
// FUNCTION: MW2 0x10009f61
void ResumeGame(void)
{
	if (!g_netRole) {
		PauseTimer(0x80, 0);
		ResumeAudio();
	}
}

// The debug keys, which RunGameKey passes on while the mission timer is stopped: views, the
// debug render flags and overlays, the difficulty switches, the objective state of the selected
// star (g_debugStar) and objective (g_debugObjective), and a number and mech section selection.
// The original compares g_debugStar with g_objectiveCount in the other operand order.
// FUNCTION: MW2 0x10009f8d
void HandleDebugKey(MechU16 p_key)
{
	MechChar text[40];

	switch (p_key) {
	case 0xa:
		SetViewMode(c_viewFreeEye);
		break;
	case 0xc:
		CycleTrackedPlayer(1, 0);
		break;
	case 0xd:
		CycleTrackedPlayer(0, 0);
		break;
	case 0xf:
		DebugBreakpoint();
		break;
	case 0x5c:
		g_debugSlot = 1;
		break;
	case 0x5d:
		g_debugSlot = 2;
		break;
	case 0x5e:
		g_debugSlot = 3;
		break;
	case 0x5f:
		g_debugSlot = 4;
		break;
	case 0x60:
		g_debugSlot = 5;
		break;
	case 0x61:
		g_debugSlot = 6;
		break;
	case 0x62:
		g_debugSlot = 7;
		break;
	case 0x63:
		g_debugSlot = 8;
		break;
	case 0x64:
		g_debugSlot = 9;
		break;
	case 0x65:
		g_debugSlot = 0;
		break;
	case 0x66:
		sprintf(text, "Head Selected");
		ShowInGameMessage(text, 1, 0x16a, 0x32);
		g_debugSection = 1;
		break;
	case 0x67:
		sprintf(text, "Right Torso Selected");
		ShowInGameMessage(text, 1, 0x16a, 0x32);
		g_debugSection = 2;
		break;
	case 0x68:
		sprintf(text, "Center Torso Selected");
		ShowInGameMessage(text, 1, 0x16a, 0x32);
		g_debugSection = 3;
		break;
	case 0x69:
		sprintf(text, "Left Torso Selected");
		ShowInGameMessage(text, 1, 0x16a, 0x32);
		g_debugSection = 4;
		break;
	case 0x6a:
		sprintf(text, "Right Arm Selected");
		ShowInGameMessage(text, 1, 0x16a, 0x32);
		g_debugSection = 5;
		break;
	case 0x6b:
		sprintf(text, "Left Arm Selected");
		ShowInGameMessage(text, 1, 0x16a, 0x32);
		g_debugSection = 6;
		break;
	case 0x6c:
		sprintf(text, "Right Leg Selected");
		ShowInGameMessage(text, 1, 0x16a, 0x32);
		g_debugSection = 7;
		break;
	case 0x6d:
		sprintf(text, "Left Leg Selected");
		ShowInGameMessage(text, 1, 0x16a, 0x32);
		g_debugSection = 8;
		break;
	case 0x6e:
	case 0x6f:
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x76:
	case 0x77:
		g_debugStar = (p_key & 0xff) - 0x6e;
		if (g_debugStar >= g_objectiveCount) {
			g_debugStar = -1;
		}
		break;
	case 0x78:
		if (g_debugStar == -1) {
			g_debugStar = -2;
		}
		else {
			g_debugStar = -1;
		}
		break;
	case 0x79:
		g_debugObjective--;
		break;
	case 0x7a:
		g_debugObjective++;
		break;
	case 0x7b:
		if (g_debugStar != -1 && g_objectiveTable[g_debugStar].m_objectives[g_debugObjective].m_state == 3) {
			g_objectiveTable[g_debugStar].m_objectives[g_debugObjective].m_state = 6;
		}
		break;
	case 0x7c:
		if (g_debugStar != -1 && g_objectiveTable[g_debugStar].m_objectives[g_debugObjective].m_state == 3) {
			g_objectiveTable[g_debugStar].m_objectives[g_debugObjective].m_state = 5;
		}
		break;
	case 0x7d:
		ToggleTextureMaps(0x100);
		break;
	case 0x7e:
		ToggleTextureMaps(0x200);
		break;
	case 0x7f:
		ToggleTextureMaps(0x400);
		break;
	case 0x80:
		ToggleTextureMaps(0x800);
		break;
	case 0x82:
		if (!g_showMemInfo) {
			g_showMemInfo = 1;
		}
		else {
			g_showMemInfo = 0;
		}

		g_showMemInfoMain = g_showMemInfo;
		break;
	case 0x84:
		switch (g_renderSettings.m_wireframeColors) {
		case 1:
			g_renderSettings.m_wireframeColors = 2;
			break;
		case 0:
			g_renderSettings.m_wireframeColors = 1;
			break;
		default:
			g_renderSettings.m_wireframeColors = 0;
			break;
		}
		break;
	case 0x85:
		if (!g_showFrameRate) {
			g_showFrameRate = 1;
		}
		else {
			g_showFrameRate = 0;
		}

		g_showFrameRateMain = g_showFrameRate;
		break;
	case 0x87:
		if (!g_showSceneInfo) {
			g_showSceneInfo = 1;
		}
		else {
			g_showSceneInfo = 0;
		}

		g_showSceneInfoMain = g_showSceneInfo;
		break;
	case 0x88:
		if (!g_showEyePosition) {
			g_showEyePosition = 1;
		}
		else {
			g_showEyePosition = 0;
		}

		g_showEyePositionMain = g_showEyePosition;
		break;
	case 0x8b:
		if (!g_difficulty->m_splashDamage) {
			g_difficulty->m_splashDamage = 1;
			sprintf(text, "Splash Damage ON");
			ShowInGameMessage(text, 1, 0x16a, 0x32);
		}
		else {
			g_difficulty->m_splashDamage = 0;
			sprintf(text, "Splash Damage OFF");
			ShowInGameMessage(text, 1, 0x16a, 0x32);
		}
		break;
	case 0x8c:
		if (!g_difficulty->m_collisionDamage) {
			g_difficulty->m_collisionDamage = 1;
			sprintf(text, "Collision Damage ON");
			ShowInGameMessage(text, 1, 0x16a, 0x32);
		}
		else {
			g_difficulty->m_collisionDamage = 0;
			sprintf(text, "Collision Damage OFF");
			ShowInGameMessage(text, 1, 0x16a, 0x32);
		}
		break;
	case 0x8d:
		if (!g_difficulty->m_heatTracking) {
			g_difficulty->m_heatTracking = 1;
			sprintf(text, "Heat Tracking ON");
			ShowInGameMessage(text, 1, 0x16a, 0x32);
		}
		else {
			g_difficulty->m_heatTracking = 0;
			sprintf(text, "Heat Tracking OFF");
			ShowInGameMessage(text, 1, 0x16a, 0x32);
		}
		break;
	case 0x8e:
		if (g_lodQuality == 2) {
			g_lodQuality = 1;
			sprintf(text, "LOD Quality HIGH");
			ShowInGameMessage(text, 1, 0x16a, 0x32);
		}
		else {
			g_lodQuality = 2;
			sprintf(text, "LOD Quality LOW");
			ShowInGameMessage(text, 1, 0x16a, 0x32);
		}
		break;
	case 0x8f:
		DumpResourceCache();
		break;
	case 0x90:
		break;
	case 0x91:
		if (!g_showFrameRate) {
			g_showFrameRate = 1;
		}
		else {
			g_showFrameRate = 0;
		}
		break;
	case 0x86:
		if (!g_cockpitPanels[c_panelRadar]->m_damage) {
			g_cockpitPanels[c_panelRadar]->m_damage = 1;
		}
		else {
			g_cockpitPanels[c_panelRadar]->m_damage = 0;
		}
		break;
	case 0x93:
		if (!g_showEyePosition) {
			g_showEyePosition = 1;
		}
		else {
			g_showEyePosition = 0;
		}
		break;
	case 0x94:
		if (!g_showSceneInfo) {
			g_showSceneInfo = 1;
		}
		else {
			g_showSceneInfo = 0;
		}
		break;
	case 0x95:
		switch (g_renderSettings.m_wireframe) {
		case 0:
			g_renderSettings.m_wireframe = 2;
			break;
		case 1:
			g_renderSettings.m_wireframe = 0;
			break;
		default:
			g_renderSettings.m_wireframe = 1;
			break;
		}
		break;
	case 0x96:
		if (!g_showPalette) {
			g_showPalette = 1;
		}
		else {
			g_showPalette = 0;
		}
		break;
	default:
		break;
	}
}
