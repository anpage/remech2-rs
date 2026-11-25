#ifndef PALCYCLE_H
#define PALCYCLE_H

#include "types.h"

// A palette colour cycle (palette.c's g_paletteCycle): the palette it started from, a working
// copy, and the range of entries RotatePaletteCycle rotates.
#pragma pack(push, 1)
typedef struct PaletteCycle {
	uint8_t* m_original;
	uint8_t* m_working;
	uint8_t m_first;
	int32_t m_count;
} PaletteCycle;
#pragma pack(pop)

#endif // PALCYCLE_H
