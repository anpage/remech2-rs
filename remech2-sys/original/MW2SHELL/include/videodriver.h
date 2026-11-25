#ifndef VIDEODRIVER_H
#define VIDEODRIVER_H

#include "palettecolor.h"
#include "pane.h"
#include "types.h"
#include "window.h"

class TextGlyphList;
class TextGlyph;
class TMPackDataBase;

#pragma pack(1)
class VideoDriver {
public:
	VideoDriver();
	~VideoDriver();

	void ExpandRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);
	void ExpandRectBySize(int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height);
	int32_t IntersectsRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);
	int32_t IntersectsRectBySize(int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height);
	void UpdatePalette();
	void DrawShell();
	void DrawFmv();
	void GetPalette(PaletteColor* p_palette);
	void SetPalette(PaletteColor* p_palette, uint32_t p_allColors);
	void LoadBackground(TMPackDataBase* p_database, int32_t p_id);
	void DrawLine(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom, int32_t p_color);
	void DrawPixels(uint8_t* p_pixels, int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height);
	void DrawPixelsClipped(uint8_t* p_pixels, int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height);
	void ReadPixels(uint8_t* p_pixels, int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height);
	void RestoreBackground(int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height);
	void CopyScreenToBackground();
	void CopyBackgroundToScreen();
	void LoadPalette(int32_t p_id);
	void DrawShpFrame(void* p_shp, uint32_t p_frame, int32_t p_left, int32_t p_top, int32_t p_width, int32_t p_height);
	void DrawShpFrameClipped(
		void* p_shp,
		uint32_t p_frame,
		int32_t p_left,
		int32_t p_top,
		int32_t p_width,
		int32_t p_height
	);
	int32_t DrawString(int32_t p_left, int32_t p_top, void* p_font, char* p_text, uint8_t* p_colorMap);
	int32_t DrawChar(int32_t p_left, int32_t p_top, void* p_font, char p_char, uint8_t* p_colorMap);
	void AddGlyph(TextGlyph* p_item, int32_t p_overlay);
	void RemoveGlyph(TextGlyph* p_item);
	void RedrawGlyphs(int32_t p_overlay);
	void ClearGlyphs(uint8_t p_delete);
	void ActivateFramebuffer();

private:
	TextGlyphList* m_overlayGlyphs; // redrawn after the videos' frames
	TextGlyphList* m_glyphs;        // redrawn before them
	uint32_t m_paletteChanged;      // the next draw loads m_palette
	uint32_t m_allColors;           // for m_setPalette

public:
	// LoopingMovie decodes into m_screenBuffer.m_pixels directly.
	WINDOW m_screenBuffer;

	WINDOW m_backBuffer;

	PANE m_screenView;

public:
	// PopupPicture draws into m_backView directly.
	PANE m_backView;

private:
	PANE m_dirtyView;
	PaletteColor m_palette[0x100];
	int32_t m_width;
	int32_t m_height;

public:
	// The color RestoreBackground fills with instead of copying the background, -1 to copy.
	// The options, leaderboard, credits and cockpit controls screens set it to 0 directly.
	int32_t m_restoreColor;

	// Set by the full-screen video player on its first frame: the next draw keeps the frame as
	// the background before loading the palette.
	uint32_t m_fmvFirstFrame;
};
#pragma pack()

// The globals of videodriver.cpp that other units use.
extern int32_t g_redrawingGlyphs;
extern int32_t g_clearPaletteOnDraw;
extern PaletteColor g_tempPalette[0x100];
extern uint8_t g_defaultColorMap[0x100];

#endif // VIDEODRIVER_H
