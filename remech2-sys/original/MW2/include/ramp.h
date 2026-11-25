#ifndef RAMP_H
#define RAMP_H

#include "types.h"

typedef struct Ramp {
	int32_t m_time;     // 0x00 — the clock at the last update
	int32_t m_target;   // 0x04
	int32_t m_value;    // 0x08
	int32_t m_duration; // 0x0c — in clock ticks
} Ramp;

// A ramp whose value wraps around a period (an angle).
typedef struct WrappedRamp {
	int32_t m_time;     // 0x00
	int32_t m_target;   // 0x04
	int32_t m_value;    // 0x08
	int32_t m_duration; // 0x0c
	int32_t m_period;   // 0x10
} WrappedRamp;

typedef struct EasedValue {
	int32_t m_value;  // 0x00
	int32_t m_target; // 0x04
	int32_t m_shift;  // 0x08 — each step closes 1 / 2^m_shift of the distance
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
