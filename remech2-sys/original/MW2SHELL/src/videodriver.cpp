#include "videodriver.h"

#include "debugprint.h"
#include "displaybackend.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "textglyphlist.h"
#include "tmpackdatabase.h"
#include "vfxa.h"
#include "video.h"
#include "windowstate.h"

#include <stdlib.h>
#include <string.h>

// Only the height field of the font data header is used by the text drawing wrappers.
struct VideoFontHeader {
	uint8_t m_reserved0x00[8];
	int32_t m_height;
};

// Set while RedrawGlyphs runs: text outside the dirty rectangle is skipped.
int32_t g_redrawingGlyphs = 0;

// Set by LoadPalette: DrawShell loads the new palette between two flips of a black one.
int32_t g_clearPaletteOnDraw = 0;

// The black palette of ClearPalette, and LoadBackground's picture palette.
PaletteColor g_tempPalette[0x100];

// The text colors when a glyph has none: each color maps to itself, but 0 is transparent.
uint8_t g_defaultColorMap[0x100];

// 0 once the framebuffer can be drawn to.
#define ACQUIRE_FRAMEBUFFER() (g_currentDisplayBackend->m_acquireFramebuffer())

void QuitWithVDriverError(int32_t p_code)
{
	ShutdownRefreshMode();

	if (g_audioSubsystem) {
		delete g_audioSubsystem;
	}

	CloseAllVideos();
	DebugPrint("vdriver error\n");
	exit(p_code + 1);
}

void ClearPalette()
{
	memset(g_tempPalette, 0, sizeof(g_tempPalette));
	g_currentDisplayBackend->m_setPalette(0, 0x100, g_tempPalette, 1);
}

VideoDriver::VideoDriver()
{
	int32_t i;

	if (!InitRefreshMode(5, 0, &m_screenBuffer, g_windowWidth, g_windowHeight, 1)) {
		QuitWithVDriverError(1);
	}

	m_width = g_windowWidth;
	m_height = g_windowHeight;
	m_screenBuffer.m_xMax = m_backBuffer.m_xMax = m_width - 1;
	m_screenBuffer.m_yMax = m_backBuffer.m_yMax = m_height - 1;

	m_backBuffer.m_buffer = (uint8_t*) MechHeapAllocZeroed(g_primaryHeap, m_width * m_height);
	if (!m_backBuffer.m_buffer) {
		QuitWithVDriverError(1);
	}

	m_screenView.m_x0 = m_backView.m_x0 = 0;
	m_screenView.m_y0 = m_backView.m_y0 = 0;
	m_screenView.m_x1 = m_backView.m_x1 = m_screenBuffer.m_xMax;
	m_screenView.m_y1 = m_backView.m_y1 = m_screenBuffer.m_yMax;
	m_screenView.m_window = &m_screenBuffer;
	m_backView.m_window = &m_backBuffer;

	// Start with an empty (inverted) dirty rectangle.
	m_dirtyView.m_x0 = m_screenView.m_x1;
	m_dirtyView.m_y0 = m_screenView.m_y1;
	m_dirtyView.m_x1 = m_screenView.m_x0;
	m_dirtyView.m_y1 = m_screenView.m_y0;
	m_dirtyView.m_window = m_screenView.m_window;

	m_overlayGlyphs = new TextGlyphList();
	m_glyphs = new TextGlyphList();

	g_defaultColorMap[0] = 0xff;
	for (i = 1; i < 0x100; i++) {
		g_defaultColorMap[i] = i;
	}

	m_restoreColor = -1;

	// Reset the palette for when we return from the sim
	memset(m_palette, 0, sizeof(m_palette));
	m_paletteChanged = 0;
	m_fmvFirstFrame = 0;
}

VideoDriver::~VideoDriver()
{
	delete m_overlayGlyphs;
	delete m_glyphs;
	MechHeapFree(g_primaryHeap, m_backBuffer.m_buffer);
	ShutdownRefreshMode();
}

