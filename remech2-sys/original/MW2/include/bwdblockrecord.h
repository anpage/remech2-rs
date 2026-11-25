#ifndef BWDBLOCKRECORD_H
#define BWDBLOCKRECORD_H

#include "bwdrecord.h"
#include "types.h"

// The BWD record BeginBlock opens a block of the static object cache from: a box as a corner
// and a size, which may be negative.
typedef struct BwdBlockRecord {
	BwdRecord m_header;
	int32_t m_origin[3];
	int32_t m_size[3];
} BwdBlockRecord;

#endif // BWDBLOCKRECORD_H
