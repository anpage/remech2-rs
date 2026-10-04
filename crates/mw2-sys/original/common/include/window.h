#ifndef WINDOW_H
#define WINDOW_H

#include "decomp.h"
#include "types.h"

#pragma pack(1)
// VFX.H's WINDOW: a buffer of pixels, m_xMax + 1 bytes to a row and m_yMax + 1 rows, that PANEs
// are rectangles of. The game's display back ends kept their bitmap info where VFX has the
// window's stencil; now nothing uses it.
// SIZE 0x14
struct WINDOW {
	undefined* m_buffer;          // 0x00
	MechS32 m_xMax;               // 0x04
	MechS32 m_yMax;               // 0x08
	void* m_stencil;              // 0x0c, always NULL
	undefined4 m_shadow;          // 0x10, only cleared, by the back ends' begin functions
};
typedef struct WINDOW WINDOW;
#pragma pack()

#endif // WINDOW_H