void VideoDriver::ExpandRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	if (p_left < m_dirtyView.m_x0) {
		m_dirtyView.m_x0 = p_left;
	}
	if (p_top < m_dirtyView.m_y0) {
		m_dirtyView.m_y0 = p_top;
	}
	if (p_right > m_dirtyView.m_x1) {
		m_dirtyView.m_x1 = p_right;
	}
	if (p_bottom > m_dirtyView.m_y1) {
		m_dirtyView.m_y1 = p_bottom;
	}
}

void VideoDriver::ExpandRectBySize(int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height)
{
	int32_t right = p_left + p_width - 1;
	int32_t bottom = p_top + p_height - 1;

	if (p_left < m_dirtyView.m_x0) {
		m_dirtyView.m_x0 = p_left;
	}
	if (p_top < m_dirtyView.m_y0) {
		m_dirtyView.m_y0 = p_top;
	}
	if (right > m_dirtyView.m_x1) {
		m_dirtyView.m_x1 = right;
	}
	if (bottom > m_dirtyView.m_y1) {
		m_dirtyView.m_y1 = bottom;
	}
}

int32_t VideoDriver::IntersectsRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	if (p_left > m_dirtyView.m_x1 || p_right < m_dirtyView.m_x0 || p_top > m_dirtyView.m_y1 ||
		p_bottom < m_dirtyView.m_y0) {
		return false;
	}
	else {
		return true;
	}
}

int32_t VideoDriver::IntersectsRectBySize(int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height)
{
	int32_t right = p_left + p_width - 1;
	int32_t bottom = p_top + p_height - 1;

	if (p_left > m_dirtyView.m_x1 || right < m_dirtyView.m_x0 || p_top > m_dirtyView.m_y1 ||
		bottom < m_dirtyView.m_y0) {
		return false;
	}
	else {
		return true;
	}
}

void VideoDriver::UpdatePalette()
{
	g_currentDisplayBackend->m_setPalette(0, 0x100, m_palette, m_allColors);
}

// Presents the frame, reloading the palette first if it changed, then empties the dirty
// rectangle.
void VideoDriver::DrawShell()
{
	if (m_screenView.m_x0 > m_dirtyView.m_x0) {
		m_dirtyView.m_x0 = m_screenView.m_x0;
	}
	if (m_dirtyView.m_y0 < m_screenView.m_y0) {
		m_dirtyView.m_y0 = m_screenView.m_y0;
	}
	if (m_dirtyView.m_x1 > m_screenView.m_x1) {
		m_dirtyView.m_x1 = m_screenView.m_x1;
	}
	if (m_dirtyView.m_y1 > m_screenView.m_y1) {
		m_dirtyView.m_y1 = m_screenView.m_y1;
	}

	if (m_paletteChanged) {
		if (m_allColors) {
			if (g_clearPaletteOnDraw) {
				ClearPalette();
				g_currentRefreshMode->m_flip();
				g_currentDisplayBackend->m_setPalette(0, 0x100, m_palette, m_allColors);
				g_currentRefreshMode->m_flip();
				g_clearPaletteOnDraw = 0;
			}
			else {
				ActivateFramebuffer();
				g_currentDisplayBackend->m_setPalette(0, 0x100, m_palette, m_allColors);
				memcpy(m_screenBuffer.m_buffer, m_backBuffer.m_buffer, m_width * m_height);
				g_currentRefreshMode->m_flip();
			}
		}
		else if (m_fmvFirstFrame) {
			memcpy(m_backBuffer.m_buffer, m_screenBuffer.m_buffer, m_width * m_height);
			ActivateFramebuffer();
			g_currentDisplayBackend->m_setPalette(0, 0x100, m_palette, m_allColors);
			memcpy(m_screenBuffer.m_buffer, m_backBuffer.m_buffer, m_width * m_height);
			g_currentRefreshMode->m_flip();
			m_fmvFirstFrame = 0;
		}
		else {
			g_currentDisplayBackend->m_setPalette(0, 0x100, m_palette, m_allColors);
			g_currentRefreshMode
				->m_blitRect(m_screenView.m_x0, m_screenView.m_y0, m_screenView.m_x1, m_screenView.m_y1);
		}

		m_paletteChanged = 0;
	}
	else {
		// The original only blitted when the dirty rectangle wasn't empty. The Rust side draws its
		// overlay with every present, so there is one every time.
		// With g_menuVisible the original called GdiBitBltRectWithMenu instead, which drew one menu
		// bar height higher.
		g_currentRefreshMode->m_blitRect(m_dirtyView.m_x0, m_dirtyView.m_y0, m_dirtyView.m_x1, m_dirtyView.m_y1);
	}

	m_dirtyView.m_x0 = m_screenView.m_x1;
	m_dirtyView.m_y0 = m_screenView.m_y1;
	m_dirtyView.m_x1 = m_screenView.m_x0;
	m_dirtyView.m_y1 = m_screenView.m_y0;
}

