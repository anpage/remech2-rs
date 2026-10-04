#include "mechviewpanel.h"

#include "cockpitpanel.h"
#include "config.h"
#include "damagepanel.h"
#include "decomp.h"
#include "eyepoint.h"
#include "fadepal.h"
#include "gamekeys.h"
#include "hud.h"
#include "object.h"
#include "palette.h"
#include "players.h"
#include "polydraw.h"
#include "random.h"
#include "recttransition.h"
#include "screenscale.h"
#include "shots.h"
#include "simmain.h"
#include "targeting.h"
#include "types.h"

// The handlers of the cockpit panel InitCockpitPanels sets up second (g_cockpitPanels[c_panelMechView]): it
// cycles through five views of the local mech (CycleMechViewMode), drawn into the panel's render
// target, optionally through the panel's rectangle transition.

// GLOBAL: MW2 0x100a88e8
MechS32 g_mechViewStatic = 0;

// FUNCTION: MW2 0x100509a0
void CycleMechViewMode(void)
{
	g_mechViewMode++;
	if (g_mechViewMode == 6) {
		g_mechViewMode = 1;
	}
}

// Stack-slot permutation: camera, mech and saved and view. The original's longer displacements
// make its code longer, so reccmp compares only the recompiled length of it.
// FUNCTION: MW2 0x100509c8
void DrawMechViewPanel(CockpitPanel* p_panel)
{
	MechS32* camera;
	MechS32 view[7];
	RenderSettings saved;
	Mech* mech;

	camera = NULL;
	if (!p_panel->m_enabled || !g_mechViewMode) {
		return;
	}

	p_panel->m_lastPowerState = g_cockpitPowerState;
	if (p_panel->m_damage == 1 && g_mechViewMode != 1 && g_mechViewMode != 2) {
		if (g_mechViewStatic) {
			if (RandomIntBelow(10) < 7) {
				g_mechViewStatic = 0;
			}

			DrawMechViewStatic(p_panel);
			return;
		}

		if (RandomIntBelow(10) < 3) {
			g_mechViewStatic = 1;
		}
	}
	else if (p_panel->m_damage > 2 && g_mechViewMode != 1 && g_mechViewMode != 2) {
		DrawMechViewStatic(p_panel);
		return;
	}

	mech = g_players[g_localPlayerId]->m_mech;
	switch (g_mechViewMode) {
	case 0:
		break;
	case 5:
		camera = GetTrackedShotView();
		if (!camera) {
			TrackLastShot();
			camera = GetTrackedShotView();
		}

		if (!camera) {
			VFX_pane_wipe(p_panel->m_target, 0);
		}
		else {
			SetMechViewRenderSettings(&saved);
			camera[4] = 0;
			RenderViewToPane(5, 0x20000, camera, 0);
			g_renderSettings = saved;
		}

		DrawMechViewFrame(p_panel, 6, 0xfd);
		break;
	case 4:
		SaveView(g_eyepoint, view);
		SetMechViewRenderSettings(&saved);
		view[0] = mech->m_player->m_position.m_x;
		view[1] = mech->m_player->m_position.m_y;
		view[2] = mech->m_player->m_position.m_z;
		view[4] = 0x5a0000;
		view[5] = 0;
		HideObjTree(mech->m_player->m_obj);
		RenderViewToPane(5, 0x20000, view, 0);
		ShowObjTree(mech->m_player->m_obj);
		DrawMechViewFrame(p_panel, 6, 0xf7);
		g_renderSettings = saved;
		break;
	case 3:
		SaveView(g_eyepoint, view);
		SetMechViewRenderSettings(&saved);
		if (g_frontViewForRear) {
			view[3] = mech->m_player->m_torsoTwist + mech->m_player->m_heading;
		}
		else {
			view[3] = mech->m_player->m_heading + 0xb40000;
		}

		view[4] = 0;
		HideObjTree(mech->m_player->m_obj);
		RenderViewToPane(5, 0x20000, view, 0);
		ShowObjTree(mech->m_player->m_obj);
		if (g_frontViewForRear) {
			OutlinePane(p_panel->m_target, 6);
		}
		else {
			DrawMechViewFrame(p_panel, 6, 0xfa);
		}

		g_renderSettings = saved;
		break;
	case 2:
		DrawArmorBars(mech, p_panel->m_target);
		break;
	default:
		DrawDamageOutline(mech, p_panel->m_target);
		break;
	}
}

