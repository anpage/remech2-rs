#ifndef AISTACKENTRY_H
#define AISTACKENTRY_H

#include "types.h"

// An AI state saved by a T_PUSH transition, for T_POP to return to.
typedef struct AiStackEntry {
	int16_t m_state;
	uint16_t m_target;
} AiStackEntry;

#endif // AISTACKENTRY_H
