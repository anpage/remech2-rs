#ifndef INPUTCONDITION_H
#define INPUTCONDITION_H

#include "types.h"

// A condition of an INPUT.MAP binding: bits of an input word that must be set, or clear.
// SIZE 0x0c
typedef struct InputCondition {
	MechU32* m_word;  // 0x00
	MechU32 m_mask;   // 0x04
	MechS32 m_negate; // 0x08 — 1: the bits must be clear
} InputCondition;

#endif // INPUTCONDITION_H
