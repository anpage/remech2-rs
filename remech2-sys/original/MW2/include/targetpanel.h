#ifndef TARGETPANEL_H
#define TARGETPANEL_H

#include "types.h"

struct CockpitPanel;

// The functions and globals of targetpanel.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_targetPanelMode;
	extern char g_anonymousInstallationName[8];
	extern int32_t g_announceTargetSide;
	extern int32_t g_targetFullNameTime;
	extern int32_t g_lastPanelTarget;
	extern int32_t g_targetPanelStatic;

	void DrawTargetPanelText(struct CockpitPanel* p_panel);
	void DrawTargetPanel(struct CockpitPanel* p_panel);
	void DrawTargetStatic(struct CockpitPanel* p_panel);
	void DrawTargetPanelStartup(struct CockpitPanel* p_panel);
	void DrawTargetPanelShutdown(struct CockpitPanel* p_panel);

#ifdef __cplusplus
}
#endif

#endif // TARGETPANEL_H
