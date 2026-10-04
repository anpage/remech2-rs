#ifndef INCLUDERECORD2_H
#define INCLUDERECORD2_H

#include "bwdrecord.h"
#include "decomp.h"
#include "types.h"

// A world stream's gamepiece record (gpspec): the stream that creates the player (RunIncludedStream),
// the player's team, AI parameters and names, and its mech's chassis and MEK configuration.
// SIZE 0x62
typedef struct IncludeRecord2 {
	BwdRecord m_header;               // 0x00
	MechS16 m_mekId;                  // 0x08 — a MEK resource (LoadMechConfig)
	MechS16 m_id;                     // 0x0a — the gamepiece stream
	MechU8 m_team;                    // 0x0c
	MechU8 m_leader;                  // 0x0d — 1: the team's leader
	MechU8 m_ai;                      // 0x0e — Player::m_aiMode; 0: the local player
	undefined m_unk0x0f;              // 0x0f
	MechU16 m_aiParams[8];            // 0x10 — Player::m_aiParams
	MechU16 m_events;                 // 0x20 — event types (WidenEventFlags)
	MechS16 m_flags;                  // 0x22 — Player::m_flags
	MechChar m_name[9];               // 0x24 — the stream, and the chassis
	MechChar m_config[9];             // 0x2d — the MEK configuration
	MechChar m_playerName[0x16];      // 0x36 — Player::m_name
	MechChar m_playerShortName[0x16]; // 0x4c — Player::m_shortName
} IncludeRecord2;

#endif // INCLUDERECORD2_H
