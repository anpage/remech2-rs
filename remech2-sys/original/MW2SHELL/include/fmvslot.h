#ifndef FMVSLOT_H
#define FMVSLOT_H

#include "smackw32.h"
#include "types.h"

// One FMV playback slot; the shell keeps 32 of them in g_fmvSlots. m_flags: 0x80 places the
// video by its bottom center, 0x1000 disables the menu for this video, 0x80000000 marks the slot in use.
struct FmvSlot {
	Smack* m_smack;      // Smacker handle
	void* m_shp;         // an SHP animation's data, instead of m_smack
	void* m_frameBuffer; // the Smacker frame, when not decoded in place
	int32_t m_flags;     // flags, see above
	int32_t m_left;
	int32_t m_top;
	int32_t m_width;
	int32_t m_height;
	int32_t m_drawnLeft;      // m_left when last drawn
	int32_t m_drawnTop;       // m_top when last drawn
	int32_t m_drawnFrame;     // m_frame when last drawn
	int32_t m_frame;          // current frame
	int32_t m_frameCount;     // frame count
	uint32_t m_frameInterval; // an SHP animation's milliseconds per frame
	uint32_t m_nextFrameTime; // and the time of its next frame
};

#endif // FMVSLOT_H
