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

	extern MechS32 g_chatRecipient;
	extern MechS32 g_showObjectives;
	extern MechChar g_chatMessage[0x30];
	extern MechChar g_ticksText[16];
	extern MechChar g_secondsText[16];

	MechChar* FormatTicks(MechS32 p_ticks);
	MechChar* FormatSeconds(MechS32 p_seconds);
	MechS32 GetTextWidth(const MechChar* p_text, void* p_font);
	void DrawObjectiveList(struct CockpitPanel* p_panel, struct Point* p_pos, void* p_font, MechU8 p_priority);
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
