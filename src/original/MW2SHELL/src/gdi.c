#include "gdi.h"

#include "brightness.h"
#include "debugprint.h"
#include "decomp.h"
#include "displaybackend.h"
#include "drawbitmapinfo.h"
#include "palettecolor.h"
#include "refreshmode.h"
#include "types.h"
#include "window.h"

#include <string.h>
#include <windows.h>

// The GDI display back end and its refresh mode: the frame is drawn into a DIB section selected
// into a memory DC, then blitted to the window. The DIB's color table holds palette indices (DIB_PAL_COLORS) into
// a logical palette realized in the window DC; the lower and upper halves of the system's static
// colors are left alone unless SetPalette asks for all 256.

MechS32 GdiBegin(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height);
MechS32 GdiEnd();
MechS32 GdiUpdateDibSection();
MechS32 GdiBlitFlip();
MechS32 GdiBitBltRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 GdiStretchBlit(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 GdiRealizePalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors);
void GdiLoadStaticColors();
MechS32 GdiSetPalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors);
MechS32 GdiSetPaletteWithBrightness(PaletteColor* p_palette);
MechS32 GdiBlendPalettes(PaletteColor* p_palette, MechS32 p_steps);
MechS32 GdiAcquireFramebuffer();

// A LOGPALETTE with room for all 256 entries.
// SIZE 0x404
typedef struct GdiLogPalette {
	WORD m_version;                // 0x00
	WORD m_numEntries;             // 0x02
	PALETTEENTRY m_entries[0x100]; // 0x04
} GdiLogPalette;

DECOMP_SIZE_ASSERT(GdiLogPalette, 0x404)

// GLOBAL: MW2SHELL 0x10066df8
HDC g_gdiWindowDc = NULL;

// GLOBAL: MW2SHELL 0x10066dfc
HPALETTE g_gdiPalette = NULL;

// The DIB's color table as RGB, loaded with SetDIBColorTable once the DIB section exists.
// GLOBAL: MW2SHELL 0x10066e00
RGBQUAD g_gdiColorTable[0x100] = {0};

// Closed by GdiEnd, never opened: nothing says what it held.
// GLOBAL: MW2SHELL 0x10067200
HANDLE g_unk0x10067200 = NULL;

// GLOBAL: MW2SHELL 0x10067204
HDC g_gdiMemoryDc = NULL;

// GLOBAL: MW2SHELL 0x10067208
HBITMAP g_gdiDibSection = NULL;

// Only cleared (GdiEnd); nothing reads it.
// GLOBAL: MW2SHELL 0x1006720c
undefined4 g_unk0x1006720c = 0;

// GLOBAL: MW2SHELL 0x10067210
MechS32 g_gdiInitialized = FALSE;

// GLOBAL: MW2SHELL 0x10067218
GdiLogPalette g_gdiLogPalette = {0x300, 0x100};

// GLOBAL: MW2SHELL 0x10067620
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

// GLOBAL: MW2SHELL 0x10067648
RefreshMode g_gdiRefreshMode =
	{5, c_displayBackendGdi, 1, 0, GdiBegin, GdiEnd, GdiBlitFlip, GdiBitBltRect, GdiStretchBlit};

// The p_allColors of the last GdiSetPalette.
// GLOBAL: MW2SHELL 0x1006766c
MechS32 g_gdiAllColors = TRUE;

// Not a function: refreshmode.c calls it as one (see there), and the linker binds the calls here.
// GLOBAL: MW2SHELL 0x100965d4
undefined4 PauseTimer;

// GLOBAL: MW2SHELL 0x100965f4
MechS32 g_gdiResult;

// GLOBAL: MW2SHELL 0x100965f8
HGDIOBJ g_gdiOldBitmap;

// The number of static colors at each end of the system palette.
// GLOBAL: MW2SHELL 0x100965fc
MechS32 g_gdiStaticColorCount;

// GLOBAL: MW2SHELL 0x10096600
HPALETTE g_gdiOldPalette;

