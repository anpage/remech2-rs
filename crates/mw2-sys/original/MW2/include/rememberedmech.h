#ifndef REMEMBEREDMECH_H
#define REMEMBEREDMECH_H

#include "decomp.h"
#include "types.h"

#pragma pack(push, 1)

// The arguments a player's mech was loaded with (RememberLoadMech), to load it again.
// SIZE 0x16
typedef struct RememberedMech {
	MechS32 m_id;         // 0x00
	MechChar m_name[9];   // 0x04
	MechChar m_config[9]; // 0x0d
} RememberedMech;

#pragma pack(pop)

#endif // REMEMBEREDMECH_H
