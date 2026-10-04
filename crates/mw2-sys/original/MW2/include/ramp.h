#ifndef RAMP_H
#define RAMP_H

#include "decomp.h"
#include "types.h"

// SIZE 0x10
typedef struct Ramp {
	MechS32 m_time;     // 0x00 — the clock at the last update
	MechS32 m_target;   // 0x04
	MechS32 m_value;    // 0x08
	MechS32 m_duration; // 0x0c — in clock ticks
} Ramp;

// A ramp whose value wraps around a period (an angle).
// SIZE 0x14
typedef struct WrappedRamp {
	MechS32 m_time;     // 0x00
	MechS32 m_target;   // 0x04
	MechS32 m_value;    // 0x08
	MechS32 m_duration; // 0x0c
	MechS32 m_period;   // 0x10
} WrappedRamp;

// SIZE 0x0c
typedef struct EasedValue {
	MechS32 m_value;  // 0x00
	MechS32 m_target; // 0x04
	MechS32 m_shift;  // 0x08 — each step closes 1 / 2^m_shift of the distance
} EasedValue;

// The functions of ramp.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 StartRamp(Ramp* p_ramp, MechS32 p_target, MechS32 p_value, MechDouble p_seconds);
	MechS32 UpdateRamp(Ramp* p_ramp);
	MechS32 StartWrappedRamp(
		WrappedRamp* p_ramp,
		MechS32 p_target,
		MechS32 p_value,
		MechDouble p_seconds,
		MechS32 p_period
	);
	MechS32 UpdateWrappedRamp(WrappedRamp* p_ramp);
	MechS32 SetWrappedRampTarget(WrappedRamp* p_ramp, MechS32 p_target);
	MechS32 UpdateEasedValue(EasedValue* p_value);

#ifdef __cplusplus
}
#endif

#endif // RAMP_H