// Stack-slot permutation: the original puts colors at [ebp-4] and i at [ebp-8].
// FUNCTION: MW2SHELL 0x10030b90
MechS32 GdiBegin(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height)
{
	MechU16* colors;
	MechS32 i;

	if (g_gdiWindowDc != NULL) {
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

	colors = (MechU16*) g_bitmapInfo.m_colors;
	for (i = 0; i < 0x100; i++) {
		colors[i] = i;
	}

	GdiLoadStaticColors();
	if (!GdiRealizePalette(0, 0x100, g_paletteColors, FALSE)) {
		return -1;
	}

	g_gdiMemoryDc = CreateCompatibleDC(g_gdiWindowDc);
	if (g_gdiMemoryDc == NULL) {
		DebugPrint("GDI CreateCompatibleDC failed\n");
		return 0;
	}

	if (!GdiUpdateDibSection()) {
		return -1;
	}

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

// FUNCTION: MW2SHELL 0x10030d31
MechS32 GdiEnd()
{
	g_gdiInitialized = FALSE;

	if (g_gdiWindowDc != NULL) {
		if (g_gdiPalette != NULL) {
			SelectObject(g_gdiWindowDc, g_gdiOldPalette);
			DeleteObject(g_gdiPalette);
		}

		if (g_gdiMemoryDc != NULL) {
			SelectObject(g_gdiMemoryDc, g_gdiOldBitmap);
			DeleteObject(g_gdiDibSection);
			DeleteDC(g_gdiMemoryDc);
			if (g_unk0x10067200 != NULL) {
				CloseHandle(g_unk0x10067200);
			}
		}

		ReleaseDC(g_gameWindow, g_gdiWindowDc);
	}

	g_gdiWindowDc = NULL;
	g_gdiPalette = NULL;
	g_unk0x10067200 = NULL;
	g_gdiMemoryDc = NULL;
	g_gdiDibSection = NULL;
	g_refreshModeBuffer->m_buffer = g_dibBits = NULL;
	g_unk0x1006720c = 0;
	return 0;
}

// Creates the DIB section on the first call, then only reloads its color table.
// FUNCTION: MW2SHELL 0x10030e3a
MechS32 GdiUpdateDibSection()
{
	if (g_gdiDibSection != NULL) {
		g_gdiResult = SetDIBColorTable(g_gdiMemoryDc, 0, 0x100, g_gdiColorTable);
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
			return FALSE;
		}

		g_dibBits = g_refreshModeBuffer->m_buffer;
		g_gdiOldBitmap = SelectObject(g_gdiMemoryDc, g_gdiDibSection);
	}

	return TRUE;
}

// FUNCTION: MW2SHELL 0x10030ef9
MechS32 GdiBlitFlip()
{
	g_gdiResult = BitBlt(g_gdiWindowDc, 0, 0, g_refreshModeWidth, g_refreshModeHeight, g_gdiMemoryDc, 0, 0, SRCCOPY);
	if (!g_gdiResult) {
		DebugPrint("GDI StretchBlt err: %d\n", GetLastError());
	}

	return g_refreshModeHeight == g_gdiResult ? 0 : -1;
}

// FUNCTION: MW2SHELL 0x10030f77
MechS32 GdiBitBltRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	g_gdiResult = BitBlt(
		g_gdiWindowDc,
		p_left,
		p_top,
		p_right - p_left + 1,
		p_bottom - p_top + 1,
		g_gdiMemoryDc,
		p_left,
		p_top,
		SRCCOPY
	);
	if (!g_gdiResult) {
		DebugPrint("GDI BitBlt Rect err: %d\n", GetLastError());
	}

	return g_refreshModeHeight == g_gdiResult ? 0 : -1;
}

