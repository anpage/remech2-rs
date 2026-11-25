#ifndef PANE_H
#define PANE_H

#include "decomp.h"
#include "types.h"

typedef struct WINDOW WINDOW;

#pragma pack(1)
// VFX.H's PANE: a rectangle of a WINDOW, inclusive, which Miles Design VFX's drawing routines
// take. Coordinates are relative to the WINDOW; VFX clips the pane to it.
struct PANE {
	WINDOW* m_window; // 0x00
	int32_t m_x0;     // 0x04
	int32_t m_y0;     // 0x08
	int32_t m_x1;     // 0x0c
	int32_t m_y1;     // 0x10
};
typedef struct PANE PANE;
#pragma pack()

#endif // PANE_H
