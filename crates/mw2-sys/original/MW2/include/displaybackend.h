#ifndef DISPLAYBACKEND_H
#define DISPLAYBACKEND_H

#include "decomp.h"
#include "palettecolor.h"
#include "types.h"
#include "window.h"

// DisplayBackend::m_id, the index in g_displayBackends.
enum DisplayBackendId {
	c_displayBackendDirectDraw = 0,
	c_displayBackendDisplayDib = 1,
	c_displayBackendGdi = 2
};

// DisplayBackend::m_windowMode and g_windowMode.
enum WindowMode {
	c_windowModeFullscreen = 1,
	c_windowModeWindowed = 2
};

#pragma pack(1)
// Function table of the active display back end (DirectDraw, DisplayDib or GDI).
// SIZE 0x28
struct DisplayBackend {
	MechS32 m_id;                                                            // 0x00 — DisplayBackendId
	MechS32 m_windowMode;                                                    // 0x04 — WindowMode
	MechU32 m_style;                                                         // 0x08 — the shell window's style
	MechS32 (*m_begin)(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height); // 0x0c
	MechS32 (*m_end)();                                                      // 0x10
	MechS32 (*m_setPalette)(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors); // 0x14
	MechS32 (*m_setPaletteWithBrightness)(PaletteColor* p_palette);                                          // 0x18
	MechS32 (*m_blendPalettes)(PaletteColor* p_palette, MechS32 p_steps);                                    // 0x1c
	// Returns 0 once the framebuffer can be drawn to (GDI: points the output buffer at the DIB bits).
	MechS32 (*m_acquireFramebuffer)(); // 0x20
	undefined4 m_unk0x24;              // 0x24 — 0 in every back end, never read
};
typedef struct DisplayBackend DisplayBackend;
#pragma pack()

#endif // DISPLAYBACKEND_H
