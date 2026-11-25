#ifndef BWDRECORD_H
#define BWDRECORD_H

#include "types.h"

// A BWD record's header. The size counts the whole record, header included.
typedef struct BwdRecord {
	uint32_t m_tag;
	uint32_t m_size;
} BwdRecord;

#endif // BWDRECORD_H
