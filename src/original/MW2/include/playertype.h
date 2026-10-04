#ifndef PLAYERTYPE_H
#define PLAYERTYPE_H

#include "players.h"
#include "types.h"

// A kind of player a world stream's gamepiece record creates (g_playerTypes): its callbacks,
// copied into the Player.
// SIZE 0x20
typedef struct PlayerType {
	MechS32 m_type;                           // 0x00 — Player::m_type
	PlayerCreatedFn m_create;                 // 0x04 — CreateSimPlayer's
	void (*m_firstClassFn)(Player* p_player); // 0x08
	PlayerMechFn m_updateFn;                  // 0x0c
	PlayerMechFn m_lateUpdateFn;              // 0x10
	PlayerMechFn m_localUpdateFn;             // 0x14 — the local player's only
	PlayerMechFn m_drawFn;                    // 0x18 — the local player's only
	PlayerMechFn m_shutdownFn;                // 0x1c
} PlayerType;

// The table of players.c.
#ifdef __cplusplus
extern "C"
{
#endif

	extern PlayerType g_playerTypes[9];

#ifdef __cplusplus
}
#endif

#endif // PLAYERTYPE_H
