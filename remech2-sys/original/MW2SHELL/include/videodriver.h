#ifndef VIDEODRIVER_H
#define VIDEODRIVER_H

#include "decomp.h"
#include "palettecolor.h"
#include "pane.h"
#include "types.h"
#include "window.h"

class TextGlyphList;
class TextGlyph;
class TMPackDataBase;

#pragma pack(1)
// SIZE 0x3ae
class VideoDriver {
public:
	VideoDriver();
	~VideoDriver();

	void ExpandRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
	void ExpandRectBySize(MechS32 p_left, MechS32 p_top, MechS32 p_width, MechS32 p_height);
	MechS32 IntersectsRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
	MechS32 IntersectsRectBySize(MechS32 p_left, MechS32 p_top, MechS32 p_width, MechS32 p_height);
	void UpdatePalette();
	void DrawShell();
	void DrawFmv();
	void GetPalette(PaletteColor* p_palette);
	void SetPalette(PaletteColor* p_palette, undefined4 p_allColors);
	void LoadPicturePalette(undefined* p_data, MechS32 p_size, PaletteColor* p_palette);
	void ReadPictureSize(MechS32* p_maxX, MechS32* p_maxY, undefined* p_data, MechS32 p_size, MechS32 p_type);
	void ShowPicture(
		undefined* p_picture,
		MechS32 p_pictureLength,
		MechS32 p_pictureType,
		MechU8 p_unk0x08,
		MechU8 p_unk0x09
	);
	void LoadBackground(TMPackDataBase* p_database, MechS32 p_id);
	void DrawPicture(undefined* p_data, MechS32 p_type, PANE* p_view);
	void DrawLine(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom, MechS32 p_color);
	void DrawPixels(undefined* p_pixels, MechS32 p_left, MechS32 p_top, MechS32 p_width, MechS32 p_height);
	void DrawPixelsClipped(undefined* p_pixels, MechS32 p_left, MechS32 p_top, MechS32 p_width, MechS32 p_height);
	void FUN_1000705f(undefined* p_pixels, MechS32 p_left, MechS32 p_top, MechS32 p_width, MechS32 p_height);
	void ReadPixels(undefined* p_pixels, MechS32 p_left, MechS32 p_top, MechS32 p_width, MechS32 p_height);
	void RestoreBackground(MechS32 p_left, MechS32 p_top, MechS32 p_width, MechS32 p_height);
	void CopyScreenToBackground();
	void CopyBackgroundToScreen();
	void LoadPalette(MechS32 p_id);
	void DrawShpFrame(
		void* p_shp,
		undefined4 p_frame,
		MechS32 p_left,
		MechS32 p_top,
		MechS32 p_width,
		MechS32 p_height
	);
	void DrawShpFrameClipped(
		void* p_shp,
		undefined4 p_frame,
		MechS32 p_left,
		MechS32 p_top,
		MechS32 p_width,
		MechS32 p_height
	);
	MechS32 DrawString(MechS32 p_left, MechS32 p_top, void* p_font, MechChar* p_text, undefined* p_colorMap);
	MechS32 DrawChar(MechS32 p_left, MechS32 p_top, void* p_font, MechChar p_char, undefined* p_colorMap);
	void AddGlyph(TextGlyph* p_item, MechS32 p_overlay);
	void RemoveGlyph(TextGlyph* p_item);
	void RedrawGlyphs(MechS32 p_overlay);
	void ClearGlyphs(MechU8 p_delete);
	void ActivateFramebuffer();

private:
	MechS32 m_pictureSize;            // 0x00 — the last picture's, see VFX_PCX_resolution
	undefined4 m_unk0x04;             // 0x04 — never accessed
	MechU8 m_unk0x08;                 // 0x08 — ShowPicture stores it, nothing reads it
	MechU8 m_unk0x09;                 // 0x09 — ShowPicture stores it, nothing reads it
	undefined* m_picture;             // 0x0a — ShowPicture's
	MechS32 m_pictureLength;          // 0x0e
	MechS32 m_pictureType;            // 0x12 — 2, the only type ReadPictureSize knows
	TextGlyphList* m_overlayGlyphs;   // 0x16 — redrawn after the videos' frames
	TextGlyphList* m_glyphs;          // 0x1a — redrawn before them
	undefined4 m_paletteChanged;      // 0x1e — the next draw loads m_palette
	undefined4 m_allColors;           // 0x22 — for m_setPalette
	undefined m_unk0x26[0x2e - 0x26]; // 0x26 — never accessed

public:
	// LoopingMovie decodes into m_screenBuffer.m_pixels directly.
	WINDOW m_screenBuffer; // 0x2e

	// ShellWindowProc clears m_backBuffer.m_pixels directly: an inline accessor would leave a jmp at /Ob1.
	WINDOW m_backBuffer; // 0x42

private:
	PANE m_screenView; // 0x56

public:
	// PopupPicture draws into m_backView directly.
	PANE m_backView; // 0x6a

private:
	PANE m_dirtyView;              // 0x7e
	PaletteColor m_palette[0x100]; // 0x92
	MechS32 m_width;               // 0x392
	MechS32 m_height;              // 0x396
	MechS32 m_pictureMaxX;         // 0x39a
	MechS32 m_pictureMaxY;         // 0x39e
	undefined4 m_unk0x3a2;         // 0x3a2 — only cleared, by the constructor

public:
	// The color RestoreBackground fills with instead of copying the background, -1 to copy.
	// The options, leaderboard, credits and cockpit controls screens set it to 0 directly.
	MechS32 m_restoreColor; // 0x3a6

	// Set by the full-screen video player on its first frame: the next draw keeps the frame as
	// the background before loading the palette.
	undefined4 m_fmvFirstFrame; // 0x3aa
};
#pragma pack()

// The globals of videodriver.cpp that other units use.
extern MechS32 g_redrawingGlyphs;
extern MechS32 g_clearPaletteOnDraw;
extern PaletteColor g_tempPalette[0x100];
extern MechU8 g_defaultColorMap[0x100];

#endif // VIDEODRIVER_H
