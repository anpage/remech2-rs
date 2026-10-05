#include "targetpanel.h"

#include "classtable.h"
#include "clock.h"
#include "cockpitpanel.h"
#include "config.h"
#include "decomp.h"
#include "eyepoint.h"
#include "fadepal.h"
#include "fixedmul29.h"
#include "fixedtrig.h"
#include "hud.h"
#include "loadres.h"
#include "mech.h"
#include "mechviewpanel.h"
#include "menu.h"
#include "mw2prj.h"
#include "navpoint.h"
#include "object.h"
#include "palette.h"
#include "players.h"
#include "polydraw.h"
#include "random.h"
#include "recttransition.h"
#include "screenscale.h"
#include "setres.h"
#include "shape.h"
#include "simmain.h"
#include "soundfx.h"
#include "speech.h"
#include "targeting.h"
#include "team.h"
#include "types.h"
#include "vfxa.h"

#include <stdio.h>
#include <string.h>

// GLOBAL: MW2 0x100ba4bc
MechS32 g_targetPanelMode = 1;

// Set to announce the target's side with the next name the target panel shows.
// GLOBAL: MW2 0x100ba4c0
MechS32 g_announceTargetSide = 0;

// When the target panel goes from the target's short name to its name.
// GLOBAL: MW2 0x100ba4c4
MechS32 g_targetFullNameTime = 0;

// The target the panel showed last.
// GLOBAL: MW2 0x100ba4c8
MechS32 g_lastPanelTarget = 0;

// The target panel flickers to static while set.
// GLOBAL: MW2 0x100ba4cc
MechS32 g_targetPanelStatic = 0;

// The name the target panel shows for an unknown installation.
// GLOBAL: MW2 0x100c26a0
MechChar g_anonymousInstallationName[8];

// Writes the target panel's text: the locked target's name (its short name when it changes, its
// name after a while), coloured by its side, and its distance.
// Stack-slot permutation of the locals. g_currentClock > g_targetFullNameTime compares with its
// operands reversed (it flipped when speech.h's declarations were added ahead of it).
// FUNCTION: MW2 0x1007b930
void DrawTargetPanelText(CockpitPanel* p_panel)
{
	MechS32 index;
	MechFloat km;
	MechS32 color;
	MechS32 kind;
	MechS32 meters;
	Mech* mech;
	MechChar text[64];
	void* font;

	color = 0xe;
	if (!p_panel->m_enabled) {
		return;
	}

	mech = g_players[g_localPlayerId]->m_mech;
	kind = mech->m_player->m_targetInfo.m_target & 0xf00;
	index = mech->m_player->m_targetInfo.m_target & 0xff;
	if (!kind || (mech->m_player->m_targetInfo.m_target & 0x1000)) {
		return;
	}

	if (mech->m_player->m_targetInfo.m_target != g_lastPanelTarget) {
		g_targetFullNameTime = 0;
	}

	if (g_inspectResult == 2) {
		g_targetFullNameTime = g_currentClock + 362;
		p_panel->m_setName(p_panel, "Out of range");
		PlayCockpitSound(0x14, -1);
	}
	else if (g_inspectResult) {
		g_targetFullNameTime = g_currentClock + 362;
		switch (kind) {
		case 0x100:
			p_panel->m_setName(p_panel, g_navTable[index].m_shortName);
			PlaySoundEffect(0xdc, 100, 0x40, 5, 0x32);
			break;
		case 0x400:
			p_panel->m_setName(p_panel, g_gameThings[index].m_shortName);
			PlaySoundEffect(0xdc, 100, 0x40, 5, 0x32);
			break;
		case 0x200:
			p_panel->m_setName(p_panel, g_players[index]->m_shortName);
			PlaySoundEffect(0xdc, 100, 0x40, 5, 0x32);
			break;
		default:
			p_panel->m_setName(p_panel, "");
			break;
		}

		if (!p_panel->m_name[0]) {
			p_panel->m_setName(p_panel, "Contents unknown");
		}
	}

	if (!g_targetFullNameTime || g_currentClock > g_targetFullNameTime) {
		g_targetFullNameTime = 0;
		switch (kind) {
		case 0x100:
			if (!(g_navTable[index].m_flags & 0x20) && (g_navTable[index].m_flags & 0x100)) {
				p_panel->m_setName(p_panel, "Unknown");
			}
			else if (!strlen(g_navTable[index].m_name)) {
				p_panel->m_setName(p_panel, "Nav Point");
			}
			else {
				p_panel->m_setName(p_panel, g_navTable[index].m_name);
			}
			break;
		case 0x400:
			if (!(g_gameThings[index].m_flags & 0x20) && (g_gameThings[index].m_flags & 0x100)) {
				if (!g_anonymousInstallationName[0]) {
					strcpy(g_anonymousInstallationName, "Unknown");
				}

				p_panel->m_setName(p_panel, g_anonymousInstallationName);
			}
			else {
				if (!strlen(g_gameThings[index].m_name)) {
					p_panel->m_setName(p_panel, "Installation");
				}
				else {
					p_panel->m_setName(p_panel, g_gameThings[index].m_name);
				}

				if (g_announceTargetSide) {
					switch (GetThingSide(index)) {
					case 0:
						PlayCockpitSound(0xf, -1);
						break;
					case 2:
						PlayCockpitSound(0x10, -1);
						break;
					case 1:
						PlayCockpitSound(0xe, -1);
						break;
					}

					g_announceTargetSide = 0;
				}
			}
			break;
		case 0x200:
			if (!(g_players[index]->m_flags & 0x20) && (g_players[index]->m_flags & 0x100)) {
				p_panel->m_setName(p_panel, "Unknown");
			}
			else {
				if (!strlen(g_players[index]->m_name)) {
					p_panel->m_setName(p_panel, "Mech");
				}
				else {
					p_panel->m_setName(p_panel, g_players[index]->m_name);
				}

				if (g_announceTargetSide) {
					switch (GetPlayerSide(index)) {
					case 0:
						PlayCockpitSound(0xf, -1);
						break;
					case 2:
						PlayCockpitSound(0x10, -1);
						break;
					case 1:
						PlayCockpitSound(0xe, -1);
						break;
					}

					g_announceTargetSide = 0;
				}
			}
			break;
		default:
			p_panel->m_setName(p_panel, "");
			break;
		}
	}

	if (kind == 0x400) {
		switch (GetThingSide(index)) {
		case 0:
			color = 0xe;
			break;
		case 2:
			color = 6;
			break;
		case 1:
			color = 0xa;
			break;
		}
	}
	else if (kind == 0x200) {
		switch (GetPlayerSide(index)) {
		case 0:
			color = 0xe;
			break;
		case 2:
			color = 6;
			break;
		case 1:
			color = 0xa;
			break;
		}
	}
	else if (kind == 0x100) {
		if (!(g_navTable[index].m_flags & 0x20)) {
			color = 2;
		}
		else {
			color = 1;
		}
	}

	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (!font) {
		return;
	}

	g_textColors[0xe] = color;
	VFX_string_draw(p_panel->m_target, 0, 0, font, p_panel->m_name, g_textColors);
	g_textColors[0xe] = 0xe;
	meters = mech->m_player->m_targetInfo.m_range / 100;
	if (meters > 1000) {
		km = meters / 1000.0;
		sprintf(text, "\n%2.2fk", km);
		DrawWrappedText(p_panel->m_target, text, font);
	}
	else {
		sprintf(text, "\n%3dm", meters);
		DrawWrappedText(p_panel->m_target, text, font);
	}

	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
	g_lastPanelTarget = mech->m_player->m_targetInfo.m_target;
}

