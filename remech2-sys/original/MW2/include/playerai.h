#ifndef PLAYERAI_H
#define PLAYERAI_H

#include "aimessage.h"
#include "types.h"

// A player's AI state (Player::m_ai): the posted message, its state and its target and goal.
typedef struct PlayerAi {
	AiMessage m_posted; // 0x00
	int16_t m_state;    // 0x06
	uint16_t m_target;  // 0x08
	uint16_t m_goal;    // 0x0a
	uint16_t m_value;   // 0x0c — the mech's (GetMechValue): AssignStarTarget matches it
	int16_t m_flags;    // 0x0e
} PlayerAi;

#endif // PLAYERAI_H
