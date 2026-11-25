#ifndef RECT_H
#define RECT_H

#include "types.h"

// A rectangle without a pixel buffer: PANE's last four fields.
typedef struct Rect {
	int32_t m_left;   // 0x00
	int32_t m_top;    // 0x04
	int32_t m_right;  // 0x08
	int32_t m_bottom; // 0x0c
} Rect;

#endif // RECT_H
