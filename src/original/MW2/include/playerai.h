#ifndef PLAYERAI_H
#define PLAYERAI_H

#include "aimessage.h"
#include "decomp.h"
#include "types.h"

// A player's AI state (Player::m_ai): the posted message, its state and its target and goal.
// SIZE 0x10
typedef struct PlayerAi {
	AiMessage m_posted;   // 0x00
	undefined2 m_unk0x04; // 0x04 — nothing reads or writes it
	MechS16 m_state;      // 0x06
	MechU16 m_target;     // 0x08
	MechU16 m_goal;       // 0x0a
	MechU16 m_value;      // 0x0c — the mech's (GetMechValue): AssignStarTarget matches it
	MechS16 m_flags;      // 0x0e
} PlayerAi;

#endif // PLAYERAI_H
