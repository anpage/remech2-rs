#include "ramp.h"

#include "clock.h"
#include "types.h"

// Values that move toward a target over time, on the 181 Hz clock: linear ramps, which reach
// the target after a duration given in seconds, optionally wrapping around a period (angles),
// and eased values, which close a fixed fraction of the distance at each step.

int32_t StartRamp(Ramp* p_ramp, int32_t p_target, int32_t p_value, double p_seconds)
{
	int32_t result = FALSE;

	p_ramp->m_target = p_target;
	p_ramp->m_value = p_value;
	p_ramp->m_duration = p_seconds * 181.0;
	p_ramp->m_time = g_currentClock;
	if (p_ramp->m_duration > 0) {
		result = TRUE;
	}

	return result;
}

int32_t UpdateRamp(Ramp* p_ramp)
{
	int32_t delta;
	int32_t step;

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

int32_t StartWrappedRamp(WrappedRamp* p_ramp, int32_t p_target, int32_t p_value, double p_seconds, int32_t p_period)
{
	int32_t result = FALSE;

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
int32_t UpdateWrappedRamp(WrappedRamp* p_ramp)
{
	int32_t half;
	int32_t delta;
	int32_t step;

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
int32_t SetWrappedRampTarget(WrappedRamp* p_ramp, int32_t p_target)
{
	int32_t half;
	int32_t target;

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
