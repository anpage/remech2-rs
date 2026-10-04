#ifndef AIRULE_H
#define AIRULE_H

#include "types.h"

#pragma pack(push, 1)

// One rule of an AI script: on a message about a target, make a transition and enter a new
// state with a new target. The target fields hold AI target ids, or '@' (the current goal),
// '%' (the target the message matched) or '*' (the current target).
// SIZE 0x0c
typedef struct AiRule {
	MechS16 m_message;   // 0x00
	MechU16 m_target;    // 0x02
	MechS16 m_arg;       // 0x04
	MechU8 m_transition; // 0x06
	MechS16 m_newState;  // 0x07
	MechS16 m_newTarget; // 0x09
	MechU8 m_unk0x0b;    // 0x0b
} AiRule;

#pragma pack(pop)

#endif // AIRULE_H
