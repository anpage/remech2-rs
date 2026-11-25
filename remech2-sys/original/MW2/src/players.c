#include "players.h"

#include "artillery.h"
#include "decomp.h"
#include "door.h"
#include "gamething.h"
#include "inputmap.h"
#include "mechclass.h"
#include "playersteering.h"
#include "playertype.h"
#include "poolsizes.h"
#include "simmain.h"
#include "staticmem.h"
#include "types.h"

#include <string.h>

// The kinds of player a gamepiece record can create, by Player::m_type: 3 has its own
// callbacks (artillery.c), 7 another (door.c); the others are mechs.
PlayerType g_playerTypes[9] = {
	{0, NULL, NULL, NULL, NULL, NULL, NULL, NULL},
	{1, CreateMech, FirstMech, UpdateMech, LateUpdateMech, UpdateLocalMech, DrawMechCockpit, ShutdownMech},
	{2, CreateMech, FirstMech, UpdateMech, LateUpdateMech, UpdateLocalMech, DrawMechCockpit, ShutdownMech},
	{3, CreateArtillery, FirstArtillery, UpdateArtillery, LateUpdateArtillery, NULL, NULL, ShutdownArtillery},
	{4, CreateMech, FirstMech, UpdateMech, LateUpdateMech, UpdateLocalMech, DrawMechCockpit, ShutdownMech},
	{5, CreateMech, FirstMech, UpdateMech, LateUpdateMech, UpdateLocalMech, DrawMechCockpit, ShutdownMech},
	{6, CreateMech, FirstMech, UpdateMech, LateUpdateMech, UpdateLocalMech, DrawMechCockpit, ShutdownMech},
	{7, CreateDoor, FirstDoor, UpdateDoor, LateUpdateDoor, NULL, NULL, ShutdownDoor},
	{8, CreateMech, FirstMech, UpdateMech, LateUpdateMech, UpdateLocalMech, DrawMechCockpit, ShutdownMech},
};

int32_t g_playerCount = 0;

int32_t g_gameThingCount = 0;

// The world loader stops at 0x3c players (BwdExecuteStream), which fill the original's room
// before g_gameThings.
Player* g_players[0x3c];

GameThing g_gameThings[254];

void FirstClassFunctions(void)
{
	int32_t i;
	void (*first)(Player* p_player);

	for (i = 0; i < g_playerCount; i++) {
		if (g_players[i]->m_firstClassFn != NULL && g_players[i]->m_mech != NULL) {
			first = g_players[i]->m_firstClassFn;
			first(g_players[i]);
		}
	}
}

void UpdateAllPlayers(void)
{
	PlayerMechFn update;
	Mech* mech;
	int32_t i;

	for (i = 0; i < g_playerCount; i++) {
		update = g_players[i]->m_updateFn;
		mech = g_players[i]->m_mech;
		if (update != NULL && mech != NULL) {
			update(mech);
		}
	}
}

void LateUpdateAllPlayers(void)
{
	int32_t i;
	PlayerMechFn update;

	for (i = 0; i < g_playerCount; i++) {
		if (g_players[i]->m_lateUpdateFn != NULL && g_players[i]->m_mech != NULL) {
			update = g_players[i]->m_lateUpdateFn;
			update(g_players[i]->m_mech);
		}
	}
}

void UpdateLocalPlayer(void)
{
	Player* player;
	PlayerMechFn update;

	player = g_players[g_localPlayerId];
	if (player != NULL) {
		update = player->m_localUpdateFn;
		if (update != NULL && player->m_mech != NULL) {
			update(player->m_mech);
		}
	}
}

void DrawLocalPlayer(void)
{
	Player* player;
	PlayerMechFn draw;

	player = g_players[g_localPlayerId];
	if (player != NULL) {
		draw = player->m_drawFn;
		if (draw != NULL && player->m_mech != NULL) {
			draw(player->m_mech);
		}
	}
}

void ShutdownAllPlayers(void)
{
	PlayerMechFn shutdown;
	int32_t i;

	for (i = 0; i < g_playerCount; i++) {
		if (g_players[i]->m_shutdownFn != NULL) {
			shutdown = g_players[i]->m_shutdownFn;
			shutdown(g_players[i]->m_mech);
		}
	}
}

