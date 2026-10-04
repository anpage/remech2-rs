#ifndef COCKPITPANEL_H
#define COCKPITPANEL_H

#include "decomp.h"
#include "point.h"
#include "targeting.h"
#include "types.h"

typedef struct CockpitPanel CockpitPanel;

/* One of the 26 cockpit panels InitCockpitPanels allocates (g_cockpitPanels): a named rectangle
   of a pane with a table of methods. InitCockpitPanel sets the defaults, and InitCockpitPanels
   installs the drawing handlers, which UpdateCockpit calls by the local mech's power state. */
// SIZE 0x88
struct CockpitPanel {
	CockpitPanel* m_self;  // 0x00
	MechS16 m_enabled;     // 0x04
	MechS16 m_damage;      // 0x06 — hits step it (DamageCockpitPanels); the panel flickers or shows static as it rises
	MechS32 m_lightUpTime; // 0x08 — the clock time the panel lights up at on startup
	MechS32 m_weapon;      // 0x0c — the weapon a weapon panel shows, -1: none
	MechChar m_name[0x20]; // 0x10
	PANE* m_target;        // 0x30
	struct Point* m_textOrigin;                          // 0x34 — where its text goes
	struct RectTransition* m_transition;                 // 0x38
	MechS32 m_lastPowerState;                            // 0x3c — the power state it last drew in (g_cockpitPowerState)
	MechS16 m_x;                                         // 0x40
	MechS16 m_y;                                         // 0x42
	MechS16 m_width;                                     // 0x44
	MechS16 m_height;                                    // 0x46
	void (*m_init)(CockpitPanel*);                       // 0x48
	void (*m_shutdown)(CockpitPanel*);                   // 0x4c — ShutdownCockpitPanels calls it
	void (*m_setLightUpTime)(CockpitPanel*, undefined4); // 0x50
	void (*m_setWeapon)(CockpitPanel*, MechS32);         // 0x54
	void (*m_setName)(CockpitPanel*, const MechChar*);   // 0x58
	void (*m_setTarget)(CockpitPanel*, PANE*);           // 0x5c
	void (*m_setTextOrigin)(CockpitPanel*, struct Point*);                // 0x60
	void (*m_setTransition)(CockpitPanel*, struct RectTransition*);       // 0x64
	void (*m_setRect)(CockpitPanel*, MechS32, MechS32, MechS32, MechS32); // 0x68
	void (*m_setDamage)(CockpitPanel*, MechS32);                          // 0x6c
	void (*m_enable)(CockpitPanel*);                                      // 0x70
	void (*m_disable)(CockpitPanel*);                                     // 0x74
	void (*m_drawStartup)();  // 0x78 — while the mech starts up (power state 1)
	void (*m_draw)();         // 0x7c — while it runs (2)
	void (*m_drawShutdown)(); // 0x80 — in the other states
	void (*m_drawStatic)();   // 0x84 — the panel as static; nothing calls it through here
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
	void SetCockpitPanelLightUpTime(CockpitPanel* p_panel, undefined4 p_lightUpTime);
	void SetCockpitPanelWeapon(CockpitPanel* p_panel, MechS32 p_weapon);
	void SetCockpitPanelName(CockpitPanel* p_panel, const MechChar* p_name);
	void SetCockpitPanelTarget(CockpitPanel* p_panel, PANE* p_target);
	void SetCockpitPanelTextOrigin(CockpitPanel* p_panel, Point* p_textOrigin);
	void SetCockpitPanelTransition(CockpitPanel* p_panel, struct RectTransition* p_transition);
	void SetCockpitPanelRect(CockpitPanel* p_panel, MechS32 p_x, MechS32 p_y, MechS32 p_width, MechS32 p_height);
	void SetCockpitPanelDamage(CockpitPanel* p_panel, MechS32 p_damage);
	void EnableCockpitPanel(CockpitPanel* p_panel);
	void DisableCockpitPanel(CockpitPanel* p_panel);

#ifdef __cplusplus
}
#endif

#endif // COCKPITPANEL_H