// DrawShell for movie frames: the dirty rectangle is stretched to the screen.
void VideoDriver::DrawFmv()
{
	if (m_paletteChanged) {
		if (m_fmvFirstFrame) {
			memcpy(m_backBuffer.m_buffer, m_screenBuffer.m_buffer, m_width * m_height);
			ActivateFramebuffer();
			g_currentDisplayBackend->m_setPalette(0, 0x100, m_palette, m_allColors);
			memcpy(m_screenBuffer.m_buffer, m_backBuffer.m_buffer, m_width * m_height);
			m_fmvFirstFrame = 0;
		}
		else {
			g_currentDisplayBackend->m_setPalette(0, 0x100, m_palette, m_allColors);
		}

		m_paletteChanged = 0;
	}

	if (m_dirtyView.m_x0 <= m_dirtyView.m_x1 && m_dirtyView.m_y0 <= m_dirtyView.m_y1) {
		// With its Little Movies option the original called GdiBlitCentered instead, which drew the
		// rectangle unscaled at (160, 140).
		g_currentRefreshMode->m_stretchBlit(m_dirtyView.m_x0, m_dirtyView.m_y0, m_dirtyView.m_x1, m_dirtyView.m_y1);
	}

	m_dirtyView.m_x0 = m_screenView.m_x1;
	m_dirtyView.m_y0 = m_screenView.m_y1;
	m_dirtyView.m_x1 = m_screenView.m_x0;
	m_dirtyView.m_y1 = m_screenView.m_y0;
}

void VideoDriver::GetPalette(PaletteColor* p_palette)
{
	memcpy(p_palette, m_palette, sizeof(m_palette));
}

void VideoDriver::SetPalette(PaletteColor* p_palette, uint32_t p_allColors)
{
	memcpy(m_palette, p_palette, sizeof(m_palette));
	m_paletteChanged = 1;
	m_allColors = p_allColors;
}

void VideoDriver::LoadBackground(TMPackDataBase* p_database, int32_t p_id)
{
	int32_t size;
	uint8_t* data;

	if (p_database->GetDBItemLZ(p_id, (void**) &data, &size)) {
		return;
	}

	VFX_PCX_palette(data, size, g_tempPalette);
	if (memcmp(g_tempPalette, m_palette, sizeof(m_palette))) {
		memcpy(m_palette, g_tempPalette, sizeof(m_palette));
		m_paletteChanged = 1;
		m_allColors = 1;
	}

	if (m_paletteChanged) {
		VFX_PCX_draw(&m_backView, data);
		DrawShell();
		ExpandRect(m_screenView.m_x0, m_screenView.m_y0, m_screenView.m_x1, m_screenView.m_y1);
	}
	else if (ACQUIRE_FRAMEBUFFER() == 0) {
		VFX_PCX_draw(&m_screenView, data);
		memcpy(m_backBuffer.m_buffer, m_screenBuffer.m_buffer, m_width * m_height);
		ExpandRect(m_screenView.m_x0, m_screenView.m_y0, m_screenView.m_x1, m_screenView.m_y1);
	}

	MechHeapFree(g_primaryHeap, data);
}

void VideoDriver::DrawLine(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom, int32_t p_color)
{
	if (ACQUIRE_FRAMEBUFFER() == 0) {
		VFX_line_draw(&m_screenView, p_left, p_top, p_right, p_bottom, 0, p_color);
	}

	ExpandRect(p_left, p_top, p_right, p_bottom);
}

