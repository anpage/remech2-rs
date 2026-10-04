#ifndef AISTACKENTRY_H
#define AISTACKENTRY_H

#include "types.h"

// An AI state saved by a T_PUSH transition, for T_POP to return to.
// SIZE 0x04
typedef struct AiStackEntry {
	MechS16 m_state;  // 0x00
	MechU16 m_target; // 0x02
} AiStackEntry;

#endif // AISTACKENTRY_H
