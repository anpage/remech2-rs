#ifndef RESOURCEREF_H
#define RESOURCEREF_H

#include "types.h"

// A reference to a resource: its id, or -1 to look it up by name (LoadResourceByRef).
typedef struct ResourceRef {
	int16_t m_id;    // 0x00
	char m_name[14]; // 0x02
} ResourceRef;

#endif // RESOURCEREF_H
