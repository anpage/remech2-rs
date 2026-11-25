#ifndef RECT_H
#define RECT_H

#include "types.h"

// A rectangle without a pixel buffer: PANE's last four fields.
typedef struct Rect {
	int32_t m_left;
	int32_t m_top;
	int32_t m_right;
	int32_t m_bottom;
} Rect;

#endif // RECT_H
