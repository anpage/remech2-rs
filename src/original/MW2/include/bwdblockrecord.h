#ifndef BWDBLOCKRECORD_H
#define BWDBLOCKRECORD_H

#include "bwdrecord.h"
#include "types.h"

// The BWD record BeginBlock opens a block of the static object cache from: a box as a corner
// and a size, which may be negative.
// SIZE 0x20
typedef struct BwdBlockRecord {
	BwdRecord m_header;  // 0x00
	MechS32 m_origin[3]; // 0x08
	MechS32 m_size[3];   // 0x14
} BwdBlockRecord;

#endif // BWDBLOCKRECORD_H
