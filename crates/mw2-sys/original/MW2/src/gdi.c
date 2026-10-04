#include "gdi.h"

#include "brightness.h"
#include "debugprint.h"
#include "decomp.h"
#include "displaybackend.h"
#include "drawbitmapinfo.h"
#include "palettecolor.h"
#include "refreshmode.h"
#include "simmain.h"
#include "types.h"
#include "window.h"

#include <string.h>
#include <windows.h>

// The GDI display back end and its refresh mode. Unlike the shell's, the simulator's draws into a
// heap buffer and hands it to the window with SetDIBitsToDevice; GdiUpdateDibSection, the shell's
// DIB section path, is left over and unreferenced. The DIB's color table holds palette indices
// (DIB_PAL_COLORS) into a logical palette realized in the window DC.

MechS32 GdiBegin(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height);
MechS32 GdiEnd(void);
MechS32 GdiUpdateDibSection(void);
MechS32 GdiBlitFlip(void);
MechS32 GdiBitBltRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 GdiBitBltRectWithMenu(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 GdiStretchBlit(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 GdiStretchBlit2(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 GdiRealizePalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors);
MechS32 GdiSetPalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors);
MechS32 GdiSetPaletteWithBrightness(PaletteColor* p_palette);
MechS32 GdiBlendPalettes(PaletteColor* p_palette, MechS32 p_steps);
MechS32 GdiAcquireFramebuffer(void);

// SIZE 0x404
// A LOGPALETTE with room for all 256 entries.
typedef struct GdiLogPalette {
	WORD m_version;              // 0x00
	WORD m_numEntries;           // 0x02
	PALETTEENTRY m_entries[256]; // 0x04
} GdiLogPalette;

DECOMP_SIZE_ASSERT(GdiLogPalette, 0x404)

// GLOBAL: MW2 0x100ad5f0
HDC g_gdiWindowDc = NULL;

// GLOBAL: MW2 0x100ad5f4
HPALETTE g_gdiPalette = NULL;

// GLOBAL: MW2 0x100ad5f8
RGBQUAD g_gdiColorTable[256] = {0};

// Cleared by GdiEnd, never set.
// GLOBAL: MW2 0x100ad9f8
HANDLE g_unk0x100ad9f8 = NULL;

// GLOBAL: MW2 0x100ad9fc
HDC g_gdiMemoryDc = NULL;

// GLOBAL: MW2 0x100ada00
HBITMAP g_gdiDibSection = NULL;

// GLOBAL: MW2 0x100ada04
undefined4 g_unk0x100ada04 = 0;

// GLOBAL: MW2 0x100ada08
MechS32 g_gdiInitialized = FALSE;

// GLOBAL: MW2 0x100ada10
GdiLogPalette g_gdiLogPalette = {0x300, 256};

// GLOBAL: MW2 0x100ade18
DisplayBackend g_gdiBackend = {
	c_displayBackendGdi,
	c_windowModeWindowed,
	WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
	GdiBegin,
	GdiEnd,
	GdiSetPalette,
	GdiSetPaletteWithBrightness,
	GdiBlendPalettes,
	GdiAcquireFramebuffer,
	0
};

// GLOBAL: MW2 0x100ade40
RefreshMode g_gdiRefreshMode =
	{5, c_displayBackendGdi, 1, 0, GdiBegin, GdiEnd, GdiBlitFlip, GdiBitBltRect, GdiStretchBlit};

// The shell's GdiSetPalette keeps its p_allColors here; the simulator's never touches it.
// GLOBAL: MW2 0x100ade64
MechS32 g_gdiAllColors = TRUE;

// GLOBAL: MW2 0x100c3560
MechS32 g_gdiResult;

// GLOBAL: MW2 0x100c3564
HGDIOBJ g_gdiOldBitmap;

// GLOBAL: MW2 0x100c356c
HPALETTE g_gdiOldPalette;

// Switches to the GDI extension and allocates the frame p_buffer describes: a p_width x p_height
// buffer of palette indices, presented with SetDIBitsToDevice. Returns 0 on success, -1 if the
// palette fails, 2 if the allocation fails and 1 if the first present fails.
// FUNCTION: MW2 0x1006de70
MechS32 GdiBegin(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height)
{
	MechU16* indices;
	MechS32 i;

	if (g_gdiWindowDc) {
		return 0;
	}

	if (g_currentDisplayBackend->m_id != c_displayBackendGdi) {
		g_currentDisplayBackend->m_end();
		if (g_gdiBackend.m_windowMode != g_windowMode) {
			AdjustWindowSize(&g_gdiBackend);
		}

		g_currentDisplayBackend = g_displayBackends[c_displayBackendGdi];
	}

	g_gdiWindowDc = GetDC(g_gameWindow);
	InitBitmapInfo(p_width, p_height);
	indices = (MechU16*) g_bitmapInfo.m_colors;
	for (i = 0; i < 256; i++) {
		indices[i] = i;
	}

	if (!GdiRealizePalette(0, 256, g_paletteColors, 0)) {
		return -1;
	}

	p_buffer->m_buffer = MechHeapAllocZeroed(g_primaryHeap, p_width * p_height);
	if (p_buffer->m_buffer == NULL) {
		return 2;
	}

	g_dibBits = p_buffer->m_buffer;
	p_buffer->m_xMax = p_width - 1;
	p_buffer->m_yMax = p_height - 1;
	p_buffer->m_shadow = 0;
	p_buffer->m_bitmapInfo = &g_bitmapInfo;
	memset(p_buffer->m_buffer, 0, p_width * p_height);
	if (GdiBlitFlip()) {
		GdiEnd();
		return 1;
	}

	g_gdiInitialized = TRUE;
	return 0;
}

// FUNCTION: MW2 0x1006dffe
MechS32 GdiEnd(void)
{
	g_gdiInitialized = FALSE;
	if (g_gdiWindowDc) {
		if (g_gdiPalette) {
			SelectObject(g_gdiWindowDc, g_gdiOldPalette);
			DeleteObject(g_gdiPalette);
		}

		if (g_dibBits) {
			MechHeapFree(g_primaryHeap, g_dibBits);
		}

		ReleaseDC(g_gameWindow, g_gdiWindowDc);
	}

	g_gdiWindowDc = NULL;
	g_gdiPalette = NULL;
	g_unk0x100ad9f8 = NULL;
	g_gdiMemoryDc = NULL;
	g_gdiDibSection = NULL;
	g_refreshModeBuffer->m_buffer = g_dibBits = NULL;
	g_unk0x100ada04 = 0;
	return 0;
}

// Unreferenced: an older path that drew into a DIB section instead of a heap buffer. The first call
// creates the section as the frame; later calls only refresh its colour table.
// FUNCTION: MW2 0x1006e0d8
MechS32 GdiUpdateDibSection(void)
{
	if (g_gdiDibSection) {
		g_gdiResult = SetDIBColorTable(g_gdiMemoryDc, 0, 256, g_gdiColorTable);
	}
	else {
		g_gdiDibSection = CreateDIBSection(
			g_gdiWindowDc,
			(BITMAPINFO*) &g_bitmapInfo,
			DIB_PAL_COLORS,
			(void**) &g_refreshModeBuffer->m_buffer,
			NULL,
			0
		);
		if (g_gdiDibSection == NULL || g_refreshModeBuffer->m_buffer == NULL) {
			DebugPrint("GDI CreateDIBSection failed: %d\n", GetLastError());
			return 0;
		}

		g_dibBits = g_refreshModeBuffer->m_buffer;
		g_gdiOldBitmap = SelectObject(g_gdiMemoryDc, g_gdiDibSection);
	}

	return 1;
}

// FUNCTION: MW2 0x1006e197
MechS32 GdiBlitFlip(void)
{
	g_gdiResult = SetDIBitsToDevice(
		g_gdiWindowDc,
		0,
		0,
		g_refreshModeWidth,
		g_refreshModeHeight,
		0,
		0,
		0,
		g_refreshModeHeight,
		g_dibBits,
		(BITMAPINFO*) &g_bitmapInfo,
		DIB_PAL_COLORS
	);
	if (g_gdiResult == 0) {
		DebugPrint("GDI StretchBlt err: %d\n", GetLastError());
	}

	return g_refreshModeHeight == g_gdiResult ? 0 : -1;
}

// Presents the inclusive rectangle (p_left, p_top)-(p_right, p_bottom). The DIB is bottom-up.
// FUNCTION: MW2 0x1006e21f
MechS32 GdiBitBltRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	g_gdiResult = SetDIBitsToDevice(
		g_gdiWindowDc,
		p_left,
		p_top,
		p_right - p_left + 1,
		p_bottom - p_top + 1,
		p_left,
		g_refreshModeHeight - p_bottom - 1,
		0,
		g_refreshModeHeight,
		g_dibBits,
		(BITMAPINFO*) &g_bitmapInfo,
		DIB_PAL_COLORS
	);
	if (g_gdiResult == 0) {
		DebugPrint("GDI BitBlt Rect err: %d\n", GetLastError());
	}

	return g_refreshModeHeight == g_gdiResult ? 0 : -1;
}

