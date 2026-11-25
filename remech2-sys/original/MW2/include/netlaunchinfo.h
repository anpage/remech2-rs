#ifndef NETLAUNCHINFO_H
#define NETLAUNCHINFO_H

#include "dplay.h"
#include "types.h"

// What the shell hands SimMain for a network game: its DirectPlay session, the local player's
// id and the ids of the players, by slot.
typedef struct NetLaunchInfo {
	LPDIRECTPLAY m_directPlay;
	DPID m_localPlayerId;
	DPID* m_playerIds;
	char* m_missionName;
} NetLaunchInfo;

#endif // NETLAUNCHINFO_H
