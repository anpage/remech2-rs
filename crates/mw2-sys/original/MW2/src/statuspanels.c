#include "statuspanels.h"

#include "approxlen.h"
#include "bargauges.h"
#include "clock.h"
#include "cockpitpanel.h"
#include "config.h"
#include "decomp.h"
#include "eyepoint.h"
#include "gamekeys.h"
#include "loadres.h"
#include "mech.h"
#include "mechclass.h"
#include "mechdamage.h"
#include "menu.h"
#include "mw2prj.h"
#include "network.h"
#include "objective.h"
#include "players.h"
#include "point.h"
#include "screenscale.h"
#include "setres.h"
#include "simmain.h"
#include "starmission.h"
#include "targeting.h"
#include "team.h"
#include "types.h"

#include <stdio.h>
#include <string.h>

// Who the chat message goes to: 0 nobody (no message is being typed), -1 everybody, or a
// player.
// GLOBAL: MW2 0x100a116c
MechS32 g_chatRecipient = 0;

// Whether the objectives panel shows.
// GLOBAL: MW2 0x100a1170
MechS32 g_showObjectives = 0;

// GLOBAL: MW2 0x100bcd88
MechChar g_ticksText[16];

// GLOBAL: MW2 0x100bcd98
MechChar g_secondsText[16];

// The chat message being typed. HandleChatKey edits it.
// GLOBAL: MW2 0x10179e90
MechChar g_chatMessage[0x30];

// Formats a tick count (181 per second) as hours:minutes:seconds.
// FUNCTION: MW2 0x10004f40
MechChar* FormatTicks(MechS32 p_ticks)
{
	MechDouble seconds;
	MechS32 hours;
	MechS32 minutes;

	hours = p_ticks / (181 * 3600);
	minutes = (p_ticks - hours * (181 * 3600)) / (181 * 60);
	seconds = (p_ticks - hours * (181 * 3600) - minutes * (181 * 60)) / 181.0;
	sprintf(g_ticksText, "%2.2d:%2.2d:%05.2f", hours, minutes, seconds);
	return g_ticksText;
}

// Formats a count of seconds as hours:minutes:seconds.
// FUNCTION: MW2 0x10004ff5
MechChar* FormatSeconds(MechS32 p_seconds)
{
	MechS32 seconds;
	MechS32 hours;
	MechS32 minutes;

	hours = p_seconds / 3600;
	minutes = (p_seconds - hours * 3600) / 60;
	seconds = p_seconds - hours * 3600 - minutes * 60;
	sprintf(g_secondsText, "%2.2d:%2.2d:%2.2d", hours, minutes, seconds);
	return g_secondsText;
}

// Matches except for the stack slots of width and c (a consistent permutation).
// Returns the width of p_text in p_font.
// FUNCTION: MW2 0x1000507d
MechS32 GetTextWidth(const MechChar* p_text, void* p_font)
{
	MechS32 width;
	const MechChar* c;

	width = 0;
	for (c = p_text; *c; c++) {
		width += VFX_character_width(p_font, *c);
	}

	return width;
}

