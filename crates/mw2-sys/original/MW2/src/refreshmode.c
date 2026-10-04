#include "refreshmode.h"

#include "debugprint.h"
#include "decomp.h"
#include "directdraw.h"
#include "dispdibmode.h"
#include "displaybackend.h"
#include "drawbitmapinfo.h"
#include "gdi.h"
#include "menu.h"
#include "mouse.h"
#include "palettecolor.h"
#include "simmain.h"
#include "ticks.h"
#include "types.h"
#include "window.h"

#include <windows.h>

// The refresh mode manager, the simulator's copy of the shell's. The DirectDraw back end lives in
// directdraw.c, the DisplayDib one in dispdib.c, the GDI one in gdi.c.

// Indexed by DisplayBackend::m_id.
// GLOBAL: MW2 0x100b1748
DisplayBackend* g_displayBackends[3] = {&g_directDrawBackend, &g_dispDibBackend, &g_gdiBackend};

// GLOBAL: MW2 0x100b1758
RefreshMode* g_refreshModes[6] = {
	&g_ddrawFlipRefreshMode,
	&g_ddrawBlitFlipRefreshMode,
	&g_ddrawVideoMemoryRefreshMode,
	&g_ddrawSystemMemoryRefreshMode,
	&g_dispDibRefreshMode,
	&g_gdiRefreshMode,
};

// GLOBAL: MW2 0x100b1770
DisplayBackend* g_currentDisplayBackend = NULL;

// GLOBAL: MW2 0x100b1774
RefreshMode* g_currentRefreshMode = NULL;

// The fastest refresh mode found by the profiling, the one to return to from windowed mode.
// GLOBAL: MW2 0x100b1778
RefreshMode* g_fastestRefreshMode = NULL;

// GLOBAL: MW2 0x100b177c
MechS32 g_refreshModeFallback = FALSE;

// The last refresh mode the profiling tries.
// GLOBAL: MW2 0x100b1780
MechS32 g_lastProfiledRefreshMode = 4;

// The buffer the active refresh mode renders into.
// GLOBAL: MW2 0x100b1784
WINDOW* g_refreshModeBuffer = NULL;

// GLOBAL: MW2 0x100b1788
PaletteColor g_paletteColors[0x100] = {0};

// The DIB bits of the GDI and DisplayDib back ends, restored into g_refreshModeBuffer by
// m_acquireFramebuffer.
// GLOBAL: MW2 0x100b1a88
undefined* g_dibBits = NULL;

// GLOBAL: MW2 0x100b1aa4
MechS32 g_windowMode = 0;

// GLOBAL: MW2 0x100b1aa8
MechS32 g_refreshModeInactive = 1;

// GLOBAL: MW2 0x100b1aac
MechS32 g_profileFrame = 0;

// GLOBAL: MW2 0x100bf1c8
static LARGE_INTEGER g_profileStart;

// The window's position and size in windowed mode (SetWindowPos arguments, not corners).
// GLOBAL: MW2 0x100c2890
RECT g_windowedRect;

// GLOBAL: MW2 0x100c28a0
DrawBitmapInfo g_bitmapInfo;

// The frame's size in pixels, p_width * p_height of InitRefreshMode.
// GLOBAL: MW2 0x100c2cc8
MechS32 g_refreshModePixelCount;

// GLOBAL: MW2 0x100c2ce4
MechS32 g_refreshModeWidth;

// GLOBAL: MW2 0x100c2ce8
MechS32 g_refreshModeHeight;

