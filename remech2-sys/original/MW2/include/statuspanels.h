#ifndef STATUSPANELS_H
#define STATUSPANELS_H

#include "decomp.h"
#include "types.h"

struct CockpitPanel;
struct Point;

// The functions and globals of statuspanels.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_chatRecipient;
	extern int32_t g_showObjectives;
	extern char g_chatMessage[0x30];
	extern char g_ticksText[16];
	extern char g_secondsText[16];

	char* FormatTicks(int32_t p_ticks);
	char* FormatSeconds(int32_t p_seconds);
	int32_t GetTextWidth(const char* p_text, void* p_font);
	void DrawObjectiveList(struct CockpitPanel* p_panel, struct Point* p_pos, void* p_font, uint8_t p_priority);
	void DrawObjectivesPanel(struct CockpitPanel* p_panel);
	void DrawNetworkPanel(struct CockpitPanel* p_panel);
	void DrawKillsPanel(struct CockpitPanel* p_panel);
	void DrawAutopilotPanel(struct CockpitPanel* p_panel);
	void DrawSpeedPanel(struct CockpitPanel* p_panel);
	void DrawMascPanel(struct CockpitPanel* p_panel);
	void DrawHeatPanel(struct CockpitPanel* p_panel);
	void DrawHeatRatePanel(struct CockpitPanel* p_panel);
	void DrawJetsPanel(struct CockpitPanel* p_panel);

#ifdef __cplusplus
}
#endif

#endif // STATUSPANELS_H
