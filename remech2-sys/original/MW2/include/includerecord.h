#ifndef INCLUDERECORD_H
#define INCLUDERECORD_H

#include "bwdrecord.h"
#include "types.h"

// An include record: the stream to run, by id or name.
typedef struct IncludeRecord {
	BwdRecord m_header;
	int16_t m_id;
	char m_name[0x0c];
} IncludeRecord;

#endif // INCLUDERECORD_H
