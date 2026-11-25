#ifndef INCLUDERECORD2_H
#define INCLUDERECORD2_H

#include "bwdrecord.h"
#include "decomp.h"
#include "types.h"

// A world stream's gamepiece record (gpspec): the stream that creates the player (RunIncludedStream),
// the player's team, AI parameters and names, and its mech's chassis and MEK configuration.
typedef struct IncludeRecord2 {
	BwdRecord m_header;           // 0x00
	int16_t m_mekId;              // 0x08 — a MEK resource (LoadMechConfig)
	int16_t m_id;                 // 0x0a — the gamepiece stream
	uint8_t m_team;               // 0x0c
	uint8_t m_leader;             // 0x0d — 1: the team's leader
	uint8_t m_ai;                 // 0x0e — Player::m_aiMode; 0: the local player
	undefined m_unk0x0f;          // 0x0f
	uint16_t m_aiParams[8];       // 0x10 — Player::m_aiParams
	uint16_t m_events;            // 0x20 — event types (WidenEventFlags)
	int16_t m_flags;              // 0x22 — Player::m_flags
	char m_name[9];               // 0x24 — the stream, and the chassis
	char m_config[9];             // 0x2d — the MEK configuration
	char m_playerName[0x16];      // 0x36 — Player::m_name
	char m_playerShortName[0x16]; // 0x4c — Player::m_shortName
} IncludeRecord2;

#endif // INCLUDERECORD2_H
