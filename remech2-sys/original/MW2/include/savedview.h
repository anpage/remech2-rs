#ifndef SAVEDVIEW_H
#define SAVEDVIEW_H

#include "types.h"

// An eyepoint's position and orientation, saved (SaveView writes one as seven words, the last
// marking it set, and RestoreView restores it). FirstEyepoint clears five of them
// (g_savedViews), which nothing else uses.
typedef struct SavedView {
	int32_t m_x;
	int32_t m_y;
	int32_t m_z;
	int32_t m_heading;
	int32_t m_pitch;
	int32_t m_roll;
	int32_t m_set;
} SavedView;

#endif // SAVEDVIEW_H
