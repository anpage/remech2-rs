#ifndef AIMESSAGE_H
#define AIMESSAGE_H

#include "types.h"

// A message posted to an AI player (PostAIMessage): a rule's message and its target, which
// the player's rules match before their own conditions. Posted to a star's leader, it is an
// order (HandleStarOrder).
typedef struct AiMessage {
	int16_t m_message;
	uint16_t m_target;
} AiMessage;

#endif // AIMESSAGE_H
