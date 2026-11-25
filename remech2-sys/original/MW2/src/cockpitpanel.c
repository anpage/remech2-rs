#include "cockpitpanel.h"

#include "recttransition.h"
#include "targeting.h"
#include "types.h"

#include <string.h>

void InitCockpitPanel(CockpitPanel* p_panel)
{
	p_panel->m_self = p_panel;
	p_panel->m_damage = 0;
	p_panel->m_lightUpTime = 0;
	p_panel->m_weapon = -1;
	memset(p_panel->m_name, 0, sizeof(p_panel->m_name));
	p_panel->m_x = 0;
	p_panel->m_y = 0;
	p_panel->m_width = 0;
	p_panel->m_height = 0;
	p_panel->m_init = InitCockpitPanel;
	p_panel->m_shutdown = ShutdownCockpitPanel;
	p_panel->m_setLightUpTime = SetCockpitPanelLightUpTime;
	p_panel->m_setWeapon = SetCockpitPanelWeapon;
	p_panel->m_setName = SetCockpitPanelName;
	p_panel->m_setTarget = SetCockpitPanelTarget;
	p_panel->m_setTextOrigin = SetCockpitPanelTextOrigin;
	p_panel->m_setTransition = SetCockpitPanelTransition;
	p_panel->m_setRect = SetCockpitPanelRect;
	p_panel->m_setDamage = SetCockpitPanelDamage;
	p_panel->m_enable = EnableCockpitPanel;
	p_panel->m_disable = DisableCockpitPanel;
	p_panel->m_drawStartup = NULL;
	p_panel->m_draw = NULL;
	p_panel->m_drawShutdown = NULL;
	p_panel->m_drawStatic = NULL;
}

void ShutdownCockpitPanel(CockpitPanel* p_panel)
{
}

void SetCockpitPanelLightUpTime(CockpitPanel* p_panel, uint32_t p_lightUpTime)
{
	p_panel->m_lightUpTime = p_lightUpTime;
}

void SetCockpitPanelWeapon(CockpitPanel* p_panel, int32_t p_weapon)
{
	p_panel->m_weapon = p_weapon;
}

void SetCockpitPanelName(CockpitPanel* p_panel, const char* p_name)
{
	strncpy(p_panel->m_name, p_name, sizeof(p_panel->m_name));
	p_panel->m_name[sizeof(p_panel->m_name) - 1] = '\0';
}

void SetCockpitPanelTarget(CockpitPanel* p_panel, PANE* p_target)
{
	p_panel->m_target = p_target;
	p_panel->m_x = p_target->m_x0;
	p_panel->m_y = p_target->m_y0;
	p_panel->m_width = p_target->m_x1 - p_target->m_x0 + 1;
	p_panel->m_height = p_target->m_y1 - p_target->m_y0 + 1;
}

void SetCockpitPanelTextOrigin(CockpitPanel* p_panel, Point* p_textOrigin)
{
	p_panel->m_textOrigin = p_textOrigin;
}

void SetCockpitPanelTransition(CockpitPanel* p_panel, RectTransition* p_transition)
{
	p_panel->m_transition = p_transition;
}

void SetCockpitPanelRect(CockpitPanel* p_panel, int32_t p_x, int32_t p_y, int32_t p_width, int32_t p_height)
{
	p_panel->m_x = p_x;
	p_panel->m_y = p_y;
	p_panel->m_width = p_width;
	p_panel->m_height = p_height;
}

void SetCockpitPanelDamage(CockpitPanel* p_panel, int32_t p_damage)
{
	p_panel->m_damage = p_damage;
}

void EnableCockpitPanel(CockpitPanel* p_panel)
{
	p_panel->m_enabled = 1;
}

void DisableCockpitPanel(CockpitPanel* p_panel)
{
	p_panel->m_enabled = 0;
}
