#include "refreshmode.h"

#include "decomp.h"
#include "display.h"
#include "displaybackend.h"
#include "palettecolor.h"
#include "types.h"
#include "window.h"

// The refresh mode manager. The original chose between DirectDraw, DisplayDib and GDI back ends
// and six ways of getting the frame to the screen, profiled them, and sized and restyled the
// window to suit. One back end and one refresh mode are left, over util/display.h: the Rust side
// owns the frame, the window and how the one reaches the other.

static int32_t DisplayBegin(WINDOW* p_buffer, int32_t p_width, int32_t p_height);
static int32_t DisplayEnd(void);
static int32_t DisplaySetPalette(int32_t p_first, int32_t p_count, PaletteColor* p_palette, int32_t p_allColors);
static int32_t DisplayAcquireFramebuffer(void);
static int32_t DisplayFlip(void);
static int32_t DisplayBlitRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);
static int32_t DisplayStretchBlit(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);

// Takes the place of the GDI back end. Nothing in the shell sets the brightness or blends
// palettes.
DisplayBackend g_displayBackend = {
	c_displayBackendGdi,
	c_windowModeWindowed,
	0,
	DisplayBegin,
	DisplayEnd,
	DisplaySetPalette,
	NULL,
	NULL,
	DisplayAcquireFramebuffer
};

// Takes the place of the GDI refresh mode, which was mode 5.
RefreshMode g_refreshMode =
	{5, c_displayBackendGdi, 1, 0, DisplayBegin, DisplayEnd, DisplayFlip, DisplayBlitRect, DisplayStretchBlit};

DisplayBackend* g_currentDisplayBackend = NULL;

RefreshMode* g_currentRefreshMode = NULL;

// The buffer the active refresh mode renders into.
WINDOW* g_refreshModeBuffer = NULL;

PaletteColor g_paletteColors[0x100] = {0};

// The frame, restored into g_refreshModeBuffer by m_acquireFramebuffer. Originally g_dibBits, the
// DIB bits of the GDI and DisplayDib back ends.
uint8_t* g_frame = NULL;

// Always windowed: full screen is the Rust side's business. The original set it from the back
// end, or to full screen when the frame covered the desktop.
int32_t g_windowMode = c_windowModeWindowed;

// The frame's size in pixels, p_width * p_height of InitRefreshMode.
int32_t g_refreshModePixelCount;

int32_t g_refreshModeWidth;

int32_t g_refreshModeHeight;

// Starts a frame of p_width x p_height in p_buffer, in place of one of another size. Returns 0
// if it can't be allocated.
// The original switched to refresh mode p_mode, falling through the later modes with
// p_allowFallback while a mode was unavailable, and sized the window to the frame, with room for
// a menu bar with p_menu.
int32_t InitRefreshMode(
	int32_t p_mode,
	int32_t p_allowFallback,
	WINDOW* p_buffer,
	int32_t p_width,
	int32_t p_height,
	int32_t p_menu
)
{
	if (g_currentRefreshMode != NULL && (g_refreshModeWidth != p_width || g_refreshModeHeight != p_height)) {
		g_currentRefreshMode->m_end();
	}

	g_currentDisplayBackend = &g_displayBackend;
	g_currentRefreshMode = &g_refreshMode;
	g_refreshModeWidth = p_width;
	g_refreshModeHeight = p_height;
	g_refreshModePixelCount = p_height * p_width;
	g_refreshModeBuffer = p_buffer;

	return g_currentRefreshMode->m_begin(p_buffer, p_width, p_height) == 0;
}

void ShutdownRefreshMode(void)
{
	if (g_currentRefreshMode != NULL) {
		g_currentRefreshMode->m_end();
	}
	if (g_currentDisplayBackend != NULL) {
		g_currentDisplayBackend->m_end();
	}
}

// Copies p_count colors of g_paletteColors from p_first into p_palette.
int32_t GetPaletteColors(int32_t p_first, int32_t p_count, PaletteColor* p_palette)
{
	int32_t i;

	if (p_palette == NULL || p_first < 0 || p_first > 0xff || p_count <= 0 || p_count > 0x100 - p_first) {
		return -1;
	}

	for (i = 0; i < p_count; i++) {
		p_palette[i] = g_paletteColors[p_first + i];
	}

	return 0;
}

// Allocates the frame p_buffer describes and shows it, black. Returns 0 on success, also when
// there already is a frame, and 2 if the allocation fails.
static int32_t DisplayBegin(WINDOW* p_buffer, int32_t p_width, int32_t p_height)
{
	if (g_frame != NULL) {
		return 0;
	}

	g_frame = MechDisplayBegin(p_width, p_height);
	if (g_frame == NULL) {
		return 2;
	}

	p_buffer->m_buffer = g_frame;
	p_buffer->m_xMax = p_width - 1;
	p_buffer->m_yMax = p_height - 1;
	p_buffer->m_shadow = 0;
	p_buffer->m_stencil = NULL;
	MechDisplaySetPalette((uint8_t*) g_paletteColors);
	MechDisplayPresent();
	return 0;
}

static int32_t DisplayEnd(void)
{
	MechDisplayEnd();
	g_frame = NULL;
	if (g_refreshModeBuffer != NULL) {
		g_refreshModeBuffer->m_buffer = NULL;
	}

	return 0;
}

// Stores p_count colors from p_first in g_paletteColors. p_allColors said whether to take over
// the system's static colors on an 8-bit desktop.
static int32_t DisplaySetPalette(int32_t p_first, int32_t p_count, PaletteColor* p_palette, int32_t p_allColors)
{
	int32_t i;

	if (g_frame == NULL) {
		return -1;
	}

	if (p_palette == NULL || p_first < 0 || p_first > 0xff || p_count <= 0 || p_count > 0x100 - p_first) {
		return -1;
	}

	for (i = 0; i < p_count; i++) {
		g_paletteColors[p_first + i] = p_palette[i];
	}

	MechDisplaySetPalette((uint8_t*) g_paletteColors);
	return 0;
}

static int32_t DisplayAcquireFramebuffer(void)
{
	g_refreshModeBuffer->m_buffer = g_frame;
	return 0;
}

static int32_t DisplayFlip(void)
{
	MechDisplayPresent();
	return 0;
}

// The original copied only the inclusive rectangle (p_left, p_top)-(p_right, p_bottom) to the
// window. The whole frame is redrawn every time now.
static int32_t DisplayBlitRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	MechDisplayPresent();
	return 0;
}

// Stretches the inclusive rectangle (p_left, p_top)-(p_right, p_bottom) over the whole window.
static int32_t DisplayStretchBlit(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	MechDisplayPresentRect(p_left, p_top, p_right, p_bottom);
	return 0;
}
