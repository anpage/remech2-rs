#ifndef SAVEDVIEW_H
#define SAVEDVIEW_H

#include "types.h"

// An eyepoint's position and orientation, saved (SaveView writes one as seven words, the last
// marking it set, and RestoreView restores it). FirstEyepoint clears five of them
// (g_savedViews), which nothing else uses.
typedef struct SavedView {
	int32_t m_x;       // 0x00
	int32_t m_y;       // 0x04
	int32_t m_z;       // 0x08
	int32_t m_heading; // 0x0c
	int32_t m_pitch;   // 0x10
	int32_t m_roll;    // 0x14
	int32_t m_set;     // 0x18
} SavedView;

#endif // SAVEDVIEW_H
