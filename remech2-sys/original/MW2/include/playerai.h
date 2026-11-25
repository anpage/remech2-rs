#ifndef PLAYERAI_H
#define PLAYERAI_H

#include "aimessage.h"
#include "types.h"

// A player's AI state (Player::m_ai): the posted message, its state and its target and goal.
typedef struct PlayerAi {
	AiMessage m_posted;
	int16_t m_state;
	uint16_t m_target;
	uint16_t m_goal;
	uint16_t m_value; // the mech's (GetMechValue): AssignStarTarget matches it
	int16_t m_flags;
} PlayerAi;

#endif // PLAYERAI_H
