#ifndef INPUTSINK_H
#define INPUTSINK_H

#include "types.h"

// A sink: a named game control and the variable the input layer writes.
// SIZE 0x20
typedef struct InputSink {
	MechChar* m_name;      // 0x00
	void* m_output;        // 0x04 — an axis's MechS32, or a button's MechS8
	MechS32 m_kind;        // 0x08 — c_inputSinkAxis, c_inputSinkButton or c_inputSinkPress
	MechS32 m_rest;        // 0x0c — an axis's value at rest
	MechS32 m_min;         // 0x10 — an axis's range
	MechS32 m_max;         // 0x14
	MechS32 m_rampShift;   // 0x18 — how fast the _plus and _minus keys move the axis
	MechS32 m_outputShift; // 0x1c — the axis's 16.16 value is shifted right by this
} InputSink;

// The kinds of sink.
enum {
	c_inputSinkAxis = 0,   // an analog value
	c_inputSinkButton = 1, // set while its keys are held
	c_inputSinkPress = 2   // set once per press
};

#endif // INPUTSINK_H
