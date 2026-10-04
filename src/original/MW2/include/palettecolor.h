#ifndef PALETTECOLOR_H
#define PALETTECOLOR_H

#include "decomp.h"
#include "types.h"

#pragma pack(1)
// One palette entry, 6 bits per component (the display back ends scale them by 4 for Windows).
// SIZE 0x03
struct PaletteColor {
	MechU8 m_red;   // 0x00
	MechU8 m_green; // 0x01
	MechU8 m_blue;  // 0x02
};
typedef struct PaletteColor PaletteColor;
#pragma pack()

#endif // PALETTECOLOR_H
