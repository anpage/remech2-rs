#include "dispdib.h"

#include "brightness.h"
#include "debugprint.h"
#include "decomp.h"
#include "dispdibmode.h"
#include "displaybackend.h"
#include "drawbitmapinfo.h"
#include "palettecolor.h"
#include "refreshmode.h"
#include "types.h"
#include "window.h"

#include <windows.h>

// The DisplayDib display back end and its refresh mode: 320x200 fullscreen through a
// DisplayDibWindow. The frame is drawn into a DIB section and handed to the window with DDM_DRAW;
// the palette travels in the DIB format's color table (DDM_SETFMT). A second DIB section receives
// StretchBlt'ed movie frames.

MechS32 DispDibBegin(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height);
MechS32 DispDibEnd();
MechS32 DispDibFlip();
MechS32 DispDibBlitRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 DispDibStretchBlit(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 DispDibAcquireFramebuffer();
MechS32 DispDibSetPalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors);
MechS32 DispDibSetPaletteWithBrightness(PaletteColor* p_palette);
MechS32 DispDibBlendPalettes(PaletteColor* p_palette, MechS32 p_steps);

enum DispDibDrawFlags {
	c_dispDibDrawFlags = DISPLAYDIB_NOWAIT | DISPLAYDIB_DONTLOCKTASK
};

// GLOBAL: MW2SHELL 0x10066b18
HWND g_dispDibWindow = NULL;

// GLOBAL: MW2SHELL 0x10066b1c
HDC g_dispDibWindowDc = NULL;

// GLOBAL: MW2SHELL 0x10066b20
HDC g_dispDibDc = NULL;

// GLOBAL: MW2SHELL 0x10066b24
HDC g_dispDibStretchDc = NULL;

// GLOBAL: MW2SHELL 0x10066b28
HBITMAP g_dispDibBitmap = NULL;

// GLOBAL: MW2SHELL 0x10066b2c
HBITMAP g_dispDibStretchBitmap = NULL;

// GLOBAL: MW2SHELL 0x10066b30
undefined* g_dispDibStretchBits = NULL;

// GLOBAL: MW2SHELL 0x10066b34
MechS32 g_dispDibInitialized = FALSE;

// GLOBAL: MW2SHELL 0x10066b38
DisplayBackend g_dispDibBackend = {
	c_displayBackendDisplayDib,
	c_windowModeFullscreen,
	WS_POPUP,
	DispDibBegin,
	DispDibEnd,
	DispDibSetPalette,
	DispDibSetPaletteWithBrightness,
	DispDibBlendPalettes,
	DispDibAcquireFramebuffer,
	0
};

// GLOBAL: MW2SHELL 0x10066b60
RefreshMode g_dispDibRefreshMode =
	{4, c_displayBackendDisplayDib, 1, 0, DispDibBegin, DispDibEnd, DispDibFlip, DispDibBlitRect, DispDibStretchBlit};

// GLOBAL: MW2SHELL 0x1009675c
MechS32 g_dispDibResult;

// Empty and never called: there is nothing to name it after.
// FUNCTION: MW2SHELL 0x1002ee30
void FUN_1002ee30()
{
	return;
}