// Unreferenced: GdiBitBltRect shifted up by the menu bar, without flipping the source.
// FUNCTION: MW2 0x1006e2b9
MechS32 GdiBitBltRectWithMenu(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	g_gdiResult = SetDIBitsToDevice(
		g_gdiWindowDc,
		p_left,
		p_top - GetSystemMetrics(SM_CYMENU),
		p_right - p_left + 1,
		p_bottom - p_top + 1,
		p_left,
		p_top,
		0,
		g_refreshModeHeight,
		g_dibBits,
		(BITMAPINFO*) &g_bitmapInfo,
		DIB_PAL_COLORS
	);
	if (g_gdiResult == 0) {
		DebugPrint("GDI BitBlt Rect err: %d\n", GetLastError());
	}

	return g_refreshModeHeight == g_gdiResult ? 0 : -1;
}

// Stretches the inclusive rectangle (p_left, p_top)-(p_right, p_bottom) over the whole window.
// FUNCTION: MW2 0x1006e357
MechS32 GdiStretchBlit(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	g_gdiResult = StretchDIBits(
		g_gdiWindowDc,
		0,
		0,
		g_refreshModeWidth,
		g_refreshModeHeight,
		p_left,
		g_refreshModeHeight - p_bottom - 1,
		p_right - p_left + 1,
		p_bottom - p_top + 1,
		g_dibBits,
		(BITMAPINFO*) &g_bitmapInfo,
		DIB_PAL_COLORS,
		SRCCOPY
	);

	return g_refreshModeHeight == g_gdiResult ? 0 : -1;
}

