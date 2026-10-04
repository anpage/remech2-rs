#ifndef NETIO_H
#define NETIO_H

#include "types.h"

#include <dplay.h>
#include <windows.h>

#ifndef DPSEND_TRYONCE
#define DPSEND_GUARANTEE 0x00000001
#define DPSEND_TRYONCE 0x00000004
#define DPOPEN_OPENSESSION 0x00000001
#define DPOPEN_CREATESESSION 0x00000002
#endif

// The functions and globals of netio.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void NetSend(MechU8* p_msg, MechU32 p_size);
	void NetSendTo(DPID p_to, void* p_msg, MechU32 p_size);
	DPID NetReceive(void);

#ifdef __cplusplus
}
#endif

#endif // NETIO_H
