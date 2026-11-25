#ifndef INCLUDERECORD_H
#define INCLUDERECORD_H

#include "bwdrecord.h"
#include "types.h"

// An include record: the stream to run, by id or name.
typedef struct IncludeRecord {
	BwdRecord m_header; // 0x00
	int16_t m_id;       // 0x08
	char m_name[0x0c];  // 0x0a
} IncludeRecord;

#endif // INCLUDERECORD_H
