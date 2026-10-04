#ifndef DRAWBITMAPINFO_H
#define DRAWBITMAPINFO_H

#include "decomp.h"
#include "types.h"

#include <windows.h>

// The 8-bit DIB format shared by the GDI and DisplayDib back ends: a BITMAPINFO with a full
// 256-entry color table. GDI fills the table with palette indices (DIB_PAL_COLORS).
// SIZE 0x428
struct DrawBitmapInfo {
	BITMAPINFOHEADER m_header; // 0x00
	RGBQUAD m_colors[0x100];   // 0x28
};
typedef struct DrawBitmapInfo DrawBitmapInfo;

#endif // DRAWBITMAPINFO_H