// Lists the objectives of priority p_priority the local player's star can see, from p_pos, each
// with its state at the right edge of the panel. A name longer than 31 characters takes two
// lines. In a network game whose only listed objective is a secondary one, it counts as primary.
// Stack-slot permutation of the locals (label, objective, twoLines, height, i, target, mission,
// primary, secondary, count, x and y).
// FUNCTION: MW2 0x100050d1
void DrawObjectiveList(CockpitPanel* p_panel, Point* p_pos, void* p_font, MechU8 p_priority)
{
	PANE* target;
	StarMission* mission;
	MechS32 height;
	MechS32 primary;
	MechS32 count;
	MechS32 secondary;
	MechS32 i;
	MissionObjective* objective;
	MechChar* label;
	MechS32 twoLines;
	MechS32 x;
	MechS32 y;
	MechChar text[256];

	target = p_panel->m_target;
	mission = &g_objectiveTable[g_localStar];
	height = VFX_font_height(p_font);
	primary = FALSE;
	if (g_isNetworkGame && !g_difficulty->m_teamGame) {
		count = 0;
		secondary = FALSE;
		for (i = 0; i < mission->m_objectiveCount; i++) {
			objective = &g_objectiveTable[g_localStar].m_objectives[i];
			count += objective->m_listed;
			if (objective->m_listed && objective->m_priority == 2) {
				secondary = TRUE;
			}
		}

		if (secondary && count == 1) {
			primary = TRUE;
		}
	}

	for (i = 0; i < mission->m_objectiveCount; i++) {
		objective = &g_objectiveTable[g_localStar].m_objectives[i];
		if (!objective->m_listed || objective->m_priority != p_priority) {
			continue;
		}

		{
			p_pos->m_x = p_panel->m_textOrigin->m_x;
			twoLines = FALSE;
			switch (p_priority) {
			case 1:
				label = "Primary: ";
				break;
			case 2:
				if (primary) {
					label = "Primary: ";
				}
				else {
					label = "Secondary: ";
				}
				break;
			case 4:
				label = "Tertiary: ";
				break;
			case 8:
				label = "Return: ";
				break;
			default:
				label = "Tertiary: ";
				break;
			}

			g_textColors[0xe] = 6;
			VFX_string_draw(p_panel->m_target, p_pos->m_x, p_pos->m_y, p_font, label, g_textColors);
			g_textColors[0xe] = 0xe;
			p_pos->m_x += GetTextWidth("Secondary: ", p_font);
			if (strlen(objective->m_name) < 0x20) {
				g_textColors[0xe] = 0xe;
				VFX_string_draw(p_panel->m_target, p_pos->m_x, p_pos->m_y, p_font, objective->m_name, g_textColors);
				g_textColors[0xe] = 0xe;
			}
			else {
				x = p_pos->m_x;
				y = p_pos->m_y;
				strncpy(text, objective->m_name, 0x20);
				text[0x20] = '\0';
				g_textColors[0xe] = 0xe;
				VFX_string_draw(p_panel->m_target, p_pos->m_x, p_pos->m_y, p_font, text, g_textColors);
				g_textColors[0xe] = 0xe;
				p_pos->m_x = x;
				p_pos->m_y += height;
				strcpy(text, objective->m_name + 0x20);
				g_textColors[0xe] = 0xe;
				VFX_string_draw(p_panel->m_target, p_pos->m_x, p_pos->m_y, p_font, text, g_textColors);
				g_textColors[0xe] = 0xe;
				p_pos->m_x = x;
				p_pos->m_y = y;
				twoLines = TRUE;
			}

			switch (objective->m_state) {
			case 5:
				sprintf(text, "Successful");
				p_pos->m_x = target->m_x1 - target->m_x0 - GetTextWidth(text, p_font);
				g_textColors[0xe] = 7;
				VFX_string_draw(p_panel->m_target, p_pos->m_x, p_pos->m_y, p_font, text, g_textColors);
				g_textColors[0xe] = 0xe;
				break;
			case 6:
				sprintf(text, "Failed");
				p_pos->m_x = target->m_x1 - target->m_x0 - GetTextWidth(text, p_font);
				g_textColors[0xe] = 0xb;
				VFX_string_draw(p_panel->m_target, p_pos->m_x, p_pos->m_y, p_font, text, g_textColors);
				g_textColors[0xe] = 0xe;
				break;
			default:
				sprintf(text, "In progress");
				p_pos->m_x = target->m_x1 - target->m_x0 - GetTextWidth(text, p_font);
				g_textColors[0xe] = 0xe;
				VFX_string_draw(p_panel->m_target, p_pos->m_x, p_pos->m_y, p_font, text, g_textColors);
				g_textColors[0xe] = 0xe;
				break;
			}

			p_pos->m_y += height;
			if (twoLines) {
				p_pos->m_y += height;
			}
		}
	}
}

