#ifndef DISPDIB_H
#define DISPDIB_H

// Stand-in for the Video for Windows DisplayDib header. DISPDIB.DLL never
// existed on NT, so window creation always fails and DispDibBegin reports an
// error.
#include <windows.h>

#define DISPLAYDIB_NOWAIT 0x0040
#define DISPLAYDIB_DONTLOCKTASK 0x0200

#define DisplayDibWindowCreate(p_parent, p_instance) ((HWND)NULL)
#define DisplayDibWindowBegin(p_hwnd) ((UINT)1)
#define DisplayDibWindowEnd(p_hwnd) ((UINT)1)
#define DisplayDibWindowClose(p_hwnd) ((void)0)
#define DisplayDibWindowDraw(p_hwnd, p_flags, p_bits, p_size) ((UINT)1)
#define DisplayDibWindowSetFmt(p_hwnd, p_info) ((UINT)1)

#endif // DISPDIB_H
