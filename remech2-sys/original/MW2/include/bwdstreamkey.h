#ifndef BWDSTREAMKEY_H
#define BWDSTREAMKEY_H

#include "types.h"

// The key OpenBwdStream looks a stream up by: a resource id, or -1 to use the name.
typedef struct BwdStreamKey {
	int16_t m_id;
	char m_name[0x0e];
} BwdStreamKey;

#endif // BWDSTREAMKEY_H
