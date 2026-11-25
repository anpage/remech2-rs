#ifndef REMEMBEREDMECH_H
#define REMEMBEREDMECH_H

#include "types.h"

#pragma pack(push, 1)

// The arguments a player's mech was loaded with (RememberLoadMech), to load it again.
typedef struct RememberedMech {
	int32_t m_id;     // 0x00
	char m_name[9];   // 0x04
	char m_config[9]; // 0x0d
} RememberedMech;

#pragma pack(pop)

#endif // REMEMBEREDMECH_H
