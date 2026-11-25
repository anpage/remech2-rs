#ifndef PALETTECOLOR_H
#define PALETTECOLOR_H

#include "types.h"

#pragma pack(1)
// One palette entry, 6 bits per component (the display back ends scale them by 4 for Windows).
struct PaletteColor {
	uint8_t m_red;
	uint8_t m_green;
	uint8_t m_blue;
};
typedef struct PaletteColor PaletteColor;
#pragma pack()

#endif // PALETTECOLOR_H