// Draws the objectives panel: the objectives by priority, then the mission clock or how the
// mission ended.
// Stack-slot permutation of the locals (cursor, mission, font, height, gap, ticks and text).
// FUNCTION: MW2 0x100056f0
void DrawObjectivesPanel(CockpitPanel* p_panel)
{
	StarMission* mission;
	void* font;
	Point pos;
	Point* cursor = &pos;
	MechS32 ticks;
	MechS32 height;
	MechChar text[256];
	MechS32 gap;

	if (!p_panel->m_enabled || !g_showObjectives) {
		return;
	}

	mission = &g_objectiveTable[g_localStar];
	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (!font) {
		return;
	}

	height = VFX_font_height(font);
	gap = height / 2;
	pos = *p_panel->m_textOrigin;
	g_textColors[0xe] = 6;
	VFX_string_draw(p_panel->m_target, cursor->m_x, cursor->m_y, font, "MISSION OBJECTIVES", g_textColors);
	g_textColors[0xe] = 0xe;
	UnderlineText(p_panel->m_target, "MISSION OBJECTIVES", pos, font, 6);
	cursor->m_y += gap + height;
	DrawObjectiveList(p_panel, cursor, font, 1);
	cursor->m_y += gap;
	DrawObjectiveList(p_panel, cursor, font, 2);
	cursor->m_y += gap;
	DrawObjectiveList(p_panel, cursor, font, 4);
	DrawObjectiveList(p_panel, cursor, font, 0);
	cursor->m_y += gap;
	DrawObjectiveList(p_panel, cursor, font, 8);
	cursor->m_x = p_panel->m_textOrigin->m_x;
	cursor->m_y += gap + height;
	switch (mission->m_status) {
	case 0:
		if (mission->m_timeLimit > 0) {
			ticks = (mission->m_startTime + mission->m_timeLimit) * 181 - g_currentClock;
			sprintf(text, "Time Remaining: %s", FormatTicks(ticks));
		}
		else {
			ticks = g_currentClock - mission->m_startTime * 181;
			sprintf(text, "Elapsed Time: %s", FormatTicks(ticks));
		}
		break;
	case 2:
		sprintf(text, "Successful at %s", FormatSeconds(mission->m_endTime));
		break;
	case 4:
		sprintf(text, "Out of time at %s", FormatSeconds(mission->m_endTime));
		break;
	case 3:
		sprintf(text, "Failed at %s", FormatSeconds(mission->m_endTime));
		break;
	}

	g_textColors[0xe] = 6;
	VFX_string_draw(p_panel->m_target, cursor->m_x, cursor->m_y, font, text, g_textColors);
	g_textColors[0xe] = 0xe;
	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
}

