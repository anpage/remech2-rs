#ifndef AIRULE_H
#define AIRULE_H

#include "types.h"

#pragma pack(push, 1)

// One rule of an AI script: on a message about a target, make a transition and enter a new
// state with a new target. The target fields hold AI target ids, or '@' (the current goal),
// '%' (the target the message matched) or '*' (the current target).
typedef struct AiRule {
	int16_t m_message;
	uint16_t m_target;
	int16_t m_arg;
	uint8_t m_transition;
	int16_t m_newState;
	int16_t m_newTarget;
	uint8_t m_reserved0x0b;
} AiRule;

#pragma pack(pop)

#endif // AIRULE_H