// Switches to refresh mode p_mode (-1: the first), falling through the later modes while a mode
// is unavailable if p_allowFallback is set. The window covers the screen unless p_width x
// p_height is smaller.
// Stack-slot permutation: mode, backend, screenWidth, screenHeight and unused.
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
	MechS32 unused;
	MechS32 screenHeight;
	RefreshMode* mode;
	DisplayBackend* backend;
	MechS32 screenWidth;
	RECT rect;

	if (p_mode == -1) {
		mode = g_refreshModes[0];
	}
	else if (p_mode >= 0 && p_mode < 6) {
		mode = g_refreshModes[p_mode];
	}
	else {
		return 0;
	}

	backend = g_displayBackends[mode->m_backend];
	g_refreshModeFallback = p_allowFallback;
	screenWidth = GetSystemMetrics(SM_CXSCREEN);
	screenHeight = GetSystemMetrics(SM_CYSCREEN);

	g_windowedRect.left = 0;
	g_windowedRect.top = 0;
	g_windowedRect.right = p_width;
	g_windowedRect.bottom = p_height;
	if (screenWidth <= p_width && screenHeight <= p_height) {
		g_gdiBackend.m_style = WS_POPUP;
		g_windowMode = c_windowModeFullscreen;
	}
	else {
		g_gdiBackend.m_style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		AdjustWindowRect(&g_windowedRect, g_gdiBackend.m_style, p_menu);
		g_windowedRect.right -= g_windowedRect.left;
		g_windowedRect.bottom -= g_windowedRect.top;
		g_windowedRect.top = (screenHeight - g_windowedRect.bottom) / 2;
		g_windowedRect.left = (screenWidth - g_windowedRect.right) / 2;
		g_windowMode = backend->m_windowMode;
	}

	if (backend->m_windowMode == c_windowModeFullscreen) {
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYSCREEN);
		unused = 8;
	}
	else {
		rect = g_windowedRect;
		unused = 0;
	}

	if (g_refreshModeInactive) {
		g_refreshModeWidth = p_width;
		g_refreshModeHeight = p_height;
		AdjustWindowSize(backend);
		g_currentDisplayBackend = backend;
	}
	else if (mode != g_currentRefreshMode || g_refreshModeWidth != p_width || g_refreshModeHeight != p_height) {
		AdjustWindowSize(backend);
		g_currentRefreshMode->m_end();
	}

	g_currentRefreshMode = mode;
	g_refreshModeWidth = p_width;
	g_refreshModeHeight = p_height;
	g_refreshModePixelCount = p_height * p_width;
	g_refreshModeBuffer = p_buffer;

	while (g_currentRefreshMode->m_available && g_currentRefreshMode->m_begin(p_buffer, p_width, p_height)) {
		DebugPrint("RefreshMode %d not available\n", g_currentRefreshMode->m_index);
		g_currentRefreshMode->m_available = FALSE;
		if (!g_refreshModeFallback || g_currentRefreshMode->m_index == 5) {
			return 0;
		}

		g_currentRefreshMode = g_refreshModes[g_currentRefreshMode->m_index + 1];
	}

	if (g_refreshModeInactive && backend->m_id != c_displayBackendGdi) {
		ShowWindow(g_gameWindow, SW_SHOWDEFAULT);
		UpdateWindow(g_gameWindow);
	}

	g_refreshModeInactive = 0;
	return 1;
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

	g_refreshModeInactive = 1;
}