// FUNCTION: MW2SHELL 0x1002ee40
MechS32 DispDibBegin(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height)
{
	if (p_width != 320 || p_height != 200) {
		return -1;
	}

	if (g_dispDibWindow != NULL) {
		return 0;
	}

	if (g_currentDisplayBackend->m_id != c_displayBackendDisplayDib) {
		g_currentDisplayBackend->m_end();
		g_currentDisplayBackend = g_displayBackends[c_displayBackendDisplayDib];
		if (g_currentDisplayBackend->m_windowMode != g_windowMode) {
			AdjustWindowSize(g_currentDisplayBackend);
		}
	}

	InitBitmapInfo(p_width, p_height);
	g_dispDibWindow = DisplayDibWindowCreate(g_gameWindow, NULL);
	if (g_dispDibWindow == NULL) {
		DebugPrint("DisplayDibWindowCreate err: null hwnd\n");
		return 1;
	}

	g_dispDibWindowDc = GetDC(g_dispDibWindow);
	if (g_dispDibWindowDc == NULL) {
		DebugPrint("DispDib GetDC failed\n");
		return 2;
	}

	g_dispDibDc = CreateCompatibleDC(g_dispDibWindowDc);
	if (g_dispDibDc == NULL) {
		DebugPrint("DispDib CreateCompatibleDC failed\n");
		return 2;
	}

	g_dispDibStretchDc = CreateCompatibleDC(g_dispDibWindowDc);
	if (g_dispDibStretchDc == NULL) {
		DebugPrint("DispDib CreateCompatibleDC2 failed\n");
		return 2;
	}

	g_dispDibBitmap = CreateDIBSection(
		g_dispDibDc,
		(BITMAPINFO*) &g_bitmapInfo,
		DIB_RGB_COLORS,
		(void**) &p_buffer->m_buffer,
		NULL,
		0
	);
	if (g_dispDibBitmap == NULL || p_buffer->m_buffer == NULL) {
		DebugPrint("DispDib CreateDIBSection failed\n");
		return 2;
	}

	g_dispDibStretchBitmap = CreateDIBSection(
		g_dispDibStretchDc,
		(BITMAPINFO*) &g_bitmapInfo,
		DIB_RGB_COLORS,
		(void**) &g_dispDibStretchBits,
		NULL,
		0
	);
	if (g_dispDibStretchBitmap == NULL || g_dispDibStretchBits == NULL) {
		DebugPrint("DispDib CreateDIBSection2 failed\n");
		return 2;
	}

	SelectObject(g_dispDibDc, g_dispDibBitmap);
	SelectObject(g_dispDibStretchDc, g_dispDibStretchBitmap);
	ReleaseDC(g_dispDibWindow, g_dispDibWindowDc);

	p_buffer->m_xMax = p_width - 1;
	p_buffer->m_yMax = p_height - 1;
	p_buffer->m_shadow = 0;
	p_buffer->m_bitmapInfo = &g_bitmapInfo;
	g_dibBits = p_buffer->m_buffer;
	g_dispDibInitialized = TRUE;

	if (DispDibSetPalette(0, 0x100, g_paletteColors, TRUE)) {
		DispDibEnd();
		return 1;
	}

	g_dispDibResult = DisplayDibWindowBegin(g_dispDibWindow);
	if (g_dispDibResult) {
		DispDibEnd();
		DebugPrint("DisplayDibWindowBegin err: %hd\n", g_dispDibResult);
		return 1;
	}

	if (DispDibFlip()) {
		DispDibEnd();
		return 1;
	}

	return 0;
}

// FUNCTION: MW2SHELL 0x1002f1fb
MechS32 DispDibEnd()
{
	MechS32 result;

	g_dispDibInitialized = FALSE;

	if (g_dispDibWindow != NULL) {
		DeleteDC(g_dispDibDc);
		DeleteDC(g_dispDibStretchDc);
		DeleteObject(g_dispDibBitmap);
		DeleteObject(g_dispDibStretchBitmap);

		result = DisplayDibWindowEnd(g_dispDibWindow);
		DisplayDibWindowClose(g_dispDibWindow);
		if (result) {
			DebugPrint("DisplayDibWindowEnd err: %hd\n", result);
		}

		g_dispDibWindow = NULL;
		g_dispDibWindowDc = g_dispDibDc = g_dispDibStretchDc = NULL;
		g_dispDibBitmap = g_dispDibStretchBitmap = NULL;
		g_refreshModeBuffer->m_buffer = g_dibBits = g_dispDibStretchBits = NULL;
	}

	return 0;
}

// FUNCTION: MW2SHELL 0x1002f2f5
MechS32 DispDibFlip()
{
	g_dispDibResult = DisplayDibWindowDraw(g_dispDibWindow, c_dispDibDrawFlags, g_dibBits, g_refreshModePixelCount);
	if (g_dispDibResult) {
		DebugPrint("DisplayDibWindowDraw err: %hd\n", g_dispDibResult);
	}

	g_refreshModeBuffer->m_buffer = NULL;
	return g_dispDibResult;
}

// DisplayDib can't draw part of the screen: the rectangle is ignored and the whole frame drawn.
// FUNCTION: MW2SHELL 0x1002f395
MechS32 DispDibBlitRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	g_dispDibResult = DisplayDibWindowDraw(g_dispDibWindow, c_dispDibDrawFlags, g_dibBits, g_refreshModePixelCount);
	if (g_dispDibResult) {
		DebugPrint("DisplayDibWindowDraw err: %hd\n", g_dispDibResult);
	}

	g_refreshModeBuffer->m_buffer = NULL;
	return g_dispDibResult;
}

// FUNCTION: MW2SHELL 0x1002f435
MechS32 DispDibStretchBlit(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	g_dispDibResult = StretchBlt(
		g_dispDibStretchDc,
		0,
		0,
		g_refreshModeWidth,
		g_refreshModeHeight,
		g_dispDibDc,
		p_left,
		p_top,
		p_right - p_left + 1,
		p_bottom - p_top + 1,
		SRCCOPY
	);
	if (!g_dispDibResult) {
		g_dispDibResult = GetLastError();
		DebugPrint("StretchBlt err: %d\n", g_dispDibResult);
	}

	g_dispDibResult =
		DisplayDibWindowDraw(g_dispDibWindow, c_dispDibDrawFlags, g_dispDibStretchBits, g_refreshModePixelCount);
	if (g_dispDibResult) {
		DebugPrint("DISPDIB_StretchBlit DisplayDibWindowDraw err: %hd\n", g_dispDibResult);
	}

	g_refreshModeBuffer->m_buffer = NULL;
	return g_dispDibResult;
}

