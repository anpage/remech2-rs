#ifndef PALCYCLE_H
#define PALCYCLE_H

#include "types.h"

// A palette colour cycle (palette.c's g_paletteCycle): the palette it started from, a working
// copy, and the range of entries RotatePaletteCycle rotates.
#pragma pack(push, 1)
typedef struct PaletteCycle {
	uint8_t* m_original; // 0x00
	uint8_t* m_working;  // 0x04
	uint8_t m_first;     // 0x08
	int32_t m_count;     // 0x09
} PaletteCycle;
#pragma pack(pop)

#endif // PALCYCLE_H
