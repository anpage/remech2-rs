#ifndef NETWORK_H
#define NETWORK_H

#include "dplay.h"
#include "types.h"

struct NetLaunchInfo;

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
