#include "recttransition.h"

#include "clock.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "targeting.h"
#include "types.h"

// Timed transitions of a pane's rectangle between two others: the edges move
// together, or the horizontal and vertical edges one after the other.

// Arms the transition: the next update starts it from its first rectangle.
void StartRectTransition(RectTransition* p_transition)
{
	p_transition->m_state->m_active = TRUE;
	p_transition->m_state->m_wasActive = FALSE;
}

void StopRectTransition(RectTransition* p_transition)
{
	p_transition->m_state->m_active = FALSE;
	p_transition->m_state->m_wasActive = FALSE;
}

// Moves p_out's edges to the fraction p_t (16.16) of the way from p_from to p_to: the horizontal
// ones unless p_axis is 2, the vertical ones unless it is 1.
PANE* LerpPaneRect(PANE* p_from, PANE* p_to, PANE* p_out, int32_t p_t, int32_t p_axis)
{
	int32_t delta;

	if (p_axis != c_rectAxisVertical) {
		delta = p_to->m_x0 - p_from->m_x0;
		p_out->m_x0 = p_from->m_x0 + FixedMul16(p_t, delta);
		delta = p_to->m_x1 - p_from->m_x1;
		p_out->m_x1 = p_from->m_x1 + FixedMul16(p_t, delta);
	}

	if (p_axis != c_rectAxisHorizontal) {
		delta = p_to->m_y0 - p_from->m_y0;
		p_out->m_y0 = p_from->m_y0 + FixedMul16(p_t, delta);
		delta = p_to->m_y1 - p_from->m_y1;
		p_out->m_y1 = p_from->m_y1 + FixedMul16(p_t, delta);
	}

	return p_out;
}

// Advances the transition, forward or (p_reverse) backward, and returns its rectangle, or NULL
// when it isn't running.
PANE* UpdateRectTransition(int32_t p_reverse, RectTransition* p_transition)
{
	int32_t t;
	RectTransitionDef* def;
	int32_t active;
	PANE* from;
	RectTransitionState* state;
	PANE* out;
	PANE* to;

	state = p_transition->m_state;
	def = p_transition->m_def;
	out = NULL;
	active = state->m_active;
	if (active == TRUE) {
		out = def->m_out;
		if (!p_reverse) {
			from = def->m_first;
			to = def->m_second;
		}
		else {
			from = def->m_second;
			to = def->m_first;
		}

		if (!state->m_wasActive) {
			*out = *from;
			state->m_elapsed = 0;
		}
		else {
			state->m_elapsed += g_deltaTime;
			t = FixedDiv16(state->m_elapsed, def->m_duration);
			if (t >= 0x10000) {
				*out = *to;
				state->m_elapsed = 0;
				active = FALSE;
			}
			else {
				LerpPaneRect(from, to, out, t, c_rectAxisBoth);
			}
		}
	}

	state->m_wasActive = state->m_active;
	state->m_active = active;
	return out;
}

// As UpdateRectTransition, but the horizontal edges move in the first half and the vertical ones
// in the second (the other way around in reverse).
PANE* UpdateRectTransitionByAxis(int32_t p_reverse, RectTransition* p_transition)
{
	int32_t t;
	RectTransitionDef* def;
	int32_t active;
	PANE* from;
	int32_t firstAxis;
	int32_t secondAxis;
	RectTransitionState* state;
	PANE* out;
	PANE* to;

	state = p_transition->m_state;
	def = p_transition->m_def;
	out = NULL;
	active = state->m_active;
	if (active == TRUE) {
		out = def->m_out;
		if (!p_reverse) {
			from = def->m_first;
			to = def->m_second;
			firstAxis = c_rectAxisHorizontal;
			secondAxis = c_rectAxisVertical;
		}
		else {
			from = def->m_second;
			to = def->m_first;
			firstAxis = c_rectAxisVertical;
			secondAxis = c_rectAxisHorizontal;
		}

		if (!state->m_wasActive) {
			*out = *from;
			state->m_elapsed = 0;
		}
		else {
			state->m_elapsed += g_deltaTime;
			t = FixedDiv16(state->m_elapsed, def->m_duration);
			if (t >= 0x10000) {
				*out = *to;
				state->m_elapsed = 0;
				active = FALSE;
			}
			else if (t <= 0x8000) {
				*out = *from;
				t = FixedMul16(t, 0x20000);
				LerpPaneRect(from, to, out, t, firstAxis);
			}
			else {
				*out = *to;
				t -= 0x8000;
				t = FixedMul16(t, 0x20000);
				LerpPaneRect(from, to, out, t, secondAxis);
			}
		}
	}

	state->m_wasActive = state->m_active;
	state->m_active = active;
	return out;
}