// Draws a block of pixels (a video's frame) to the screen.
void VideoDriver::DrawPixels(uint8_t* p_pixels, int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height)
{
	PANE view;
	WINDOW buffer;

	view.m_window = &buffer;
	view.m_x0 = 0;
	view.m_y0 = 0;
	view.m_x1 = buffer.m_xMax = p_width - 1;
	view.m_y1 = buffer.m_yMax = p_height - 1;
	buffer.m_buffer = p_pixels;

	if (ACQUIRE_FRAMEBUFFER() == 0) {
		VFX_pane_copy(&view, 0, 0, &m_screenView, p_left, p_top, -1);
	}

	ExpandRectBySize(p_left, p_top, p_width, p_height);
}

// DrawPixels, skipped unless the block meets the dirty rectangle.
void VideoDriver::DrawPixelsClipped(uint8_t* p_pixels, int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height)
{
	PANE view;
	WINDOW buffer;

	if (!IntersectsRectBySize(p_left, p_top, p_width, p_height)) {
		return;
	}

	view.m_window = &buffer;
	view.m_x0 = 0;
	view.m_y0 = 0;
	view.m_x1 = buffer.m_xMax = p_width - 1;
	view.m_y1 = buffer.m_yMax = p_height - 1;
	buffer.m_buffer = p_pixels;

	if (ACQUIRE_FRAMEBUFFER() == 0) {
		VFX_pane_copy(&view, 0, 0, &m_screenView, p_left, p_top, -1);
	}

	ExpandRectBySize(p_left, p_top, p_width, p_height);
}

void VideoDriver::ReadPixels(uint8_t* p_pixels, int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height)
{
	PANE view;
	WINDOW buffer;

	view.m_window = &buffer;
	view.m_x0 = 0;
	view.m_y0 = 0;
	view.m_x1 = buffer.m_xMax = p_width - 1;
	view.m_y1 = buffer.m_yMax = p_height - 1;
	buffer.m_buffer = p_pixels;

	if (ACQUIRE_FRAMEBUFFER() == 0) {
		VFX_pane_copy(&m_screenView, p_left, p_top, &view, 0, 0, -1);
	}
}

void VideoDriver::RestoreBackground(int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height)
{
	PANE view;

	view.m_x0 = p_left;
	view.m_y0 = p_top;
	view.m_x1 = p_left + p_width - 1;
	view.m_y1 = p_top + p_height - 1;
	view.m_window = &m_backBuffer;

	if (ACQUIRE_FRAMEBUFFER() == 0) {
		VFX_pane_copy(&view, 0, 0, &m_screenView, p_left, p_top, m_restoreColor);
	}

	ExpandRectBySize(p_left, p_top, p_width, p_height);
}

void VideoDriver::CopyScreenToBackground()
{
	VFX_pane_copy(&m_screenView, 0, 0, &m_backView, 0, 0, -1);
}

void VideoDriver::CopyBackgroundToScreen()
{
	VFX_pane_copy(&m_backView, 0, 0, &m_screenView, 0, 0, -1);
}

void VideoDriver::LoadPalette(int32_t p_id)
{
	int32_t size;
	uint8_t* data;

	if (g_mw2Database->GetDBItemLZ(p_id, (void**) &data, &size)) {
		return;
	}

	VFX_PCX_palette(data, size, m_palette);

	if (ACQUIRE_FRAMEBUFFER() == 0) {
		VFX_PCX_draw(&m_screenView, data);
	}

	ExpandRect(m_screenView.m_x0, m_screenView.m_y0, m_screenView.m_x1, m_screenView.m_y1);
	m_paletteChanged = 1;
	m_allColors = 1;
	g_clearPaletteOnDraw = 1;
	MechHeapFree(g_primaryHeap, data);
}

