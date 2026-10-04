#ifndef RECTTRANSITION_H
#define RECTTRANSITION_H

#include "targeting.h"
#include "types.h"

// LerpPaneRect's axes.
enum {
	c_rectAxisBoth = 0,
	c_rectAxisHorizontal = 1,
	c_rectAxisVertical = 2
};

// SIZE 0x0c
typedef struct RectTransitionState {
	MechS32 m_active;    // 0x00
	MechS32 m_wasActive; // 0x04 — m_active at the previous update
	MechS32 m_elapsed;   // 0x08 — in clock ticks
} RectTransitionState;

// SIZE 0x10
typedef struct RectTransitionDef {
	MechS32 m_duration; // 0x00 — in clock ticks
	PANE* m_first;      // 0x04
	PANE* m_second;     // 0x08
	PANE* m_out;        // 0x0c — the rectangle the transition moves
} RectTransitionDef;

typedef struct RectTransition {
	RectTransitionState* m_state; // 0x00
	RectTransitionDef* m_def;     // 0x04
} RectTransition;

// The functions of recttransition.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void StartRectTransition(RectTransition* p_transition);
	void StopRectTransition(RectTransition* p_transition);
	PANE* LerpPaneRect(PANE* p_from, PANE* p_to, PANE* p_out, MechS32 p_t, MechS32 p_axis);
	PANE* UpdateRectTransition(MechS32 p_reverse, RectTransition* p_transition);
	PANE* UpdateRectTransitionByAxis(MechS32 p_reverse, RectTransition* p_transition);

#ifdef __cplusplus
}
#endif

#endif // RECTTRANSITION_H
