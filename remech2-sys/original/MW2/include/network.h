#ifndef NETWORK_H
#define NETWORK_H

#include "dplay.h"
#include "types.h"

struct NetLaunchInfo;

#pragma pack(push, 1)

// DA: a player's state, filled from the local player every g_stateInterval ticks.
typedef struct NetStateMsg {
	char m_tag[2];           // 0x00
	uint8_t m_player;        // 0x02
	int16_t m_target;        // 0x03 — the player's target (PlayerTargetInfo::m_target)
	int16_t m_killer;        // 0x05
	uint16_t m_flags;        // 0x07 — the power state in the low nibble
	int32_t m_rearArmor3;    // 0x09 — m_sections[3].m_armor[1]
	int32_t m_rearArmor2;    // 0x0d — m_sections[2]'s
	int32_t m_rearArmor1;    // 0x11 — m_sections[1]'s
	int32_t m_frontArmor[8]; // 0x15 — each section's m_armor[0]
	int32_t m_internal[8];   // 0x35 — each section's m_internal
	int32_t m_clock;         // 0x55
	int32_t m_x;             // 0x59
	int32_t m_y;             // 0x5d
	int32_t m_z;             // 0x61
	int32_t m_pitch;         // 0x65
	int32_t m_heading;       // 0x69
	int32_t m_roll;          // 0x6d
	int32_t m_speed;         // 0x71
	int32_t m_throttle;      // 0x75
	int32_t m_turnRate;      // 0x79
	int32_t m_torsoTwist;    // 0x7d
	int32_t m_torsoPitch;    // 0x81
} NetStateMsg;

// WE: the weapons a player fired since the last one, a bit each.
typedef struct NetWeaponsMsg {
	char m_tag[2];      // 0x00
	uint8_t m_player;   // 0x02
	uint16_t m_weapons; // 0x03
} NetWeaponsMsg;

// CO: a collision push, sent to the player pushed.
typedef struct NetCollisionMsg {
	char m_tag[2];       // 0x00
	int32_t m_normalX;   // 0x02
	int32_t m_normalY;   // 0x06
	int32_t m_normalZ;   // 0x0a
	int32_t m_velocityX; // 0x0e — the pusher's Mech::m_newVelocityX
	int32_t m_velocityY; // 0x12
	int32_t m_velocityZ; // 0x16
} NetCollisionMsg;

// CH: a chat line.
typedef struct NetChatMsg {
	char m_tag[2];     // 0x00
	char m_text[0x50]; // 0x02
} NetChatMsg;

#pragma pack(pop)

// The functions and globals of network.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_isNetworkGame;
	extern int32_t g_netRole;
	extern char* g_sessionName;
	extern LPDIRECTPLAY g_directPlay;
	extern DPID g_localDpid;
	extern char* g_netRecvBuffer;
	extern int32_t g_stateInterval;
	extern int16_t g_clockSynced;
	extern int16_t g_launchedByShell;
	extern int32_t g_stateCount;
	extern int32_t g_messagesReceived;
	extern int32_t g_unrecognizedMessages;
	extern int32_t g_successSent;
	extern int16_t g_gameStarted;
	extern struct NetLaunchInfo* g_netLaunch;
	extern NetStateMsg* g_stateMsg;
	extern NetWeaponsMsg* g_weaponsMsg;
	extern uint8_t* g_thingsMsg;
	extern NetChatMsg* g_chatMsg;
	extern int32_t g_netState;
	extern GUID g_sessionGuid;
	extern DPID g_masterDpid;
	extern LPGUID g_serviceProvider;
	extern int32_t g_nextStateTime;
	extern uint32_t g_stateMsgSize;
	extern int32_t g_lastHeard[8];
	extern int32_t g_lastStateClock[8];
	extern int8_t g_playerReady[8];
	extern int32_t g_playersFound;
	extern uint32_t g_thingsMsgSize;
	extern int8_t g_playerDestroyed[8];
	extern int32_t g_lastThingsCount[8];

	int32_t GetPlayerSlotFromNetId(DPID p_id);
	void FirstNetwork(struct NetLaunchInfo* p_netLaunch);
	int32_t FirstExternalCtrl(void);
	int32_t UpdateNetwork(void);
	void ShutdownNetwork(void);
	int32_t SendChatMsg(int32_t p_to, char* p_text);
	void SendCollisionMsg(int32_t p_slot, int32_t p_x, int32_t p_y, int32_t p_z);
	int32_t StartExternalIO(struct NetLaunchInfo* p_netLaunch);
	int32_t StopExternalIO(void);
	void ElectMaster(void);
	void SendSuccessMsg(void);

#ifdef __cplusplus
}
#endif

#endif // NETWORK_H