// Called once a frame while the refresh modes are profiled: times four frames of the current mode,
// then moves to the next one; after the last, SelectFastestRefreshMode picks the fastest.
// Operand order: the original compares the high parts as `cmp [g_profileStart+4], eax` with
// end.HighPart in eax; swapping the operands doesn't flip it.
// FUNCTION: MW2 0x100770a8
void ProfileRefreshModes(void)
{
	LARGE_INTEGER end;

	if (g_currentRefreshMode->m_index > g_lastProfiledRefreshMode) {
		g_refreshModeFallback = FALSE;
	}
	else if (g_profileFrame == 1) {
		if (!QueryPerformanceCounter(&g_profileStart)) {
			g_refreshModeFallback = FALSE;
			return;
		}

		g_profileFrame++;
	}
	else if (g_profileFrame == 5) {
		if (!QueryPerformanceCounter(&end)) {
			g_refreshModeFallback = FALSE;
			return;
		}

		if (end.HighPart != g_profileStart.HighPart) {
			g_currentRefreshMode->m_profileTime = ~g_profileStart.LowPart + end.LowPart + 1;
		}
		else {
			g_currentRefreshMode->m_profileTime = end.LowPart - g_profileStart.LowPart;
		}

		DebugPrint(
			"Refresh mode %d start=(%u,%d) end=(%u,%d) diff=%u\n",
			g_currentRefreshMode->m_index,
			g_profileStart.LowPart,
			g_profileStart.HighPart,
			end.LowPart,
			end.HighPart,
			g_currentRefreshMode->m_profileTime
		);
		if (g_currentRefreshMode->m_index == g_lastProfiledRefreshMode) {
			g_refreshModeFallback = FALSE;
		}
		else {
			while (g_currentRefreshMode->m_index < g_lastProfiledRefreshMode) {
				g_currentRefreshMode->m_end();
				g_currentRefreshMode = g_refreshModes[g_currentRefreshMode->m_index + 1];
				if (g_currentRefreshMode->m_available &&
					!g_currentRefreshMode->m_begin(g_refreshModeBuffer, g_refreshModeWidth, g_refreshModeHeight)) {
					g_profileFrame = 0;
					break;
				}
				else {
					DebugPrint("Refresh mode %d not available\n", g_currentRefreshMode->m_index);
					g_currentRefreshMode->m_available = FALSE;
					if (g_currentRefreshMode->m_index == g_lastProfiledRefreshMode) {
						g_refreshModeFallback = FALSE;
					}
				}
			}
		}

		if (!g_refreshModeFallback) {
			SelectFastestRefreshMode();
		}
	}
	else {
		g_profileFrame++;
	}
}

// Switches to the available refresh mode with the shortest profile time.
// FUNCTION: MW2 0x100772a4
void SelectFastestRefreshMode(void)
{
	MechS32 i;
	RefreshMode* best;

	best = NULL;
	for (i = 0; i < 6; i++) {
		if (g_refreshModes[i]->m_available && g_refreshModes[i]->m_profileTime > 0 &&
			(best == NULL || g_refreshModes[i]->m_profileTime < best->m_profileTime)) {
			best = g_refreshModes[i];
		}
	}

	if (best != g_currentRefreshMode) {
		g_currentRefreshMode->m_end();
		g_currentRefreshMode = best;
		g_currentRefreshMode->m_begin(g_refreshModeBuffer, g_refreshModeWidth, g_refreshModeHeight);
	}

	g_fastestRefreshMode = g_currentRefreshMode;
	DebugPrint(
		"Refresh mode %d selected with profile time: %u\n",
		g_currentRefreshMode->m_index,
		g_currentRefreshMode->m_profileTime
	);
}

// Switches between fullscreen and the first available windowed refresh mode, or back to the
// fullscreen mode last selected. Unlike the shell's, it does nothing while the frame covers the
// whole screen, unless g_shouldToggleFullscreen is set.
// Stack-slot permutation: i, mode and backend.
// FUNCTION: MW2 0x10077392
void ToggleFullScreen(void)
{
	MechS32 i;
	RefreshMode* mode;
	DisplayBackend* backend;

	if (g_refreshModeFallback) {
		return;
	}

	if (!g_shouldToggleFullscreen && g_refreshModeWidth >= g_desktopWidth && g_refreshModeHeight >= g_desktopHeight) {
		return;
	}

	DebugPrint("ToggleFullScreen(1): pause_timer(TRUE)");
	PauseTimer(0x80, TRUE);

	if (g_currentDisplayBackend->m_windowMode == c_windowModeFullscreen) {
		for (i = 0; i < 6; i++) {
			mode = g_refreshModes[i];
			backend = g_displayBackends[mode->m_backend];
			if (mode->m_available && backend->m_windowMode == c_windowModeWindowed) {
				break;
			}
		}

		if (i == 6) {
			ShowMessage("MechWarrior2 cannot run in a window in the current resolution on your video hardware");
			if (!g_simPaused) {
				DebugPrint("ToggleFullScreen(2): pause_timer(FALSE)");
				PauseTimer(0x80, FALSE);
			}
			return;
		}
	}
	else {
		if (!g_fastestRefreshMode) {
			ShowMessage(
				"MechWarrior2 cannot support full screen mode in the current resolution on your video hardware"
			);
			if (!g_simPaused) {
				DebugPrint("ToggleFullScreen(3): pause_timer(FALSE)");
				PauseTimer(0x80, FALSE);
			}
			return;
		}
		else {
			mode = g_fastestRefreshMode;
		}

		GetWindowRect(g_gameWindow, &g_windowedRect);
		g_windowedRect.right -= g_windowedRect.left;
		g_windowedRect.bottom -= g_windowedRect.top;
	}

	g_currentRefreshMode->m_end();
	g_currentRefreshMode = mode;
	g_currentRefreshMode->m_begin(g_refreshModeBuffer, g_refreshModeWidth, g_refreshModeHeight);
	g_currentDisplayBackend->m_setPalette(0, 0x100, g_paletteColors, TRUE);
	g_reclipCursor = 1;
	if (!g_simPaused) {
		DebugPrint("ToggleFullScreen(4): pause_timer(FALSE)");
		PauseTimer(0x80, FALSE);
	}
}

