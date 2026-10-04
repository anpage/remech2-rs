#ifndef FMVSLOT_H
#define FMVSLOT_H

#include "decomp.h"
#include "smackw32.h"
#include "types.h"

// SIZE 0x4c in the original, whose offsets the fields are marked with: it had four more fields
// at 0x04-0x10 for a VideoSound stream that no slot ever used.
// One FMV playback slot; the shell keeps 32 of them in g_fmvSlots. Flags at 0x1c: 0x80 places the
// video by its bottom center, 0x1000 disables the menu for this video, 0x80000000 marks the slot in use.
struct FmvSlot {
	Smack* m_smack;            // 0x00 — Smacker handle
	void* m_shp;               // 0x14 — an SHP animation's data, instead of m_smack
	void* m_frameBuffer;       // 0x18 — the Smacker frame, when not decoded in place
	MechS32 m_flags;           // 0x1c — flags, see above
	MechS32 m_left;            // 0x20
	MechS32 m_top;             // 0x24
	MechS32 m_width;           // 0x28
	MechS32 m_height;          // 0x2c
	MechS32 m_drawnLeft;       // 0x30 — m_left when last drawn
	MechS32 m_drawnTop;        // 0x34 — m_top when last drawn
	MechS32 m_drawnFrame;      // 0x38 — m_frame when last drawn
	MechS32 m_frame;           // 0x3c — current frame
	MechS32 m_frameCount;      // 0x40 — frame count
	MechU32 m_frameInterval;   // 0x44 — an SHP animation's milliseconds per frame
	MechU32 m_nextFrameTime;   // 0x48 — and the time of its next frame
};

#endif // FMVSLOT_H
