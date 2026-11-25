#ifndef PALETTECOLOR_H
#define PALETTECOLOR_H

#include "decomp.h"
#include "types.h"

#pragma pack(1)
// One palette entry, 6 bits per component (the display back ends scale them by 4 for Windows).
struct PaletteColor {
	uint8_t m_red;   // 0x00
	uint8_t m_green; // 0x01
	uint8_t m_blue;  // 0x02
};
typedef struct PaletteColor PaletteColor;
#pragma pack()

#endif // PALETTECOLOR_H