// Draws the target panel: the locked target through a camera behind it, a nav point's icon, or
// static while the panel is damaged (m_damage).
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1007c126
void DrawTargetPanel(CockpitPanel* p_panel)
{
	MechS32 index;
	MechS32 dz;
	RenderSettings saved;
	Player* targetPlayer;
	MechS32 view[7];
	SceneObject* object;
	MechS32 kind;
	Mech* mech;
	MechS32 centerX;
	Player* player;
	MechS32 centerY;
	MechS32 distance;
	MechS32 heading;
	MechS32 x;
	MechS32 y;
	MechS32 z;
	MechS32 dx;
	MechS32 icon;
	void* noTarget;
	void* noObject;
	MechS32 targetIndex;

	if (!p_panel->m_enabled || !g_targetPanelMode) {
		return;
	}

	p_panel->m_lastPowerState = g_cockpitPowerState;
	if (p_panel->m_damage == 1) {
		if (g_targetPanelStatic) {
			if (RandomIntBelow(10) < 7) {
				g_targetPanelStatic = 0;
			}

			DrawTargetStatic(p_panel);
			return;
		}
		else {
			if (RandomIntBelow(10) < 3) {
				g_targetPanelStatic = 1;
			}
		}
	}
	else if (p_panel->m_damage > 2) {
		DrawTargetStatic(p_panel);
		return;
	}

	mech = g_players[g_localPlayerId]->m_mech;
	player = mech->m_player;
	kind = player->m_targetInfo.m_target & 0xf00;
	index = mech->m_player->m_targetInfo.m_target & 0xff;
	if (!kind || (player->m_targetInfo.m_target & 0x1000)) {
		VFX_pane_wipe(p_panel->m_target, 0);
		OutlinePane(p_panel->m_target, 8);
		return;
	}

	if (kind == 0x100) {
		centerX = (p_panel->m_target->m_x1 - p_panel->m_target->m_x0) / 2;
		centerY = (p_panel->m_target->m_y1 - p_panel->m_target->m_y0) / 2;
		if (!(g_navTable[index].m_flags & 0x20)) {
			icon = 0x100;
		}
		else {
			icon = 0x103;
		}

		if (icon) {
			VFX_pane_wipe(p_panel->m_target, 0);
			DrawPaneShape(centerX, centerY, icon, p_panel->m_target);
			OutlinePane(p_panel->m_target, 8);
		}

		return;
	}

	if (kind == 0x200) {
		targetPlayer = g_players[mech->m_player->m_targetInfo.m_target & 0xff];
		LoadClassLevel(targetPlayer->m_index, 0);
	}

	SaveView(g_eyepoint, view);
	SetMechViewRenderSettings(&saved);
	x = player->m_targetInfo.m_position.m_x;
	y = player->m_targetInfo.m_position.m_y;
	z = player->m_targetInfo.m_position.m_z;
	heading = player->m_targetInfo.m_heading;
	object = GetLocalTargetObject();
	if (!object) {
		noTarget = LoadCachedResource(g_mw2PrjHandle, 0x5b, g_resourceTypeTags[c_resTagShp], 0);
		if (noTarget) {
			VFX_pane_wipe(p_panel->m_target, 0);
			VFX_shape_draw(p_panel->m_target, noTarget, 0, 1, 1);
			OutlinePane(p_panel->m_target, 8);
			UnlockCachedResource(0x5b, g_resourceTypeTags[c_resTagShp]);
		}

		return;
	}
	else if (!object->m_shape) {
		noObject = LoadCachedResource(g_mw2PrjHandle, 0x58, g_resourceTypeTags[c_resTagShp], 0);
		if (noObject) {
			VFX_pane_wipe(p_panel->m_target, 0);
			VFX_shape_draw(p_panel->m_target, noObject, 0, 1, 1);
			OutlinePane(p_panel->m_target, 8);
			UnlockCachedResource(0x58, g_resourceTypeTags[c_resTagShp]);
		}

		return;
	}

	if (kind == 0x400) {
		distance = GetShapeBounds(object->m_shape, &x, &y, &z) * 3;
	}
	else {
		targetIndex = player->m_targetInfo.m_target & 0xff;
		distance = g_players[targetIndex]->m_mech->m_radius * 3;
	}

	dx = -FixedMul29(FixedSin(heading), distance);
	view[0] = x + dx;
	view[1] = y;
	dz = -FixedMul29(FixedCos(heading), distance);
	view[2] = z + dz;
	view[3] = heading;
	view[4] = 0;
	view[5] = 0;
	if (g_targetPanelMode == 1) {
		g_renderSettings.m_wireframe = 1;
		g_renderSettings.m_wireframeColors = 0;
	}
	else {
		g_renderSettings.m_wireframe = 0;
	}

	g_renderSettings.m_drawSky = g_renderSettings.m_drawGround = 0;
	VFX_pane_wipe(p_panel->m_target, 0);
	if (g_cockpitPowerState == 2) {
		RenderViewToPane(7, 0x20000, view, object);
	}

	OutlinePane(p_panel->m_target, 8);
	g_renderSettings = saved;
}

