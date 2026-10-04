#ifndef TARGETPANEL_H
#define TARGETPANEL_H

#include "types.h"

struct CockpitPanel;

// The functions and globals of targetpanel.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_targetPanelMode;
	extern MechChar g_anonymousInstallationName[8];

	void DrawTargetPanelText(struct CockpitPanel* p_panel);
	void DrawTargetPanel(struct CockpitPanel* p_panel);
	void DrawTargetStatic(struct CockpitPanel* p_panel);
	void DrawTargetPanelStartup(struct CockpitPanel* p_panel);
	void DrawTargetPanelShutdown(struct CockpitPanel* p_panel);

#ifdef __cplusplus
}
#endif

#endif // TARGETPANEL_H
