#ifndef DISPLAYBACKEND_H
#define DISPLAYBACKEND_H

#include "palettecolor.h"
#include "types.h"
#include "window.h"

// DisplayBackend::m_id. Only the GDI back end's place is left, taken by the one in refreshmode.c.
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
struct DisplayBackend {
	int32_t m_id;         // DisplayBackendId
	int32_t m_windowMode; // WindowMode
	uint32_t m_style;     // the shell window's style
	int32_t (*m_begin)(WINDOW* p_buffer, int32_t p_width, int32_t p_height);
	int32_t (*m_end)(void);
	int32_t (*m_setPalette)(int32_t p_first, int32_t p_count, PaletteColor* p_palette, int32_t p_allColors);
	int32_t (*m_setPaletteWithBrightness)(PaletteColor* p_palette);
	int32_t (*m_blendPalettes)(PaletteColor* p_palette, int32_t p_steps);
	// Returns 0 once the framebuffer can be drawn to (GDI: points the output buffer at the DIB bits).
	int32_t (*m_acquireFramebuffer)(void);
};
typedef struct DisplayBackend DisplayBackend;
#pragma pack()

#endif // DISPLAYBACKEND_H
