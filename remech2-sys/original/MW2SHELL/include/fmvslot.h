#ifndef FMVSLOT_H
#define FMVSLOT_H

#include "smackw32.h"
#include "types.h"

// The fields are marked with the original's offsets: it had four more fields at 0x04-0x10 for a
// VideoSound stream that no slot ever used.
// One FMV playback slot; the shell keeps 32 of them in g_fmvSlots. Flags at 0x1c: 0x80 places the
// video by its bottom center, 0x1000 disables the menu for this video, 0x80000000 marks the slot in use.
struct FmvSlot {
	Smack* m_smack;           // 0x00 — Smacker handle
	void* m_shp;              // 0x14 — an SHP animation's data, instead of m_smack
	void* m_frameBuffer;      // 0x18 — the Smacker frame, when not decoded in place
	int32_t m_flags;          // 0x1c — flags, see above
	int32_t m_left;           // 0x20
	int32_t m_top;            // 0x24
	int32_t m_width;          // 0x28
	int32_t m_height;         // 0x2c
	int32_t m_drawnLeft;      // 0x30 — m_left when last drawn
	int32_t m_drawnTop;       // 0x34 — m_top when last drawn
	int32_t m_drawnFrame;     // 0x38 — m_frame when last drawn
	int32_t m_frame;          // 0x3c — current frame
	int32_t m_frameCount;     // 0x40 — frame count
	uint32_t m_frameInterval; // 0x44 — an SHP animation's milliseconds per frame
	uint32_t m_nextFrameTime; // 0x48 — and the time of its next frame
};

#endif // FMVSLOT_H
