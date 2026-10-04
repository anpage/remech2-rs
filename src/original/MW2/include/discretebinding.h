#ifndef DISCRETEBINDING_H
#define DISCRETEBINDING_H

#include "inputcondition.h"
#include "types.h"

#pragma pack(push, 1)

// An INPUT.MAP binding of buttons to a button sink.
// SIZE 0x6e
typedef struct DiscreteBinding {
	MechS8* m_output;               // 0x00
	MechS8 m_edge;                  // 0x04 — set only on the frame the buttons go down
	MechS8 m_previous;              // 0x05 — the conditions held last frame
	MechS32 m_conditionCount;       // 0x06
	MechS32 m_menuOnly;             // 0x0a
	InputCondition m_conditions[8]; // 0x0e
} DiscreteBinding;

#pragma pack(pop)

#endif // DISCRETEBINDING_H
