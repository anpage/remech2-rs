#ifndef NETLAUNCHINFO_H
#define NETLAUNCHINFO_H

#include "decomp.h"
#include "types.h"

#include <dplay.h>
#include <windows.h>

// What the shell hands SimMain for a network game: its DirectPlay session, the local player's
// id and the ids of the players, by slot.
typedef struct NetLaunchInfo {
	LPDIRECTPLAY m_directPlay;        // 0x00
	DPID m_localPlayerId;             // 0x04
	undefined4 m_unk0x08;             // 0x08
	undefined4 m_unk0x0c;             // 0x0c
	DPID* m_playerIds;                // 0x10
	undefined4 m_unk0x14;             // 0x14
	undefined m_unk0x18[0x24 - 0x18]; // 0x18
	char* m_missionName;              // 0x24
} NetLaunchInfo;

#endif // NETLAUNCHINFO_H