// Draws the network status panel: whom the camera tracks, or how to regenerate, or that the
// game waits for the other players; then the chat message being typed, with its recipients and
// a cursor.
// Stack-slot permutation of the locals (state, cursor, pos, color, height, player, gap, text and font).
// FUNCTION: MW2 0x10005add
void DrawNetworkPanel(CockpitPanel* p_panel)
{
	MechChar* state;
	Point pos;
	Point* cursor = &pos;
	MechS32 color;
	MechS32 height;
	MechS32 player;
	MechS32 gap;
	MechChar text[40];
	void* font;

	color = 0xe;
	state = "";
	if (!p_panel->m_enabled) {
		return;
	}

	if (!g_statusMessage && !g_chatRecipient) {
		return;
	}

	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (!font) {
		return;
	}

	height = VFX_font_height(font);
	gap = height / 2;
	pos = *p_panel->m_textOrigin;
	if (g_statusMessage == 1) {
		if (GetPlayerSide(g_trackedPlayer)) {
			color = 0xb;
		}

		if (g_players[g_trackedPlayer]->m_flags & 6) {
			state = "(dead)";
		}

		sprintf(text, "Tracking: %s %s", g_players[g_trackedPlayer]->m_name, state);
		g_textColors[0xe] = color;
		VFX_string_draw(p_panel->m_target, cursor->m_x, cursor->m_y, font, text, g_textColors);
		g_textColors[0xe] = 0xe;
	}
	else if (g_statusMessage == 2) {
		g_textColors[0xe] = 6;
		VFX_string_draw(
			p_panel->m_target,
			cursor->m_x,
			cursor->m_y,
			font,
			"Press SPACEBAR to regenerate, or CTRL-Q to exit.",
			g_textColors
		);
		g_textColors[0xe] = 0xe;
	}
	else if (g_statusMessage == 3) {
		g_textColors[0xe] = 0xe;
		VFX_string_draw(
			p_panel->m_target,
			cursor->m_x,
			cursor->m_y,
			font,
			"         Waiting for remote players...",
			g_textColors
		);
		g_textColors[0xe] = 0xe;
	}

	cursor->m_y += gap + height;
	if (g_chatRecipient) {
		switch (g_chatRecipient) {
		case -1:
			g_textColors[0xe] = 0xe;
			VFX_string_draw(p_panel->m_target, cursor->m_x, cursor->m_y, font, "Communication", g_textColors);
			g_textColors[0xe] = 0xe;
			UnderlineText(p_panel->m_target, "Communication", pos, font, 2);
			cursor->m_y += gap + height;
			if (g_difficulty->m_teamGame) {
				g_textColors[0xe] = 0xe;
				VFX_string_draw(
					p_panel->m_target,
					cursor->m_x,
					cursor->m_y,
					font,
					" [Enter] Send to all",
					g_textColors
				);
				g_textColors[0xe] = 0xe;
				cursor->m_y += height;
				g_textColors[0xe] = 0xe;
				VFX_string_draw(
					p_panel->m_target,
					cursor->m_x,
					cursor->m_y,
					font,
					" CTRL-F  Send to friendly mechs",
					g_textColors
				);
				g_textColors[0xe] = 0xe;
				cursor->m_y += height;
				g_textColors[0xe] = 0xe;
				VFX_string_draw(
					p_panel->m_target,
					cursor->m_x,
					cursor->m_y,
					font,
					" CTRL-E  Send to enemy mechs",
					g_textColors
				);
				g_textColors[0xe] = 0xe;
				cursor->m_y += height;
				g_textColors[0xe] = 0xe;
				VFX_string_draw(p_panel->m_target, cursor->m_x, cursor->m_y, font, " [Esc] to abort", g_textColors);
				g_textColors[0xe] = 0xe;
			}
			else {
				g_textColors[0xe] = 0xe;
				VFX_string_draw(
					p_panel->m_target,
					cursor->m_x,
					cursor->m_y,
					font,
					" [Enter] Send to all,    [Esc] to abort",
					g_textColors
				);
				g_textColors[0xe] = 0xe;
			}

			cursor->m_y += gap + height;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			if (g_chatRecipient == g_localPlayerId) {
				player = 0;
			}
			else {
				player = g_chatRecipient;
			}

			if (!g_players[player] || g_players[player]->m_type != c_playerTypeMech) {
				return;
			}

			if (g_players[player]->m_flags & 0x4800) {
				return;
			}

			sprintf(text, "Communication to %s", g_players[player]->m_name);
			g_textColors[0xe] = 0xe;
			VFX_string_draw(p_panel->m_target, cursor->m_x, cursor->m_y, font, text, g_textColors);
			g_textColors[0xe] = 0xe;
			UnderlineText(p_panel->m_target, text, pos, font, 2);
			cursor->m_y += gap + height;
			g_textColors[0xe] = 0xe;
			VFX_string_draw(
				p_panel->m_target,
				cursor->m_x,
				cursor->m_y,
				font,
				" [Enter] to send,    [ESC] to abort",
				g_textColors
			);
			g_textColors[0xe] = 0xe;
			cursor->m_y += gap + height;
			break;
		}

		g_textColors[0xe] = 0xe;
		VFX_string_draw(p_panel->m_target, cursor->m_x, cursor->m_y, font, g_chatMessage, g_textColors);
		g_textColors[0xe] = 0xe;
		BoxText(p_panel->m_target, "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM", pos, font, 6);
		cursor->m_x = GetTextWidth(g_chatMessage, font);
		UnderlineText(p_panel->m_target, " ", pos, font, 0xe);
	}

	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
}

// Shows the local player's kill count in a network game.
// FUNCTION: MW2 0x100060b6
void DrawKillsPanel(CockpitPanel* p_panel)
{
	void* font;
	MechChar text[40];

	if (!p_panel->m_enabled) {
		return;
	}

	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (!font) {
		return;
	}

	if (g_isNetworkGame && !g_statusMessage && !g_localMechLost) {
		sprintf(text, "Kills: %i", g_killCount);
		VFX_string_draw(
			p_panel->m_target,
			p_panel->m_textOrigin->m_x,
			p_panel->m_textOrigin->m_y,
			font,
			text,
			g_textColors
		);
	}

	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
}

// Shows the autopilot's state.
// Stack-slot permutation of text and font.
// FUNCTION: MW2 0x10006189
void DrawAutopilotPanel(CockpitPanel* p_panel)
{
	MechChar* text;
	Mech* mech;
	void* font;

	if (!p_panel->m_enabled) {
		return;
	}

	mech = g_players[g_localPlayerId]->m_mech;
	switch (mech->m_autopilot) {
	case 1:
		text = "AUTOPILOT";
		break;
	case 2:
		text = "AUTOPILOT";
		break;
	default:
		text = "";
		return;
	}

	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (!font) {
		return;
	}

	if (*text) {
		VFX_string_draw(
			p_panel->m_target,
			p_panel->m_textOrigin->m_x,
			p_panel->m_textOrigin->m_y,
			font,
			text,
			g_textColors
		);
	}

	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
}

