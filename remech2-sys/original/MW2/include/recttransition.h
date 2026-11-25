#ifndef RECTTRANSITION_H
#define RECTTRANSITION_H

#include "pane.h"
#include "types.h"

// LerpPaneRect's axes.
enum {
	c_rectAxisBoth = 0,
	c_rectAxisHorizontal = 1,
	c_rectAxisVertical = 2
};

typedef struct RectTransitionState {
	int32_t m_active;
	int32_t m_wasActive; // m_active at the previous update
	int32_t m_elapsed;   // in clock ticks
} RectTransitionState;

typedef struct RectTransitionDef {
	int32_t m_duration; // in clock ticks
	PANE* m_first;
	PANE* m_second;
	PANE* m_out; // the rectangle the transition moves
} RectTransitionDef;

typedef struct RectTransition {
	RectTransitionState* m_state;
	RectTransitionDef* m_def;
} RectTransition;

// The functions of recttransition.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void StartRectTransition(RectTransition* p_transition);
	void StopRectTransition(RectTransition* p_transition);
	PANE* LerpPaneRect(PANE* p_from, PANE* p_to, PANE* p_out, int32_t p_t, int32_t p_axis);
	PANE* UpdateRectTransition(int32_t p_reverse, RectTransition* p_transition);
	PANE* UpdateRectTransitionByAxis(int32_t p_reverse, RectTransition* p_transition);

#ifdef __cplusplus
}
#endif

#endif // RECTTRANSITION_H
