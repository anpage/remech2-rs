#ifndef BWDNAME_H
#define BWDNAME_H

#include "types.h"

// A BWD resource's name: its number, or -1 and a name of up to 12 characters.
typedef struct BwdName {
	int16_t m_id;    // 0x00
	char m_name[14]; // 0x02
} BwdName;

#endif // BWDNAME_H
