#ifndef PLAYERTYPE_H
#define PLAYERTYPE_H

#include "players.h"
#include "types.h"

// A kind of player a world stream's gamepiece record creates (g_playerTypes): its callbacks,
// copied into the Player.
typedef struct PlayerType {
	int32_t m_type;           // Player::m_type
	PlayerCreatedFn m_create; // CreateSimPlayer's
	void (*m_firstClassFn)(Player* p_player);
	PlayerMechFn m_updateFn;
	PlayerMechFn m_lateUpdateFn;
	PlayerMechFn m_localUpdateFn; // the local player's only
	PlayerMechFn m_drawFn;        // the local player's only
	PlayerMechFn m_shutdownFn;
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
