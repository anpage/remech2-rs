#ifndef RESOURCEREF_H
#define RESOURCEREF_H

#include "types.h"

// A reference to a resource: its id, or -1 to look it up by name (LoadResourceByRef).
typedef struct ResourceRef {
	int16_t m_id;
	char m_name[14];
} ResourceRef;

#endif // RESOURCEREF_H
