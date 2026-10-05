#ifndef NETWORK_H
#define NETWORK_H

#include "dplay.h"
#include "types.h"

struct NetLaunchInfo;

#pragma pack(push, 1)

// DA: a player's state, filled from the local player every g_stateInterval ticks.
// SIZE 0x85
typedef struct NetStateMsg {
	MechChar m_tag[2];       // 0x00
	MechU8 m_player;         // 0x02
	MechS16 m_target;        // 0x03 — the player's target (PlayerTargetInfo::m_target)
	MechS16 m_killer;        // 0x05
	MechU16 m_flags;         // 0x07 — the power state in the low nibble
	MechS32 m_rearArmor3;    // 0x09 — m_sections[3].m_armor[1]
	MechS32 m_rearArmor2;    // 0x0d — m_sections[2]'s
	MechS32 m_rearArmor1;    // 0x11 — m_sections[1]'s
	MechS32 m_frontArmor[8]; // 0x15 — each section's m_armor[0]
	MechS32 m_internal[8];   // 0x35 — each section's m_unk0x08
	MechS32 m_clock;         // 0x55
	MechS32 m_x;             // 0x59
	MechS32 m_y;             // 0x5d
	MechS32 m_z;             // 0x61
	MechS32 m_pitch;         // 0x65
	MechS32 m_heading;       // 0x69
	MechS32 m_roll;          // 0x6d
	MechS32 m_speed;         // 0x71
	MechS32 m_throttle;      // 0x75
	MechS32 m_turnRate;      // 0x79
	MechS32 m_torsoTwist;    // 0x7d
	MechS32 m_torsoPitch;    // 0x81
} NetStateMsg;

// WE: the weapons a player fired since the last one, a bit each.
// SIZE 0x05
typedef struct NetWeaponsMsg {
	MechChar m_tag[2]; // 0x00
	MechU8 m_player;   // 0x02
	MechU16 m_weapons; // 0x03
} NetWeaponsMsg;

// CO: a collision push, sent to the player pushed.
// SIZE 0x1a
typedef struct NetCollisionMsg {
	MechChar m_tag[2];   // 0x00
	MechS32 m_normalX;   // 0x02
	MechS32 m_normalY;   // 0x06
	MechS32 m_normalZ;   // 0x0a
	MechS32 m_velocityX; // 0x0e — the pusher's Mech::m_newVelocityX
	MechS32 m_velocityY; // 0x12
	MechS32 m_velocityZ; // 0x16
} NetCollisionMsg;

// CH: a chat line.
// SIZE 0x52
typedef struct NetChatMsg {
	MechChar m_tag[2];     // 0x00
	MechChar m_text[0x50]; // 0x02
} NetChatMsg;

#pragma pack(pop)

// The functions and globals of network.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_isNetworkGame;
	extern MechS32 g_netRole;
	extern MechChar* g_sessionName;
	extern LPDIRECTPLAY g_directPlay;
	extern DPID g_localDpid;
	extern MechChar* g_netRecvBuffer;
	extern MechS32 g_stateInterval;
	extern MechS16 g_clockSynced;
	extern MechS16 g_launchedByShell;
	extern MechS32 g_stateCount;
	extern MechS32 g_messagesReceived;
	extern MechS32 g_unrecognizedMessages;
	extern MechS32 g_successSent;
	extern MechS16 g_gameStarted;
	extern struct NetLaunchInfo* g_netLaunch;
	extern NetStateMsg* g_stateMsg;
	extern NetWeaponsMsg* g_weaponsMsg;
	extern MechU8* g_thingsMsg;
	extern NetChatMsg* g_chatMsg;
	extern MechS32 g_netState;
	extern GUID g_sessionGuid;
	extern DPID g_masterDpid;
	extern LPGUID g_serviceProvider;
	extern MechS32 g_nextStateTime;
	extern void* g_unk0x101770cc;
	extern MechU32 g_stateMsgSize;
	extern MechS32 g_lastHeard[8];
	extern MechS32 g_lastStateClock[8];
	extern MechS8 g_playerReady[8];
	extern MechS32 g_playersFound;
	extern MechU32 g_thingsMsgSize;
	extern MechS8 g_playerDestroyed[8];
	extern MechS32 g_lastThingsCount[8];

	MechS32 GetPlayerSlotFromNetId(DPID p_id);
	void FirstNetwork(struct NetLaunchInfo* p_netLaunch);
	MechS32 FirstExternalCtrl(void);
	MechS32 UpdateNetwork(void);
	void ShutdownNetwork(void);
	MechS32 SendChatMsg(MechS32 p_to, MechChar* p_text);
	void SendCollisionMsg(MechS32 p_slot, MechS32 p_x, MechS32 p_y, MechS32 p_z);
	MechS32 StartExternalIO(struct NetLaunchInfo* p_netLaunch);
	MechS32 StopExternalIO(void);
	void ElectMaster(void);
	void SendSuccessMsg(void);

#ifdef __cplusplus
}
#endif

#endif // NETWORK_H
