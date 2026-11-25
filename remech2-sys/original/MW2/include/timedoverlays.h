#ifndef TIMEDOVERLAYS_H
#define TIMEDOVERLAYS_H

#include "pane.h"
#include "timedoverlay.h"
#include "types.h"

// The functions and globals of timedoverlays.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern PANE g_topMessagePane;
	extern PANE g_bottomMessagePane;
	extern char g_bottomMessageText[0x100];
	extern char g_topMessageText[0x100];
	extern TimedOverlay g_timedOverlays[2];

	void LayoutMessageBoxes(void);
	int32_t ShowInGameMessage(char* p_text, int32_t p_font, int32_t p_duration, int32_t p_priority);
	void DrawTimedOverlays(void);
	void DrawTextBox(int32_t p_background, int32_t p_font, char* p_text, int32_t p_x, int32_t p_y);
	void DrawTextResourceBox(int32_t p_background, int32_t p_font, int32_t p_id, int32_t p_x, int32_t p_y);

#ifdef __cplusplus
}
#endif

#endif // TIMEDOVERLAYS_H
