#ifndef PALCYCLE_H
#define PALCYCLE_H

#include "decomp.h"
#include "types.h"

// A palette colour cycle (palette.c's g_paletteCycle): the palette it started from, a working
// copy, and the range of entries RotatePaletteCycle rotates.
#pragma pack(push, 1)
// SIZE 0x0d
typedef struct PaletteCycle {
	MechU8* m_original; // 0x00
	MechU8* m_working;  // 0x04
	MechU8 m_first;     // 0x08
	MechS32 m_count;    // 0x09
} PaletteCycle;
#pragma pack(pop)

#endif // PALCYCLE_H