// Draws frame p_frame of an SHP animation to the screen.
void VideoDriver::DrawShpFrame(
	void* p_shp,
	uint32_t p_frame,
	int32_t p_left,
	int32_t p_top,
	int32_t p_width,
	int32_t p_height
)
{
	if (ACQUIRE_FRAMEBUFFER() == 0) {
		VFX_shape_draw(&m_screenView, p_shp, p_frame, p_left, p_top);
	}

	ExpandRectBySize(p_left, p_top, p_width, p_height);
}

// DrawShpFrame, skipped unless the frame meets the dirty rectangle.
void VideoDriver::DrawShpFrameClipped(
	void* p_shp,
	uint32_t p_frame,
	int32_t p_left,
	int32_t p_top,
	int32_t p_width,
	int32_t p_height
)
{
	if (!IntersectsRectBySize(p_left, p_top, p_width, p_height)) {
		return;
	}

	if (ACQUIRE_FRAMEBUFFER() == 0) {
		VFX_shape_draw(&m_screenView, p_shp, p_frame, p_left, p_top);
	}

	ExpandRectBySize(p_left, p_top, p_width, p_height);
}

int32_t VideoDriver::DrawString(int32_t p_left, int32_t p_top, void* p_font, char* p_text, uint8_t* p_colorMap)
{
	char* cursor;
	int32_t width;
	int32_t acquired;

	if (p_text == NULL) {
		return 0;
	}

	width = 0;
	for (cursor = p_text; *cursor != '\0'; cursor++) {
		width += VFX_character_width(p_font, *cursor);
	}

	if (width == 0) {
		return width;
	}

	if (g_redrawingGlyphs && !IntersectsRectBySize(p_left, p_top, width, ((VideoFontHeader*) p_font)->m_height)) {
		return width;
	}

	if (p_colorMap == NULL) {
		p_colorMap = g_defaultColorMap;
	}

	acquired = ACQUIRE_FRAMEBUFFER();
	if (acquired == 0) {
		VFX_string_draw(&m_screenView, p_left, p_top, p_font, p_text, p_colorMap);
	}

	ExpandRectBySize(p_left, p_top, width, ((VideoFontHeader*) p_font)->m_height);
	return width;
}

int32_t VideoDriver::DrawChar(int32_t p_left, int32_t p_top, void* p_font, char p_char, uint8_t* p_colorMap)
{
	int32_t width;
	int32_t acquired;

	width = VFX_character_width(p_font, p_char);
	if (g_redrawingGlyphs && !IntersectsRectBySize(p_left, p_top, width, ((VideoFontHeader*) p_font)->m_height)) {
		return width;
	}

	if (p_colorMap == NULL) {
		p_colorMap = g_defaultColorMap;
	}

	acquired = ACQUIRE_FRAMEBUFFER();
	if (acquired == 0) {
		VFX_character_draw(&m_screenView, p_left, p_top, p_font, p_char, p_colorMap);
	}

	ExpandRectBySize(p_left, p_top, width, ((VideoFontHeader*) p_font)->m_height);
	return width;
}

void VideoDriver::AddGlyph(TextGlyph* p_item, int32_t p_overlay)
{
	if (p_overlay) {
		m_overlayGlyphs->Add(p_item);
	}
	else {
		m_glyphs->Add(p_item);
	}
}

void VideoDriver::RemoveGlyph(TextGlyph* p_item)
{
	m_overlayGlyphs->Remove(p_item);
	m_glyphs->Remove(p_item);
}

void VideoDriver::RedrawGlyphs(int32_t p_overlay)
{
	g_redrawingGlyphs = 1;

	if (p_overlay) {
		m_overlayGlyphs->DrawAll();
	}
	else {
		m_glyphs->DrawAll();
	}

	g_redrawingGlyphs = 0;
}

void VideoDriver::ClearGlyphs(uint8_t p_delete)
{
	m_overlayGlyphs->Clear(p_delete);
	m_glyphs->Clear(p_delete);
}

void VideoDriver::ActivateFramebuffer()
{
	if (ACQUIRE_FRAMEBUFFER() == 0) {
		VFX_pane_wipe(&m_screenView, 0);
	}

	g_currentRefreshMode->m_flip();
}
