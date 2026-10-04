#ifndef WEAPONPANEL_H
#define WEAPONPANEL_H

#include "types.h"

struct CockpitPanel;

// The functions and globals of weaponpanel.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void DrawWeaponPanel(struct CockpitPanel* p_panel);
	void DrawWeaponPanelStartup(struct CockpitPanel* p_panel);

#ifdef __cplusplus
}
#endif

#endif // WEAPONPANEL_H
