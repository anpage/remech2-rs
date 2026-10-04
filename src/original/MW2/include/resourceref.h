#ifndef RESOURCEREF_H
#define RESOURCEREF_H

#include "types.h"

// A reference to a resource: its id, or -1 to look it up by name (LoadResourceByRef).
// SIZE 0x10
typedef struct ResourceRef {
	MechS16 m_id;        // 0x00
	MechChar m_name[14]; // 0x02
} ResourceRef;

#endif // RESOURCEREF_H
