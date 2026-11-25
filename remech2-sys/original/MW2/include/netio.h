#ifndef NETIO_H
#define NETIO_H

#include "dplay.h"
#include "types.h"

// The functions and globals of netio.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_sendRetries;
	extern int32_t g_recvRetries;
	extern HRESULT g_sendResult;

	void NetSend(uint8_t* p_msg, uint32_t p_size);
	void NetSendTo(DPID p_to, void* p_msg, uint32_t p_size);
	DPID NetReceive(void);

#ifdef __cplusplus
}
#endif

#endif // NETIO_H