void ZeroGameThing(int32_t p_index)
{
	GameThing* thing;

	thing = &g_gameThings[p_index];
	thing->m_flags = 0;
	thing->m_teamsReached = 0;
	thing->m_staticObject = -1;
	thing->m_hitPoints = 0;
	thing->m_affiliation = 0;
	thing->m_name[0] = '\0';
}

void ZeroGamethings(void)
{
	int32_t i;

	for (i = 0; i < 254; i++) {
		ZeroGameThing(i);
	}
}

// Creates player p_player: allocates it, sets it up, gives it its steering (the input sinks for
// the local player, the room after it otherwise), and passes it to p_fn if given.
void CreateSimPlayer(int32_t p_player, PlayerCreatedFn p_fn)
{
	PlayerCreatedFn fn;
	Player* player;

	fn = NULL;
	g_players[p_player] = NULL;
	fn = p_fn;
	if (AllocPlayer(p_player)) {
		player = g_players[p_player];
		InitPlayer(player);
		player->m_index = p_player;
		if (p_player == g_localPlayerId) {
			player->m_steering = &g_localSteering;
		}
		else {
			player->m_steering = (PlayerSteering*) (player + 1);
		}

		if (player->m_steering) {
			memset(player->m_steering, 0, sizeof(PlayerSteering));
		}

		if (fn) {
			fn(p_player, player);
		}
	}
}

// Allocates player p_player: a remote player gets room for its steering after it.
int32_t AllocPlayer(int32_t p_player)
{
	Player* player;
	uint32_t size;

	size = sizeof(Player);
	if (p_player != g_localPlayerId) {
		size += sizeof(PlayerSteering);
	}

	player = StaticPoolAlloc(size, g_staticPoolTags[0]);
	if (!player) {
		return FALSE;
	}

	g_players[p_player] = player;
	return TRUE;
}

// Sets up a newly allocated player.
void InitPlayer(Player* p_player)
{
	p_player->m_type = 0;
	p_player->m_index = 0;
	p_player->m_team = 0;
	p_player->m_slot = 0;
	p_player->m_aiMode = 0;
	p_player->m_flags = 0;
	p_player->m_inspectedBy = 0;
	p_player->m_baseLevel = 0;
	p_player->m_detailLevel = -1;
	p_player->m_mech = NULL;
	p_player->m_mechSize = 0;
	p_player->m_firstClassFn = NULL;
	p_player->m_updateFn = NULL;
	p_player->m_lateUpdateFn = NULL;
	p_player->m_localUpdateFn = NULL;
	p_player->m_drawFn = NULL;
	p_player->m_shutdownFn = NULL;
	p_player->m_obj = NULL;
	p_player->m_eyeObj = NULL;
	p_player->m_firingObj = NULL;
	p_player->m_steering = NULL;
	p_player->m_position.m_x = p_player->m_position.m_y = p_player->m_position.m_z = 0;
	p_player->m_pitch = p_player->m_heading = p_player->m_roll = 0;
	p_player->m_torsoPitch = p_player->m_torsoTwist = p_player->m_torsoRoll = 0;
	p_player->m_groundHeight = 0;
	p_player->m_onGround = 0;
	p_player->m_collidedWith = -1;
	p_player->m_animFlags = 0;
	p_player->m_motionState = -1;
	p_player->m_nextMotionState = -1;
	p_player->m_speedLevel = 0;
	p_player->m_pendingSound = -1;
	p_player->m_animRate = 0;
	StartRamp(&p_player->m_aimRange, 50000, 50000, 0.2);
	StartRamp(&p_player->m_aimDistance, 50000, 50000, 20.0);
	p_player->m_headingSin = p_player->m_headingCos = 0;
	p_player->m_targetInfo.m_distance = 0;
	p_player->m_targetInfo.m_range = 0;
	p_player->m_targetInfo.m_position.m_x = p_player->m_targetInfo.m_position.m_y =
		p_player->m_targetInfo.m_position.m_z = 0;
	p_player->m_targetInfo.m_heading = 0;
	p_player->m_targetInfo.m_pitch = 0;
	p_player->m_targetInfo.m_target = -1;
	memset(p_player->m_name, 0, 0x16);
}
