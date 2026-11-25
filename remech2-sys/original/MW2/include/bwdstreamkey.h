#ifndef BWDSTREAMKEY_H
#define BWDSTREAMKEY_H

#include "types.h"

// The key OpenBwdStream looks a stream up by: a resource id, or -1 to use the name.
typedef struct BwdStreamKey {
	int16_t m_id;      // 0x00
	char m_name[0x0e]; // 0x02
} BwdStreamKey;

#endif // BWDSTREAMKEY_H
