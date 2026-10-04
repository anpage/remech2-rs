#include "ramp.h"

#include "clock.h"
#include "types.h"

// Values that move toward a target over time, on the 181 Hz clock: linear ramps, which reach
// the target after a duration given in seconds, optionally wrapping around a period (angles),
// and eased values, which close a fixed fraction of the distance at each step.

DECOMP_SIZE_ASSERT(Ramp, 0x10)
DECOMP_SIZE_ASSERT(WrappedRamp, 0x14)
DECOMP_SIZE_ASSERT(EasedValue, 0x0c)

// FUNCTION: MW2 0x100495b0
MechS32 StartRamp(Ramp* p_ramp, MechS32 p_target, MechS32 p_value, MechDouble p_seconds)
{
	MechS32 result = FALSE;

	p_ramp->m_target = p_target;
	p_ramp->m_value = p_value;
	p_ramp->m_duration = p_seconds * 181.0;
	p_ramp->m_time = g_currentClock;
	if (p_ramp->m_duration > 0) {
		result = TRUE;
	}

	return result;
}

// FUNCTION: MW2 0x10049611
MechS32 UpdateRamp(Ramp* p_ramp)
{
	MechS32 delta;
	MechS32 step;

	delta = p_ramp->m_target - p_ramp->m_value;
	if (delta != 0) {
		step = (g_currentClock - p_ramp->m_time) * delta / p_ramp->m_duration;
		if ((delta < 0 && step <= delta) || (delta > 0 && step >= delta)) {
			p_ramp->m_value = p_ramp->m_target;
		}
		else {
			p_ramp->m_value += step;
		}
	}

	p_ramp->m_time = g_currentClock;
	return p_ramp->m_value;
}

// FUNCTION: MW2 0x100496ab
MechS32 StartWrappedRamp(WrappedRamp* p_ramp, MechS32 p_target, MechS32 p_value, MechDouble p_seconds, MechS32 p_period)
{
	MechS32 result = FALSE;

	p_ramp->m_target = p_target;
	p_ramp->m_value = p_value;
	p_ramp->m_duration = p_seconds * 181.0;
	p_ramp->m_time = g_currentClock;
	p_ramp->m_period = p_period;
	if (p_ramp->m_duration > 0) {
		result = TRUE;
	}

	return result;
}

// Moves the value the short way around the period.
// FUNCTION: MW2 0x10049715
MechS32 UpdateWrappedRamp(WrappedRamp* p_ramp)
{
	MechS32 half;
	MechS32 delta;
	MechS32 step;

	half = p_ramp->m_period >> 1;
	delta = p_ramp->m_target - p_ramp->m_value;
	delta %= p_ramp->m_period;
	if (delta > half) {
		delta -= p_ramp->m_period;
	}
	else if (delta < -half) {
		delta += p_ramp->m_period;
	}

	step = (g_currentClock - p_ramp->m_time) * delta / p_ramp->m_duration;
	if ((delta < 0 && step <= delta) || (delta > 0 && step >= delta)) {
		p_ramp->m_value = p_ramp->m_target;
	}
	else {
		p_ramp->m_value += step;
	}

	p_ramp->m_value = p_ramp->m_value % p_ramp->m_period;
	p_ramp->m_time = g_currentClock;
	return p_ramp->m_value;
}

// Sets the target, brought within half a period of zero.
// FUNCTION: MW2 0x10049805
MechS32 SetWrappedRampTarget(WrappedRamp* p_ramp, MechS32 p_target)
{
	MechS32 half;
	MechS32 target;

	half = p_ramp->m_period >> 1;
	target = p_target % p_ramp->m_period;
	if (target > half) {
		target -= p_ramp->m_period;
	}
	else if (target < -half) {
		target += p_ramp->m_period;
	}

	p_ramp->m_target = target;
	return target;
}

// FUNCTION: MW2 0x10049871
MechS32 UpdateEasedValue(EasedValue* p_value)
{
	MechS32 delta;
	MechS32 step;

	delta = p_value->m_target - p_value->m_value;
	step = delta >> p_value->m_shift;
	p_value->m_value += step;
	return p_value->m_value;
}