// FUNCTION: MW2 0x10050dc3
void SetMechViewRenderSettings(RenderSettings* p_saved)
{
	*p_saved = g_renderSettings;
	g_renderSettings.m_drawPixels = 0;
	g_renderSettings.m_unk0x08 = 0;
	g_renderSettings.m_textures = 1;
	g_renderSettings.m_gouraud = 1;
	g_renderSettings.m_untexturedKinds = 0xb00;
	g_renderSettings.m_affineTextures = 1;
	g_renderSettings.m_flags &= ~4;
}

// FUNCTION: MW2 0x10050e20
void DrawMechViewStatic(CockpitPanel* p_panel)
{
	if (!p_panel->m_enabled || !g_mechViewMode) {
		return;
	}

	p_panel->m_lastPowerState = g_cockpitPowerState;
	DrawPanelAnim(p_panel->m_target, 0, 0, 0);
}

// Stack-slot permutation: target and y.
// FUNCTION: MW2 0x10050e6c
void DrawMechViewFrame(CockpitPanel* p_panel, MechS32 p_color, MechS32 p_shapeId)
{
	PANE* target;
	MechS32 x;
	MechS32 y;

	target = p_panel->m_target;
	OutlinePane(target, p_color);
	x = p_panel->m_width >> 1;
	y = 2;
	DrawPaneShape(x, y, p_shapeId, target);
}

// Stack-slot permutation: frame, savedSlot and savedTarget and transition.
// FUNCTION: MW2 0x10050ebe
void DrawMechViewStartup(CockpitPanel* p_panel)
{
	PANE savedTarget;
	PANE savedSlot;
	RectTransition* transition;
	PANE* frame;

	if (!p_panel->m_enabled || !g_mechViewMode || g_mechViewMode == 2 || g_mechViewMode == 1) {
		return;
	}

	transition = p_panel->m_transition;
	if (transition) {
		if (p_panel->m_lastPowerState != 1) {
			StartRectTransition(transition);
		}

		frame = UpdateRectTransitionByAxis(0, transition);
		if (frame) {
			savedSlot = g_panes[5];
			savedTarget = *p_panel->m_target;
			g_panes[5] = *frame;
			*p_panel->m_target = *frame;
			DrawMechViewPanel(p_panel);
			g_panes[5] = savedSlot;
			*p_panel->m_target = savedTarget;
		}
		else {
			DrawMechViewPanel(p_panel);
		}
	}

	p_panel->m_lastPowerState = g_cockpitPowerState;
}

// Stack-slot permutation: frame, savedSlot and savedTarget and transition.
// FUNCTION: MW2 0x10050fd6
void DrawMechViewShutdown(CockpitPanel* p_panel)
{
	PANE savedTarget;
	PANE savedSlot;
	RectTransition* transition;
	PANE* frame;

	if (!p_panel->m_enabled || !g_mechViewMode || g_mechViewMode == 2 || g_mechViewMode == 1) {
		return;
	}

	transition = p_panel->m_transition;
	if (transition) {
		if (p_panel->m_lastPowerState != 0 && p_panel->m_lastPowerState != 3 && p_panel->m_lastPowerState != 4) {
			StartRectTransition(transition);
		}

		frame = UpdateRectTransitionByAxis(1, transition);
		if (frame) {
			savedSlot = g_panes[5];
			savedTarget = *p_panel->m_target;
			g_panes[5] = *frame;
			*p_panel->m_target = *frame;
			DrawMechViewPanel(p_panel);
			g_panes[5] = savedSlot;
			*p_panel->m_target = savedTarget;
		}
	}

	p_panel->m_lastPowerState = g_cockpitPowerState;
}
