#ifndef NETWORK_H
#define NETWORK_H

#include "dplay.h"
#include "types.h"

struct NetLaunchInfo;

#pragma pack(push, 1)

// DA: a player's state, filled from the local player every g_stateInterval ticks.
typedef struct NetStateMsg {
	char m_tag[2];
	uint8_t m_player;
	int16_t m_target; // the player's target (PlayerTargetInfo::m_target)
	int16_t m_killer;
	uint16_t m_flags;        // the power state in the low nibble
	int32_t m_rearArmor3;    // m_sections[3].m_armor[1]
	int32_t m_rearArmor2;    // m_sections[2]'s
	int32_t m_rearArmor1;    // m_sections[1]'s
	int32_t m_frontArmor[8]; // each section's m_armor[0]
	int32_t m_internal[8];   // each section's m_internal
	int32_t m_clock;
	int32_t m_x;
	int32_t m_y;
	int32_t m_z;
	int32_t m_pitch;
	int32_t m_heading;
	int32_t m_roll;
	int32_t m_speed;
	int32_t m_throttle;
	int32_t m_turnRate;
	int32_t m_torsoTwist;
	int32_t m_torsoPitch;
} NetStateMsg;

// WE: the weapons a player fired since the last one, a bit each.
typedef struct NetWeaponsMsg {
	char m_tag[2];
	uint8_t m_player;
	uint16_t m_weapons;
} NetWeaponsMsg;

// CO: a collision push, sent to the player pushed.
typedef struct NetCollisionMsg {
	char m_tag[2];
	int32_t m_normalX;
	int32_t m_normalY;
	int32_t m_normalZ;
	int32_t m_velocityX; // the pusher's Mech::m_newVelocityX
	int32_t m_velocityY;
	int32_t m_velocityZ;
} NetCollisionMsg;

// CH: a chat line.
typedef struct NetChatMsg {
	char m_tag[2];
	char m_text[0x50];
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
