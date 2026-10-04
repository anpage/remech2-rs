#ifndef BWDRECORD_H
#define BWDRECORD_H

#include "types.h"

// A BWD record's header. The size counts the whole record, header included.
typedef struct BwdRecord {
	MechU32 m_tag;  // 0x00
	MechU32 m_size; // 0x04
} BwdRecord;

#endif // BWDRECORD_H
