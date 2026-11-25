#ifndef INCLUDERECORD2_H
#define INCLUDERECORD2_H

#include "bwdrecord.h"
#include "types.h"

// A world stream's gamepiece record (gpspec): the stream that creates the player (RunIncludedStream),
// the player's team, AI parameters and names, and its mech's chassis and MEK configuration.
typedef struct IncludeRecord2 {
	BwdRecord m_header;
	int16_t m_mekId; // a MEK resource (LoadMechConfig)
	int16_t m_id;    // the gamepiece stream
	uint8_t m_team;
	uint8_t m_leader; // 1: the team's leader
	uint8_t m_ai;     // Player::m_aiMode; 0: the local player
	uint8_t m_reserved0x0f;
	uint16_t m_aiParams[8];       // Player::m_aiParams
	uint16_t m_events;            // event types (WidenEventFlags)
	int16_t m_flags;              // Player::m_flags
	char m_name[9];               // the stream, and the chassis
	char m_config[9];             // the MEK configuration
	char m_playerName[0x16];      // Player::m_name
	char m_playerShortName[0x16]; // Player::m_shortName
} IncludeRecord2;

#endif // INCLUDERECORD2_H
