#ifndef AIRULE_H
#define AIRULE_H

#include "types.h"

#pragma pack(push, 1)

// One rule of an AI script: on a message about a target, make a transition and enter a new
// state with a new target. The target fields hold AI target ids, or '@' (the current goal),
// '%' (the target the message matched) or '*' (the current target).
typedef struct AiRule {
	int16_t m_message;    // 0x00
	uint16_t m_target;    // 0x02
	int16_t m_arg;        // 0x04
	uint8_t m_transition; // 0x06
	int16_t m_newState;   // 0x07
	int16_t m_newTarget;  // 0x09
	uint8_t m_unk0x0b;    // 0x0b
} AiRule;

#pragma pack(pop)

#endif // AIRULE_H