// A top-down 8-bit DIB of p_width x p_height.
// FUNCTION: MW2 0x100775b4
void InitBitmapInfo(MechS32 p_width, MechS32 p_height)
{
	g_bitmapInfo.m_header.biSize = sizeof(BITMAPINFOHEADER);
	g_bitmapInfo.m_header.biWidth = p_width;
	g_bitmapInfo.m_header.biHeight = p_height * -1;
	g_bitmapInfo.m_header.biPlanes = 1;
	g_bitmapInfo.m_header.biBitCount = 8;
	g_bitmapInfo.m_header.biCompression = BI_RGB;
	g_bitmapInfo.m_header.biSizeImage = 0;
	g_bitmapInfo.m_header.biClrUsed = 0;
	g_bitmapInfo.m_header.biXPelsPerMeter = 0;
	g_bitmapInfo.m_header.biYPelsPerMeter = 0;
	g_bitmapInfo.m_header.biClrImportant = 0;
}

// Restyles the game window for p_backend's window mode, and sets g_windowMode: a mode that
// covers the whole screen counts as fullscreen.
// FUNCTION: MW2 0x1007762f
void AdjustWindowSize(DisplayBackend* p_backend)
{
	if (p_backend == NULL) {
		return;
	}

	if (p_backend->m_windowMode == c_windowModeWindowed) {
		SetWindowLong(g_gameWindow, GWL_STYLE, p_backend->m_style | WS_VISIBLE);
	}
	else {
		SetWindowLong(g_gameWindow, GWL_STYLE, (p_backend->m_style | WS_VISIBLE) & ~WS_SYSMENU);
	}

	if (p_backend->m_windowMode == c_windowModeWindowed) {
		if (g_currentDisplayBackend != NULL && g_currentDisplayBackend->m_id == c_displayBackendDirectDraw) {
			g_windowedSwitchTime = timeGetTime();
			g_windowedSwitchDeadline = g_windowedSwitchTime + 3000;
			g_windowedSwitchPending = 1;
		}

		SetWindowPos(
			g_gameWindow,
			HWND_NOTOPMOST,
			g_windowedRect.left,
			g_windowedRect.top,
			g_windowedRect.right,
			g_windowedRect.bottom,
			SWP_NOACTIVATE
		);
		if (g_simPaused && !GetMenuSlotState(4)) {
			while (ShowCursor(TRUE) < 0) {
			}
		}
	}
	else if (g_simPaused) {
		while (ShowCursor(FALSE) >= 0) {
		}
	}

	if (GetSystemMetrics(SM_CXSCREEN) <= g_refreshModeWidth && GetSystemMetrics(SM_CYSCREEN) <= g_refreshModeHeight) {
		g_windowMode = c_windowModeFullscreen;
	}
	else {
		g_windowMode = p_backend->m_windowMode;
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
