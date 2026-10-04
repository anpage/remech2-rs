#ifndef SAVEDVIEW_H
#define SAVEDVIEW_H

#include "types.h"

// An eyepoint's position and orientation, saved (SaveView writes one as seven words, the last
// marking it set, and RestoreView restores it). FirstEyepoint clears five of them
// (g_savedViews), which nothing else uses.
// SIZE 0x1c
typedef struct SavedView {
	MechS32 m_x;       // 0x00
	MechS32 m_y;       // 0x04
	MechS32 m_z;       // 0x08
	MechS32 m_heading; // 0x0c
	MechS32 m_pitch;   // 0x10
	MechS32 m_roll;    // 0x14
	MechS32 m_set;     // 0x18
} SavedView;

#endif // SAVEDVIEW_H