// FUNCTION: MW2SHELL 0x1002f544
MechS32 DispDibAcquireFramebuffer()
{
	g_refreshModeBuffer->m_buffer = g_dibBits;
	return 0;
}

// FUNCTION: MW2SHELL 0x1002f563
MechS32 DispDibSetPalette(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors)
{
	MechS32 i;

	if (!g_dispDibInitialized) {
		return -1;
	}

	if (p_palette == NULL || p_first < 0 || p_first > 0xff || p_count <= 0 || p_count > 0x100 - p_first) {
		return -1;
	}

	for (i = 0; i < p_count; i++) {
		g_paletteColors[p_first + i] = p_palette[i];
		g_bitmapInfo.m_colors[p_first + i].rgbRed = p_palette[i].m_red * 4;
		g_bitmapInfo.m_colors[p_first + i].rgbGreen = p_palette[i].m_green * 4;
		g_bitmapInfo.m_colors[p_first + i].rgbBlue = p_palette[i].m_blue * 4;
	}

	g_dispDibResult = DisplayDibWindowSetFmt(g_dispDibWindow, &g_bitmapInfo);
	if (g_dispDibResult) {
		DebugPrint("DisplayDibWindowSetFmt err: %d\n", g_dispDibResult);
		return -1;
	}

	return 0;
}

// FUNCTION: MW2SHELL 0x1002f6fd
MechS32 DispDibSetPaletteWithBrightness(PaletteColor* p_palette)
{
	MechS32 i;

	if (p_palette == NULL) {
		return -1;
	}

	for (i = 0; i < 0x100; i++) {
		g_paletteColorsPreBrightness[i] = p_palette[i];
		CopyPaletteColorWithBrightness(&p_palette[i], &g_paletteColors[i]);
		g_bitmapInfo.m_colors[i].rgbRed = g_paletteColors[i].m_red * 4;
		g_bitmapInfo.m_colors[i].rgbGreen = g_paletteColors[i].m_green * 4;
		g_bitmapInfo.m_colors[i].rgbBlue = g_paletteColors[i].m_blue * 4;
	}

	g_dispDibResult = DisplayDibWindowSetFmt(g_dispDibWindow, &g_bitmapInfo);
	if (g_dispDibResult) {
		DebugPrint("DisplayDibWindowSetFmt err: %d\n", g_dispDibResult);
		return -1;
	}

	return 0;
}

// Fades from the current palette to p_palette in p_steps steps of 16 ms, at 8 bits per component.
// FUNCTION: MW2SHELL 0x1002f859
MechS32 DispDibBlendPalettes(PaletteColor* p_palette, MechS32 p_steps)
{
	MechS32 i;
	MechS32 j;
	MechDouble deltas[0x100][3];

	if (p_palette == NULL) {
		return -1;
	}

	for (i = 0; i < 0x100; i++) {
		deltas[i][0] = (MechDouble) ((p_palette[i].m_red - g_paletteColors[i].m_red) * 4) / p_steps;
		deltas[i][1] = (MechDouble) ((p_palette[i].m_green - g_paletteColors[i].m_green) * 4) / p_steps;
		deltas[i][2] = (MechDouble) ((p_palette[i].m_blue - g_paletteColors[i].m_blue) * 4) / p_steps;
	}

	i = p_steps;
	while (i--) {
		j = 0x100;
		while (j--) {
			g_bitmapInfo.m_colors[j].rgbRed = (MechU8) ((p_steps - i) * deltas[j][0]) + g_paletteColors[j].m_red * 4;
			g_bitmapInfo.m_colors[j].rgbGreen =
				(MechU8) ((p_steps - i) * deltas[j][1]) + g_paletteColors[j].m_green * 4;
			g_bitmapInfo.m_colors[j].rgbBlue = (MechU8) ((p_steps - i) * deltas[j][2]) + g_paletteColors[j].m_blue * 4;
			if (i == 0) {
				g_paletteColors[j].m_red = g_bitmapInfo.m_colors[j].rgbRed >> 2;
				g_paletteColors[j].m_green = g_bitmapInfo.m_colors[j].rgbGreen >> 2;
				g_paletteColors[j].m_blue = g_bitmapInfo.m_colors[j].rgbBlue >> 2;
			}
		}

		g_dispDibResult = DisplayDibWindowSetFmt(g_dispDibWindow, &g_bitmapInfo);
		if (g_dispDibResult) {
			DebugPrint("DisplayDibWindowSetFmt err: %d\n", g_dispDibResult);
			return -1;
		}

		Sleep(16);
	}

	return 0;
}