// Stack-slot permutation of mech and font.
// Shows the local mech's speed in kph, in a different color while it backs up, and its throttle bar.
// FUNCTION: MW2 0x10006291
void DrawSpeedPanel(CockpitPanel* p_panel)
{
	MechS32 speed;
	Mech* mech;
	MechChar text[64];
	void* font;

	if (!p_panel->m_enabled) {
		return;
	}

	mech = g_players[g_localPlayerId]->m_mech;
	speed = ApproximateVectorLength(mech->m_velocityX, mech->m_velocityY, mech->m_velocityZ) / 10002 * 1.5;
	if (mech->m_speed.m_value < 0) {
		speed = -speed;
		g_textColors[0xe] = 6;
	}
	else {
		g_textColors[0xe] = 0xe;
	}

	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (!font) {
		return;
	}

	sprintf(text, "%d kph", speed);
	VFX_string_draw(
		p_panel->m_target,
		p_panel->m_textOrigin->m_x,
		p_panel->m_textOrigin->m_y,
		font,
		text,
		g_textColors
	);
	g_textColors[0xe] = 0xe;
	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
	DrawThrottleGauge(p_panel->m_target);
}

// Labels the MASC panel while MASC is available.
// FUNCTION: MW2 0x100063cd
void DrawMascPanel(CockpitPanel* p_panel)
{
	void* font;

	if (!p_panel->m_enabled || !g_mascEngaged) {
		return;
	}

	p_panel->m_setName(p_panel, "MASC");
	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (!font) {
		return;
	}

	VFX_string_draw(
		p_panel->m_target,
		p_panel->m_textOrigin->m_x,
		p_panel->m_textOrigin->m_y,
		font,
		p_panel->m_name,
		g_textColors
	);
	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
}

// Stack-slot permutation of mech and font.
// Labels the heat panel with the shutdown state and draws the heat bar.
// FUNCTION: MW2 0x10006484
void DrawHeatPanel(CockpitPanel* p_panel)
{
	Mech* mech;
	void* font;

	mech = g_players[g_localPlayerId]->m_mech;
	if (!p_panel->m_enabled) {
		return;
	}

	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (!font) {
		return;
	}

	if (mech->m_flags & 4) {
		if (!(mech->m_flags & 8)) {
			p_panel->m_setName(p_panel, "Shutdown...");
			g_textColors[0xe] = 0xb;
		}
		else {
			p_panel->m_setName(p_panel, "Overridden");
			g_textColors[0xe] = 0xb;
		}
	}
	else {
		p_panel->m_setName(p_panel, "Heat");
		g_textColors[0xe] = 0xe;
	}

	VFX_string_draw(
		p_panel->m_target,
		p_panel->m_textOrigin->m_x,
		p_panel->m_textOrigin->m_y,
		font,
		p_panel->m_name,
		g_textColors
	);
	g_textColors[0xe] = 0xe;
	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
	DrawHeatBar(p_panel->m_target);
}

// Labels the heat rate panel and draws its bar.
// FUNCTION: MW2 0x100065c3
void DrawHeatRatePanel(CockpitPanel* p_panel)
{
	void* font;

	if (!p_panel->m_enabled) {
		return;
	}

	p_panel->m_setName(p_panel, "dH/dT");
	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (!font) {
		return;
	}

	VFX_string_draw(
		p_panel->m_target,
		p_panel->m_textOrigin->m_x,
		p_panel->m_textOrigin->m_y,
		font,
		p_panel->m_name,
		g_textColors
	);
	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
	DrawHeatRateBar(p_panel->m_target);
}

// Labels the jump jet panel of a mech with jump jets and draws the fuel bar.
// FUNCTION: MW2 0x1000667c
void DrawJetsPanel(CockpitPanel* p_panel)
{
	void* font;

	if (!p_panel->m_enabled) {
		return;
	}

	if (g_players[g_localPlayerId]->m_mech->m_jumpFuel < 0) {
		return;
	}

	p_panel->m_setName(p_panel, "Jets");
	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont], 0);
	if (!font) {
		return;
	}

	VFX_string_draw(
		p_panel->m_target,
		p_panel->m_textOrigin->m_x,
		p_panel->m_textOrigin->m_y,
		font,
		p_panel->m_name,
		g_textColors
	);
	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
	DrawJumpFuelBar(p_panel->m_target);
}
