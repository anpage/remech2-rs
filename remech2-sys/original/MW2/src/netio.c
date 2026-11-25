#include "netio.h"

#include "clock.h"
#include "debugprint.h"
#include "elapsed.h"
#include "network.h"
#include "types.h"

// The consecutive failed sends and receives: each one after the first waits before the next.

int32_t g_sendRetries = 0;

int32_t g_recvRetries = 0;

// The result of the last broadcast, or 5 after a failed one: a send after a failure goes out
// guaranteed.
HRESULT g_sendResult;

// Sends a message to every player of the session.
void NetSend(uint8_t* p_msg, uint32_t p_size)
{
	DWORD flags;

	if (g_sendResult >= 5) {
		flags = DPSEND_GUARANTEE;
	}
	else {
		flags = DPSEND_TRYONCE;
	}

	g_sendResult = g_directPlay->lpVtbl->Send(g_directPlay, g_localDpid, 0, flags, p_msg, p_size);
	if (g_sendResult == DPERR_BUSY) {
		DebugPrint("Send '%c%c' --- Busy=%d\n", p_msg[0], p_msg[1], g_sendResult);
		switch (g_sendRetries++) {
		case 0:
		case 1:
			break;
		case 2:
			MechSleep(100);
			break;
		case 3:
			MechSleep(200);
			break;
		default:
			g_sendRetries = 0;
			break;
		}

		g_sendResult = 5;
	}
	else if (g_sendResult) {
		if (g_sendResult < 0) {
			DebugPrint("Send '%c%c' --- Error %d\n", g_sendResult & 0xff);
		}
		else {
			DebugPrint("Send '%c%c' --- %d outgoing messages\n", p_msg[0], p_msg[1], g_sendResult);
		}

		if (g_sendRetries++) {
			MechSleep(100);
		}

		g_sendResult = 5;
	}
	else {
		g_sendRetries = 0;
	}
}

// Sends a message to one player.
void NetSendTo(DPID p_to, void* p_msg, uint32_t p_size)
{
	g_directPlay->lpVtbl->Send(g_directPlay, g_localDpid, p_to, DPSEND_TRYONCE, p_msg, p_size);
}

// Receives the next message into g_netRecvBuffer. Returns the sender, or -1 when there is none.
DPID NetReceive(void)
{
	DPID from;
	HRESULT result;
	DWORD size;
	DPID to;

	size = 0x100;
	result = g_directPlay->lpVtbl->Receive(g_directPlay, &from, &to, DPRECEIVE_ALL, g_netRecvBuffer, &size);
	if (result == DP_OK) {
		g_recvRetries = 0;
		DebugPrint("Recv '%c%c' from %d (%i)\n", g_netRecvBuffer[0], g_netRecvBuffer[1], from, g_currentClock);
		return from;
	}
	else if (result == DPERR_NOMESSAGES) {
	}
	else {
		DebugPrint("Recv Bad Packet\n");
		switch (g_recvRetries++) {
		case 0:
			break;
		case 1:
			MechSleep(100);
			break;
		default:
			MechSleep(100);
			break;
		}
	}

	return -1;
}
