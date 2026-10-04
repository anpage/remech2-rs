#ifndef NETIO_H
#define NETIO_H

#include "dplay.h"
#include "types.h"

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
