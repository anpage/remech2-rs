#include "weaponpanel.h"

#include "clock.h"
#include "cockpitpanel.h"
#include "loadres.h"
#include "mech.h"
#include "menu.h"
#include "mw2prj.h"
#include "players.h"
#include "point.h"
#include "screenscale.h"
#include "setres.h"
#include "simmain.h"
#include "targeting.h"
#include "types.h"

#include <stdio.h>

// Draws a weapon panel: the name of the local mech's weapon p_panel->m_weapon with its ammo,
// in the color of its state (and the weapon group's, while it is ready), and frames the panel
// in that color if the weapon is selected.
void DrawWeaponPanel(CockpitPanel* p_panel)
{
	Mech* mech;
	int32_t color;
	void* font;
	char text[64];
	WeaponSlot* weapon;

	if (!p_panel->m_enabled || p_panel->m_weapon < 0) {
		return;
	}

	mech = g_players[g_localPlayerId]->m_mech;
	weapon = &mech->m_weapons[p_panel->m_weapon];
	if (weapon->m_type < 0) {
		return;
	}

	switch (weapon->m_state) {
	case 1:
		switch (weapon->m_group) {
		case 0:
			color = 0xe;
			break;
		case 1:
			color = 0xfe;
			break;
		case 2:
			color = 3;
			break;
		}
		break;
	case 0:
		color = 0xb;
		break;
	case -1:
		color = 8;
		break;
	case 2:
		color = 0xe;
		break;
	default:
		color = 0xb;
		break;
	}

	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
	if (font) {
		g_textColors[0xe] = color;
		if (weapon->m_ammo < 0) {
			sprintf(text, "%s", p_panel->m_name);
			VFX_string_draw(
				p_panel->m_target,
				p_panel->m_textOrigin->m_x,
				p_panel->m_textOrigin->m_y,
				font,
				text,
				g_textColors
			);
		}
		else {
			sprintf(text, "%s %d", p_panel->m_name, weapon->m_ammo);
			VFX_string_draw(
				p_panel->m_target,
				p_panel->m_textOrigin->m_x,
				p_panel->m_textOrigin->m_y,
				font,
				text,
				g_textColors
			);
		}

		g_textColors[0xe] = 0xe;
		UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
	}

	if (p_panel->m_weapon == mech->m_selectedWeapon) {
		OutlinePane(p_panel->m_target, color);
	}
}

// Draws a panel's name once the clock passes p_panel->m_lightUpTime, if the local mech has the
// weapon p_panel->m_weapon.
void DrawWeaponPanelStartup(CockpitPanel* p_panel)
{
	Mech* mech;
	WeaponSlot* weapon;
	int32_t clock;
	void* font;

	if (!p_panel->m_enabled || p_panel->m_weapon < 0) {
		return;
	}

	mech = g_players[g_localPlayerId]->m_mech;
	weapon = &mech->m_weapons[p_panel->m_weapon];
	if (weapon->m_type < 0) {
		return;
	}

	clock = g_currentClock;
	font = LoadCachedResource(g_mw2PrjHandle, g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
	if (!font) {
		return;
	}

	if (p_panel->m_enabled && p_panel->m_lightUpTime < clock) {
		VFX_string_draw(p_panel->m_target, 0, 0, font, p_panel->m_name, g_textColors);
	}

	UnlockCachedResource(g_artResolution + 1, g_resourceTypeTags[c_resTagFont]);
}
