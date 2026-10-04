#ifndef ANALOGBINDING_H
#define ANALOGBINDING_H

#include "inputaxis.h"
#include "inputcondition.h"
#include "types.h"

// An INPUT.MAP binding of a device channel to an axis sink.
// SIZE 0x80
typedef struct AnalogBinding {
	MechS32 m_device;               // 0x00 — -1 for a binding of the axis's keys only
	MechS32 m_channel;              // 0x04
	MechS32* m_channelValue;        // 0x08 — the channel's polled value, or NULL
	MechS32 m_lastValue;            // 0x0c
	MechS32 m_sign;                 // 0x10 — -1 inverts the channel
	InputAxis* m_axis;              // 0x14
	MechS32 m_conditionCount;       // 0x18
	InputCondition m_conditions[8]; // 0x1c
	MechS32 m_menuOnly;             // 0x7c — a menu_ sink: active while gameplay input is off
} AnalogBinding;

#endif // ANALOGBINDING_H