// GdiBitBltRect for a window with its menu bar shown: the client area starts one menu height
// lower.
// FUNCTION: MW2SHELL 0x10031001
MechS32 GdiBitBltRectWithMenu(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	g_gdiResult = BitBlt(
		g_gdiWindowDc,
		p_left,
		p_top - GetSystemMetrics(SM_CYMENU),
		p_right - p_left + 1,
		p_bottom - p_top + 1,
		g_gdiMemoryDc,
		p_left,
		p_top,
		SRCCOPY
	);
	if (!g_gdiResult) {
		DebugPrint("GDI BitBlt Rect err: %d\n", GetLastError());
	}

	return g_refreshModeHeight == g_gdiResult ? 0 : -1;
}

// FUNCTION: MW2SHELL 0x10031095
MechS32 GdiStretchBlit(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	g_gdiResult = StretchBlt(
		g_gdiWindowDc,
		0,
		0,
		g_refreshModeWidth,
		g_refreshModeHeight,
		g_gdiMemoryDc,
		p_left,
		p_top,
		p_right - p_left + 1,
		p_bottom - p_top + 1,
		SRCCOPY
	);

	return g_refreshModeHeight == g_gdiResult ? 0 : -1;
}

// Blits unscaled into the middle of the window, a square of side p_right - p_left + 1 at
// (160, 140); p_bottom is ignored.
// FUNCTION: MW2SHELL 0x10031106
MechS32 GdiBlitCentered(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	g_gdiResult = BitBlt(
		g_gdiWindowDc,
		160,
		140,
		p_right - p_left + 1,
		p_right - p_left + 1,
		g_gdiMemoryDc,
		p_left,
		p_top,
		SRCCOPY
	);

	return g_refreshModeHeight == g_gdiResult ? 0 : -1;
}

// Copies p_count colors into g_paletteColors from p_first and realizes the logical palette,
// either all 256 entries or only those between the static colors.
// FUNCTION: MW2SHELL 0x10031171
MechS32 GdiRealizePalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors)
{
	MechS32 i;

	if (p_palette == NULL || p_first < 0 || p_first > 0xff || p_count <= 0 || p_count > 0x100 - p_first) {
		return FALSE;
	}

	for (i = 0; i < p_count; i++) {
		g_paletteColors[p_first + i] = p_palette[i];
	}

	if (p_allColors) {
		for (i = 0; i < 0x100; i++) {
			g_gdiColorTable[i].rgbRed = g_paletteColors[i].m_red * 4;
			g_gdiLogPalette.m_entries[i].peRed = g_gdiColorTable[i].rgbRed;
			g_gdiColorTable[i].rgbGreen = g_paletteColors[i].m_green * 4;
			g_gdiLogPalette.m_entries[i].peGreen = g_gdiColorTable[i].rgbGreen;
			g_gdiColorTable[i].rgbBlue = g_paletteColors[i].m_blue * 4;
			g_gdiLogPalette.m_entries[i].peBlue = g_gdiColorTable[i].rgbBlue;
			g_gdiLogPalette.m_entries[i].peFlags = PC_NOCOLLAPSE;
		}
	}
	else {
		for (i = g_gdiStaticColorCount; i < 0x100 - g_gdiStaticColorCount; i++) {
			g_gdiColorTable[i].rgbRed = g_paletteColors[i].m_red * 4;
			g_gdiLogPalette.m_entries[i].peRed = g_gdiColorTable[i].rgbRed;
			g_gdiColorTable[i].rgbGreen = g_paletteColors[i].m_green * 4;
			g_gdiLogPalette.m_entries[i].peGreen = g_gdiColorTable[i].rgbGreen;
			g_gdiColorTable[i].rgbBlue = g_paletteColors[i].m_blue * 4;
			g_gdiLogPalette.m_entries[i].peBlue = g_gdiColorTable[i].rgbBlue;
			g_gdiLogPalette.m_entries[i].peFlags = PC_NOCOLLAPSE;
		}
	}

	if (g_gdiPalette == NULL) {
		g_gdiPalette = CreatePalette((LOGPALETTE*) &g_gdiLogPalette);
		g_gdiOldPalette = SelectPalette(g_gdiWindowDc, g_gdiPalette, FALSE);
	}
	else {
		SetPaletteEntries(g_gdiPalette, 0, 0x100, g_gdiLogPalette.m_entries);
	}

	SetSystemPaletteUse(g_gdiWindowDc, SYSPAL_NOSTATIC);
	SetSystemPaletteUse(g_gdiWindowDc, SYSPAL_STATIC);
	RealizePalette(g_gdiWindowDc);
	return TRUE;
}

