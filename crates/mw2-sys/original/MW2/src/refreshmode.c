#include "refreshmode.h"

#include "brightness.h"
#include "decomp.h"
#include "display.h"
#include "displaybackend.h"
#include "palettecolor.h"
#include "types.h"
#include "window.h"

// The refresh mode manager, the simulator's copy of the shell's. The original chose between
// DirectDraw, DisplayDib and GDI back ends and six ways of getting the frame to the screen,
// profiled them, and sized and restyled the window to suit. One back end and one refresh mode are
// left, over util/display.h: the Rust side owns the frame, the window and how the one reaches the
// other.

static MechS32 DisplayBegin(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height);
static MechS32 DisplayEnd(void);
static MechS32 DisplaySetPalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors);
static MechS32 DisplaySetPaletteWithBrightness(PaletteColor* p_palette);
static MechS32 DisplayBlendPalettes(PaletteColor* p_palette, MechS32 p_steps);
static MechS32 DisplayAcquireFramebuffer(void);
static MechS32 DisplayFlip(void);
static MechS32 DisplayBlitRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
static MechS32 DisplayStretchBlit(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);

// Takes the place of the GDI back end.
static DisplayBackend g_displayBackend = {
	c_displayBackendGdi,
	c_windowModeWindowed,
	0,
	DisplayBegin,
	DisplayEnd,
	DisplaySetPalette,
	DisplaySetPaletteWithBrightness,
	DisplayBlendPalettes,
	DisplayAcquireFramebuffer,
	0
};

// Takes the place of the GDI refresh mode, which was mode 5.
static RefreshMode g_refreshMode =
	{5, c_displayBackendGdi, 1, 0, DisplayBegin, DisplayEnd, DisplayFlip, DisplayBlitRect, DisplayStretchBlit};

// GLOBAL: MW2 0x100b1770
DisplayBackend* g_currentDisplayBackend = NULL;

// GLOBAL: MW2 0x100b1774
RefreshMode* g_currentRefreshMode = NULL;

// The buffer the active refresh mode renders into.
// GLOBAL: MW2 0x100b1784
WINDOW* g_refreshModeBuffer = NULL;

// GLOBAL: MW2 0x100b1788
PaletteColor g_paletteColors[0x100] = {0};

// The frame, restored into g_refreshModeBuffer by m_acquireFramebuffer. Originally g_dibBits, the
// DIB bits of the GDI and DisplayDib back ends.
// GLOBAL: MW2 0x100b1a88
static MechU8* g_frame = NULL;

// Always windowed: full screen is the Rust side's business. The original set it from the back
// end, or to full screen when the frame covered the desktop.
// GLOBAL: MW2 0x100b1aa4
MechS32 g_windowMode = c_windowModeWindowed;

// The frame's size in pixels, p_width * p_height of InitRefreshMode.
// GLOBAL: MW2 0x100c2cc8
MechS32 g_refreshModePixelCount;

// GLOBAL: MW2 0x100c2ce4
MechS32 g_refreshModeWidth;

// GLOBAL: MW2 0x100c2ce8
MechS32 g_refreshModeHeight;

// Starts a frame of p_width x p_height in p_buffer, in place of one of another size. Returns 0
// if it can't be allocated.
// The original switched to refresh mode p_mode, falling through the later modes with
// p_allowFallback while a mode was unavailable, and sized the window to the frame, with room for
// a menu bar with p_menu.
// FUNCTION: MW2 0x10076d50
MechS32 InitRefreshMode(
	MechS32 p_mode,
	MechS32 p_allowFallback,
	WINDOW* p_buffer,
	MechS32 p_width,
	MechS32 p_height,
	MechS32 p_menu
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

// FUNCTION: MW2 0x10077069
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
// FUNCTION: MW2 0x100777a5
MechS32 GetPaletteColors(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette)
{
	MechS32 i;

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
static MechS32 DisplayBegin(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height)
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
	p_buffer->m_bitmapInfo = NULL;
	MechDisplaySetPalette((MechU8*) g_paletteColors);
	MechDisplayPresent();
	return 0;
}

static MechS32 DisplayEnd(void)
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
static MechS32 DisplaySetPalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors)
{
	MechS32 i;

	if (g_frame == NULL) {
		return -1;
	}

	if (p_palette == NULL || p_first < 0 || p_first > 0xff || p_count <= 0 || p_count > 0x100 - p_first) {
		return -1;
	}

	for (i = 0; i < p_count; i++) {
		g_paletteColors[p_first + i] = p_palette[i];
	}

	MechDisplaySetPalette((MechU8*) g_paletteColors);
	return 0;
}

