#ifndef RAMP_H
#define RAMP_H

#include "types.h"

typedef struct Ramp {
	int32_t m_time; // the clock at the last update
	int32_t m_target;
	int32_t m_value;
	int32_t m_duration; // in clock ticks
} Ramp;

// A ramp whose value wraps around a period (an angle).
typedef struct WrappedRamp {
	int32_t m_time;
	int32_t m_target;
	int32_t m_value;
	int32_t m_duration;
	int32_t m_period;
} WrappedRamp;

typedef struct EasedValue {
	int32_t m_value;
	int32_t m_target;
	int32_t m_shift; // each step closes 1 / 2^m_shift of the distance
} EasedValue;

// The functions of ramp.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t StartRamp(Ramp* p_ramp, int32_t p_target, int32_t p_value, double p_seconds);
	int32_t UpdateRamp(Ramp* p_ramp);
	int32_t StartWrappedRamp(
		WrappedRamp* p_ramp,
		int32_t p_target,
		int32_t p_value,
		double p_seconds,
		int32_t p_period
	);
	int32_t UpdateWrappedRamp(WrappedRamp* p_ramp);
	int32_t SetWrappedRampTarget(WrappedRamp* p_ramp, int32_t p_target);
	// Implemented on the Rust side (src/sim/timing.rs)
	int32_t UpdateEasedValue(EasedValue* p_value);

#ifdef __cplusplus
}
#endif

#endif // RAMP_H