// Draws a panel as static (animation 0).
// FUNCTION: MW2 0x1007c6df
void DrawTargetStatic(CockpitPanel* p_panel)
{
	if (!p_panel->m_enabled) {
		return;
	}

	p_panel->m_lastPowerState = g_cockpitPowerState;
	DrawPanelAnim(p_panel->m_target, 0, 0, 0);
}

// Draws the target panel while its transition opens it, the view resized to the transition's
// rectangle (g_panes[7] and the panel's target) for the frame.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1007c71e
void DrawTargetPanelStartup(CockpitPanel* p_panel)
{
	PANE* rect;
	RectTransition* transition;
	PANE savedView;
	PANE savedTarget;

	if (!p_panel->m_enabled || !g_targetPanelMode) {
		return;
	}

	transition = p_panel->m_transition;
	if (transition) {
		if (p_panel->m_lastPowerState != 1) {
			StartRectTransition(transition);
		}

		rect = UpdateRectTransitionByAxis(0, transition);
		if (rect) {
			savedView = g_panes[7];
			savedTarget = *p_panel->m_target;
			g_panes[7] = *rect;
			*p_panel->m_target = *rect;
			DrawTargetPanel(p_panel);
			g_panes[7] = savedView;
			*p_panel->m_target = savedTarget;
		}
		else {
			DrawTargetPanel(p_panel);
		}
	}

	p_panel->m_lastPowerState = g_cockpitPowerState;
}

// Draws the target panel while its transition closes it, unless the view it last drew (0, 3 or
// 4) already had it closed.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1007c81c
void DrawTargetPanelShutdown(CockpitPanel* p_panel)
{
	PANE* rect;
	RectTransition* transition;
	PANE savedView;
	PANE savedTarget;

	if (!p_panel->m_enabled || !g_targetPanelMode) {
		return;
	}

	transition = p_panel->m_transition;
	if (transition) {
		if (p_panel->m_lastPowerState != 0 && p_panel->m_lastPowerState != 3 && p_panel->m_lastPowerState != 4) {
			StartRectTransition(transition);
		}

		rect = UpdateRectTransitionByAxis(1, transition);
		if (rect) {
			savedView = g_panes[7];
			savedTarget = *p_panel->m_target;
			g_panes[7] = *rect;
			*p_panel->m_target = *rect;
			DrawTargetPanel(p_panel);
			g_panes[7] = savedView;
			*p_panel->m_target = savedTarget;
		}
	}

	p_panel->m_lastPowerState = g_cockpitPowerState;
}