// Keeps p_palette in g_paletteColorsPreBrightness and shows the frame with its
// brightness-adjusted copy.
static MechS32 DisplaySetPaletteWithBrightness(PaletteColor* p_palette)
{
	MechS32 i;

	for (i = 0; i < 0x100; i++) {
		g_paletteColorsPreBrightness[i] = p_palette[i];
		CopyPaletteColorWithBrightness(&p_palette[i], &g_paletteColors[i]);
	}

	MechDisplaySetPalette((MechU8*) g_paletteColors);
	MechDisplayPresent();
	g_refreshModeBuffer->m_buffer = g_frame;
	return 0;
}

// Fades linearly from the current palette to p_palette over p_steps / 2 frames, writing each
// in-between palette to p_palette (which ends back at the target).
static MechS32 DisplayBlendPalettes(PaletteColor* p_palette, MechS32 p_steps)
{
	MechS32 i;
	MechS32 j;
	MechDouble deltas[0x100][3];

	if (p_palette == NULL) {
		return -1;
	}

	p_steps >>= 1;
	for (i = 0; i < 0x100; i++) {
		g_paletteColorsPreBrightness[i] = g_paletteColors[i];
		deltas[i][0] = (MechDouble) (p_palette[i].m_red - g_paletteColorsPreBrightness[i].m_red) / p_steps;
		deltas[i][1] = (MechDouble) (p_palette[i].m_green - g_paletteColorsPreBrightness[i].m_green) / p_steps;
		deltas[i][2] = (MechDouble) (p_palette[i].m_blue - g_paletteColorsPreBrightness[i].m_blue) / p_steps;
	}

	i = p_steps;
	while (i--) {
		for (j = 0; j < 0x100; j++) {
			p_palette[j].m_red = (MechU8) ((p_steps - i) * deltas[j][0]) + g_paletteColorsPreBrightness[j].m_red;
			p_palette[j].m_green = (MechU8) ((p_steps - i) * deltas[j][1]) + g_paletteColorsPreBrightness[j].m_green;
			p_palette[j].m_blue = (MechU8) ((p_steps - i) * deltas[j][2]) + g_paletteColorsPreBrightness[j].m_blue;
			g_paletteColors[j] = p_palette[j];
		}

		MechDisplaySetPalette((MechU8*) g_paletteColors);
		MechDisplayPresent();
	}

	g_refreshModeBuffer->m_buffer = g_frame;
	return 0;
}

static MechS32 DisplayAcquireFramebuffer(void)
{
	g_refreshModeBuffer->m_buffer = g_frame;
	return 0;
}

static MechS32 DisplayFlip(void)
{
	MechDisplayPresent();
	return 0;
}

// The original copied only the inclusive rectangle (p_left, p_top)-(p_right, p_bottom) to the
// window. The whole frame is redrawn every time now.
static MechS32 DisplayBlitRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	MechDisplayPresent();
	return 0;
}

// Stretches the inclusive rectangle (p_left, p_top)-(p_right, p_bottom) over the whole window.
static MechS32 DisplayStretchBlit(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	MechDisplayPresentRect(p_left, p_top, p_right, p_bottom);
	return 0;
}
