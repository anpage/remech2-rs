#ifndef REFRESHMODE_H
#define REFRESHMODE_H

#include "decomp.h"
#include "displaybackend.h"
#include "drawbitmapinfo.h"
#include "palettecolor.h"
#include "types.h"
#include "window.h"

#include <windows.h>

#pragma pack(1)
// A refresh mode (the original's debug strings name them): how the framebuffer reaches the
// screen through one of the back ends in g_displayBackends. Several modes share a back end.
// SIZE 0x24
struct RefreshMode {
	MechS32 m_index;                                                         // 0x00 — in g_refreshModes
	MechS32 m_backend;                                                       // 0x04 — in g_displayBackends
	MechS32 m_available;                                                     // 0x08 — cleared when m_begin fails
	MechU32 m_profileTime;                                                   // 0x0c
	MechS32 (*m_begin)(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height); // 0x10
	MechS32 (*m_end)();                                                      // 0x14
	MechS32 (*m_flip)();                                                     // 0x18
	MechS32 (*m_blitRect)(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);    // 0x1c
	MechS32 (*m_stretchBlit)(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom); // 0x20
};
typedef struct RefreshMode RefreshMode;
#pragma pack()

// The functions and globals of refreshmode.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern DisplayBackend* g_displayBackends[3];
	extern RefreshMode* g_refreshModes[6];
	extern DisplayBackend* g_currentDisplayBackend;
	extern RefreshMode* g_currentRefreshMode;
	extern RefreshMode* g_fastestRefreshMode;
	extern MechS32 g_refreshModeFallback;
	extern WINDOW* g_refreshModeBuffer;
	extern PaletteColor g_paletteColors[0x100];
	extern undefined* g_dibBits;
	extern MechS32 g_windowMode;
	extern RECT g_windowedRect;
	extern DrawBitmapInfo g_bitmapInfo;
	extern MechS32 g_refreshModePixelCount;
	extern MechS32 g_refreshModeWidth;
	extern MechS32 g_refreshModeHeight;

	MechS32 InitRefreshMode(
		MechS32 p_mode,
		MechS32 p_allowFallback,
		WINDOW* p_buffer,
		MechS32 p_width,
		MechS32 p_height,
		MechS32 p_menu
	);
	void ShutdownRefreshMode(void);
	void ProfileRefreshModes(void);
	void SelectFastestRefreshMode(void);
	void ToggleFullScreen(void);
	void InitBitmapInfo(MechS32 p_width, MechS32 p_height);
	void AdjustWindowSize(DisplayBackend* p_backend);
	MechS32 GetPaletteColors(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette);

#ifdef __cplusplus
}
#endif

#endif // REFRESHMODE_H