// Unreferenced: the same as GdiStretchBlit.
// FUNCTION: MW2 0x1006e3d5
MechS32 GdiStretchBlit2(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	g_gdiResult = StretchDIBits(
		g_gdiWindowDc,
		0,
		0,
		g_refreshModeWidth,
		g_refreshModeHeight,
		p_left,
		g_refreshModeHeight - p_bottom - 1,
		p_right - p_left + 1,
		p_bottom - p_top + 1,
		g_dibBits,
		(BITMAPINFO*) &g_bitmapInfo,
		DIB_PAL_COLORS,
		SRCCOPY
	);

	return g_refreshModeHeight == g_gdiResult ? 0 : -1;
}

// Stores p_count colours from p_first in g_paletteColors, then loads all 256 into the GDI palette
// (scaled from 6 to 8 bits) and realizes it.
// FUNCTION: MW2 0x1006e453
MechS32 GdiRealizePalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors)
{
	MechS32 i;

	if (p_palette == NULL || p_first < 0 || p_first > 255 || p_count <= 0 || 256 - p_first < p_count) {
		return FALSE;
	}

	for (i = 0; i < p_count; i++) {
		g_paletteColors[p_first + i] = p_palette[i];
	}

	for (i = 0; i < 256; i++) {
		g_gdiLogPalette.m_entries[i].peRed = g_paletteColors[i].m_red * 4;
		g_gdiLogPalette.m_entries[i].peGreen = g_paletteColors[i].m_green * 4;
		g_gdiLogPalette.m_entries[i].peBlue = g_paletteColors[i].m_blue * 4;
		g_gdiLogPalette.m_entries[i].peFlags = PC_NOCOLLAPSE;
	}

	if (g_gdiPalette == NULL) {
		g_gdiPalette = CreatePalette((LOGPALETTE*) &g_gdiLogPalette);
		g_gdiOldPalette = SelectPalette(g_gdiWindowDc, g_gdiPalette, FALSE);
	}
	else {
		SetPaletteEntries(g_gdiPalette, 0, 256, g_gdiLogPalette.m_entries);
	}

	RealizePalette(g_gdiWindowDc);
	return TRUE;
}

