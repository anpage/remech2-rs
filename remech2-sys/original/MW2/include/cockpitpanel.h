#ifndef COCKPITPANEL_H
#define COCKPITPANEL_H

#include "point.h"
#include "targeting.h"
#include "types.h"

typedef struct CockpitPanel CockpitPanel;

/* One of the 26 cockpit panels InitCockpitPanels allocates (g_cockpitPanels): a named rectangle
   of a pane with a table of methods. InitCockpitPanel sets the defaults, and InitCockpitPanels
   installs the drawing handlers, which UpdateCockpit calls by the local mech's power state. */
struct CockpitPanel {
	CockpitPanel* m_self;
	int16_t m_enabled;
	int16_t m_damage;      // hits step it (DamageCockpitPanels); the panel flickers or shows static as it rises
	int32_t m_lightUpTime; // the clock time the panel lights up at on startup
	int32_t m_weapon;      // the weapon a weapon panel shows, -1: none
	char m_name[0x20];
	PANE* m_target;
	struct Point* m_textOrigin; // where its text goes
	struct RectTransition* m_transition;
	int32_t m_lastPowerState; // the power state it last drew in (g_cockpitPowerState)
	int16_t m_x;
	int16_t m_y;
	int16_t m_width;
	int16_t m_height;
	void (*m_init)(CockpitPanel*);
	void (*m_shutdown)(CockpitPanel*); // ShutdownCockpitPanels calls it
	void (*m_setLightUpTime)(CockpitPanel*, uint32_t);
	void (*m_setWeapon)(CockpitPanel*, int32_t);
	void (*m_setName)(CockpitPanel*, const char*);
	void (*m_setTarget)(CockpitPanel*, PANE*);
	void (*m_setTextOrigin)(CockpitPanel*, struct Point*);
	void (*m_setTransition)(CockpitPanel*, struct RectTransition*);
	void (*m_setRect)(CockpitPanel*, int32_t, int32_t, int32_t, int32_t);
	void (*m_setDamage)(CockpitPanel*, int32_t);
	void (*m_enable)(CockpitPanel*);
	void (*m_disable)(CockpitPanel*);
	void (*m_drawStartup)(CockpitPanel*);  // while the mech starts up (power state 1)
	void (*m_draw)(CockpitPanel*);         // while it runs (2)
	void (*m_drawShutdown)(CockpitPanel*); // in the other states
	void (*m_drawStatic)(CockpitPanel*);   // the panel as static; nothing calls it through here
};

// The cockpit panels, by index in g_cockpitPanels.
enum {
	c_panelRadar = 0, // its damage also breaks up the map views
	c_panelMechView = 2,
	c_panelFirstWeapon = 3, // to 12
	c_panelTarget = 13,
	c_panelTargetText = 14,
	c_panelObjectives = 15,
	c_panelNetwork = 16,
	c_panelAutopilot = 17,
	c_panelSpeed = 18,
	c_panelMasc = 19,
	c_panelHeat = 20,
	c_panelHeatRate = 21,
	c_panelJets = 22,
	c_panelAltimeter = 23,
	c_panelCompass = 24,
	c_panelKills = 25,
	c_panelCount = 26
};

// The functions and globals of cockpitpanel.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void InitCockpitPanel(CockpitPanel* p_panel);
	void ShutdownCockpitPanel(CockpitPanel* p_panel);
	void SetCockpitPanelLightUpTime(CockpitPanel* p_panel, uint32_t p_lightUpTime);
	void SetCockpitPanelWeapon(CockpitPanel* p_panel, int32_t p_weapon);
	void SetCockpitPanelName(CockpitPanel* p_panel, const char* p_name);
	void SetCockpitPanelTarget(CockpitPanel* p_panel, PANE* p_target);
	void SetCockpitPanelTextOrigin(CockpitPanel* p_panel, Point* p_textOrigin);
	void SetCockpitPanelTransition(CockpitPanel* p_panel, struct RectTransition* p_transition);
	void SetCockpitPanelRect(CockpitPanel* p_panel, int32_t p_x, int32_t p_y, int32_t p_width, int32_t p_height);
	void SetCockpitPanelDamage(CockpitPanel* p_panel, int32_t p_damage);
	void EnableCockpitPanel(CockpitPanel* p_panel);
	void DisableCockpitPanel(CockpitPanel* p_panel);

#ifdef __cplusplus
}
#endif

#endif // COCKPITPANEL_H