// On an 8-bit display, copies the system palette's static colors into both ends of the logical
// palette and the DIB color table.
// Operand order: the original compares i < g_gdiStaticColorCount as
// `cmp [g_gdiStaticColorCount], eax`; one declaration-order attempt didn't flip it.
// FUNCTION: MW2SHELL 0x10031424
void GdiLoadStaticColors()
{
	MechS32 i;

	if (GetDeviceCaps(g_gdiWindowDc, BITSPIXEL) != 8) {
		return;
	}

	g_gdiStaticColorCount = GetDeviceCaps(g_gdiWindowDc, NUMCOLORS);
	g_gdiStaticColorCount >>= 1;
	GetSystemPaletteEntries(g_gdiWindowDc, 0, g_gdiStaticColorCount, g_gdiLogPalette.m_entries);
	GetSystemPaletteEntries(
		g_gdiWindowDc,
		0x100 - g_gdiStaticColorCount,
		g_gdiStaticColorCount,
		&g_gdiLogPalette.m_entries[0x100 - g_gdiStaticColorCount]
	);

	for (i = 0; i < g_gdiStaticColorCount; i++) {
		g_gdiColorTable[i].rgbRed = g_gdiLogPalette.m_entries[i].peRed;
		g_gdiColorTable[i].rgbGreen = g_gdiLogPalette.m_entries[i].peGreen;
		g_gdiColorTable[i].rgbBlue = g_gdiLogPalette.m_entries[i].peBlue;
		g_gdiLogPalette.m_entries[i].peFlags = 0;
	}

	for (i = 0x100 - g_gdiStaticColorCount; i < 0x100; i++) {
		g_gdiColorTable[i].rgbRed = g_gdiLogPalette.m_entries[i].peRed;
		g_gdiColorTable[i].rgbGreen = g_gdiLogPalette.m_entries[i].peGreen;
		g_gdiColorTable[i].rgbBlue = g_gdiLogPalette.m_entries[i].peBlue;
		g_gdiLogPalette.m_entries[i].peFlags = 0;
	}
}

// FUNCTION: MW2SHELL 0x10031591
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

	if (!p_allColors && g_gdiAllColors) {
		GdiLoadStaticColors();
	}
	g_gdiAllColors = p_allColors;

	result = GdiUpdateDibSection();
	if (!result) {
		return -1;
	}

	return 0;
}

// FUNCTION: MW2SHELL 0x1003162c
MechS32 GdiSetPaletteWithBrightness(PaletteColor* p_palette)
{
	MechS32 i;

	for (i = 0; i < 0x100; i++) {
		g_paletteColorsPreBrightness[i] = p_palette[i];
		CopyPaletteColorWithBrightness(&p_palette[i], &g_paletteColors[i]);
	}

	GdiRealizePalette(0, 0x100, g_paletteColors, FALSE);
	GdiBlitFlip();
	g_refreshModeBuffer->m_buffer = g_dibBits;
	return 0;
}

// Fades from the current palette to p_palette in p_steps / 2 steps, one blit each. p_palette is
// overwritten with the intermediate colors.
// FUNCTION: MW2SHELL 0x100316c9
MechS32 GdiBlendPalettes(PaletteColor* p_palette, MechS32 p_steps)
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
		}

		GdiRealizePalette(0, 0x100, p_palette, FALSE);
		GdiBlitFlip();
	}

	g_refreshModeBuffer->m_buffer = g_dibBits;
	return 0;
}

// FUNCTION: MW2SHELL 0x10031948
MechS32 GdiAcquireFramebuffer()
{
	g_refreshModeBuffer->m_buffer = g_dibBits;
	return 0;
}
