#ifndef TIMEDOVERLAYS_H
#define TIMEDOVERLAYS_H

#include "decomp.h"
#include "pane.h"
#include "timedoverlay.h"
#include "types.h"

// The functions and globals of timedoverlays.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern PANE g_topMessagePane;
	extern undefined4 g_unk0x100adef4;
	extern PANE g_bottomMessagePane;
	extern undefined4 g_unk0x100adf0c;
	extern MechChar g_bottomMessageText[0x100];
	extern MechChar g_topMessageText[0x100];
	extern TimedOverlay g_timedOverlays[2];

	void LayoutMessageBoxes(void);
	MechS32 ShowInGameMessage(MechChar* p_text, MechS32 p_font, MechS32 p_duration, MechS32 p_priority);
	void DrawTimedOverlays(void);
	void DrawTextBox(MechS32 p_background, MechS32 p_font, MechChar* p_text, MechS32 p_x, MechS32 p_y);
	void DrawTextResourceBox(MechS32 p_background, MechS32 p_font, MechS32 p_id, MechS32 p_x, MechS32 p_y);

#ifdef __cplusplus
}
#endif

#endif // TIMEDOVERLAYS_H
