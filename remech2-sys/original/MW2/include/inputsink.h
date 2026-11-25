#ifndef INPUTSINK_H
#define INPUTSINK_H

#include "types.h"

// A sink: a named game control and the variable the input layer writes.
typedef struct InputSink {
	char* m_name;
	void* m_output; // an axis's int32_t, or a button's int8_t
	int32_t m_kind; // c_inputSinkAxis, c_inputSinkButton or c_inputSinkPress
	int32_t m_rest; // an axis's value at rest
	int32_t m_min;  // an axis's range
	int32_t m_max;
	int32_t m_rampShift;   // how fast the _plus and _minus keys move the axis
	int32_t m_outputShift; // the axis's 16.16 value is shifted right by this
} InputSink;

// The kinds of sink.
enum {
	c_inputSinkAxis = 0,   // an analog value
	c_inputSinkButton = 1, // set while its keys are held
	c_inputSinkPress = 2   // set once per press
};

#endif // INPUTSINK_H