// FUNCTION: MW2 0x1006e5d8
MechS32 GdiSetPalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors)
{
	MechS32 result;

	if (!g_gdiInitialized) {
		return -1;
	}

	result = GdiRealizePalette(p_first, p_count, p_palette, p_allColors);
	if (!result) {
		return -1;
	}

	return 0;
}

// Keeps p_palette in g_paletteColorsPreBrightness and loads their brightness-adjusted copies.
// FUNCTION: MW2 0x1006e633
MechS32 GdiSetPaletteWithBrightness(PaletteColor* p_palette)
{
	MechS32 i;

	for (i = 0; i < 256; i++) {
		g_paletteColorsPreBrightness[i] = p_palette[i];
		CopyPaletteColorWithBrightness(&p_palette[i], &g_paletteColors[i]);
	}

	GdiRealizePalette(0, 256, g_paletteColors, 0);
	GdiBlitFlip();
	g_refreshModeBuffer->m_buffer = g_dibBits;
	return 0;
}

// Fades linearly from the current palette to p_palette over p_steps / 2 frames, writing each
// in-between palette to p_palette (which ends back at the target).
// FUNCTION: MW2 0x1006e6d0
MechS32 GdiBlendPalettes(PaletteColor* p_palette, MechS32 p_steps)
{
	MechS32 i;
	MechS32 j;
	MechDouble deltas[0x100][3];

	if (p_palette == NULL) {
		return -1;
	}

	p_steps >>= 1;
	for (i = 0; i < 256; i++) {
		g_paletteColorsPreBrightness[i] = g_paletteColors[i];
		deltas[i][0] = (MechDouble) (p_palette[i].m_red - g_paletteColorsPreBrightness[i].m_red) / p_steps;
		deltas[i][1] = (MechDouble) (p_palette[i].m_green - g_paletteColorsPreBrightness[i].m_green) / p_steps;
		deltas[i][2] = (MechDouble) (p_palette[i].m_blue - g_paletteColorsPreBrightness[i].m_blue) / p_steps;
	}

	i = p_steps;
	while (i--) {
		for (j = 0; j < 256; j++) {
			p_palette[j].m_red = (MechU8) ((p_steps - i) * deltas[j][0]) + g_paletteColorsPreBrightness[j].m_red;
			p_palette[j].m_green = (MechU8) ((p_steps - i) * deltas[j][1]) + g_paletteColorsPreBrightness[j].m_green;
			p_palette[j].m_blue = (MechU8) ((p_steps - i) * deltas[j][2]) + g_paletteColorsPreBrightness[j].m_blue;
		}

		GdiRealizePalette(0, 256, p_palette, 0);
		GdiBlitFlip();
	}

	g_refreshModeBuffer->m_buffer = g_dibBits;
	return 0;
}

// FUNCTION: MW2 0x1006e94f
MechS32 GdiAcquireFramebuffer(void)
{
	g_refreshModeBuffer->m_buffer = g_dibBits;
	return 0;
}
