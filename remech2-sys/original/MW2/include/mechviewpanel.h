#ifndef MECHVIEWPANEL_H
#define MECHVIEWPANEL_H

#include "cockpitpanel.h"
#include "rendersettings.h"
#include "types.h"

// The functions and globals of mechviewpanel.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_mechViewStatic;

	void CycleMechViewMode(void);
	void DrawMechViewPanel(CockpitPanel* p_panel);
	void SetMechViewRenderSettings(RenderSettings* p_saved);
	void DrawMechViewStatic(CockpitPanel* p_panel);
	void DrawMechViewFrame(CockpitPanel* p_panel, int32_t p_color, int32_t p_shapeId);
	void DrawMechViewStartup(CockpitPanel* p_panel);
	void DrawMechViewShutdown(CockpitPanel* p_panel);

#ifdef __cplusplus
}
#endif

#endif // MECHVIEWPANEL_H
