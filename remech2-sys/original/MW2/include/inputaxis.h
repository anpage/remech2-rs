#ifndef INPUTAXIS_H
#define INPUTAXIS_H

#include "types.h"

#pragma pack(push, 1)

// The state of an axis sink that analog bindings drive: its position and the keys that move it.
// SIZE 0x32
typedef struct InputAxis {
	MechS32* m_output;    // 0x00 — the sink's output
	MechS32* m_rate;      // 0x04 — m_ownRate, or m_position for a _delta sink
	MechS32 m_range;      // 0x08 — max - min
	MechS32 m_minFixed;   // 0x0c — min (16.16)
	MechS32 m_rest;       // 0x10 — the rest position, between -1 and 1 (16.16)
	MechU8 m_outputShift; // 0x14
	MechS8 m_driven;      // 0x15 — a binding moved the axis this frame
	MechS32 m_ownRate;    // 0x16
	MechS32 m_position;   // 0x1a — between -1 and 1 (16.16)
	MechS32 m_rampShift;  // 0x1e
	MechS8* m_plusHeld;   // 0x22 — the outputs of the sink's _plus, _minus, _reset and _set sinks
	MechS8* m_minusHeld;  // 0x26
	MechS8* m_resetHeld;  // 0x2a
	MechS8* m_setHeld;    // 0x2e
} InputAxis;

#pragma pack(pop)

#endif // INPUTAXIS_H
