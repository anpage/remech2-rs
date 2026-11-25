#ifndef BWDRECORD_H
#define BWDRECORD_H

#include "types.h"

// A BWD record's header. The size counts the whole record, header included.
typedef struct BwdRecord {
	uint32_t m_tag;  // 0x00
	uint32_t m_size; // 0x04
} BwdRecord;

#endif // BWDRECORD_H
