#include "refreshmode.h"

#include "debugprint.h"
#include "decomp.h"
#include "directdraw.h"
#include "dispdibmode.h"
#include "displaybackend.h"
#include "drawbitmapinfo.h"
#include "gdi.h"
#include "mouse.h"
#include "palettecolor.h"
#include "refreshmode.h"
#include "types.h"
#include "window.h"
#include "windowstate.h"

#include <ddraw.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// The refresh mode manager. The DirectDraw back end lives in directdraw.c, the DisplayDib one in
// dispdib.c, the GDI one in gdi.c.

void SelectFastestRefreshMode();

// The original declares a function the shell never defines: the linker binds the calls to the
// variable of the same name (gdi.c), so they land in BSS. The debug strings call it pause_timer.
void PauseTimer(MechS32 p_flags, MechS32 p_pause);

// Indexed by DisplayBackend::m_id.
// GLOBAL: MW2SHELL 0x10062ca0
DisplayBackend* g_displayBackends[3] = {&g_directDrawBackend, &g_dispDibBackend, &g_gdiBackend};

// GLOBAL: MW2SHELL 0x10062cb0
RefreshMode* g_refreshModes[6] = {
	&g_ddrawFlipRefreshMode,
	&g_ddrawBlitFlipRefreshMode,
	&g_ddrawVideoMemoryRefreshMode,
	&g_ddrawSystemMemoryRefreshMode,
	&g_dispDibRefreshMode,
	&g_gdiRefreshMode,
};

// GLOBAL: MW2SHELL 0x10062cc8
DisplayBackend* g_currentDisplayBackend = NULL;

// GLOBAL: MW2SHELL 0x10062ccc
RefreshMode* g_currentRefreshMode = NULL;

// The fastest refresh mode found by the profiling, the one to return to from windowed mode.
// GLOBAL: MW2SHELL 0x10062cd0
RefreshMode* g_fastestRefreshMode = NULL;

// GLOBAL: MW2SHELL 0x10062cd4
MechS32 g_refreshModeFallback = FALSE;

// The last refresh mode the profiling tries.
// GLOBAL: MW2SHELL 0x10062cd8
MechS32 g_lastProfiledRefreshMode = 4;

// The buffer the active refresh mode renders into.
// GLOBAL: MW2SHELL 0x10062cdc
WINDOW* g_refreshModeBuffer = NULL;

// GLOBAL: MW2SHELL 0x10062ce0
PaletteColor g_paletteColors[0x100] = {0};

// The DIB bits of the GDI and DisplayDib back ends, restored into g_refreshModeBuffer by
// m_acquireFramebuffer.
// GLOBAL: MW2SHELL 0x10062fe0
undefined* g_dibBits = NULL;

// GLOBAL: MW2SHELL 0x10062ffc
MechS32 g_windowMode = 0;

// GLOBAL: MW2SHELL 0x10063000
MechS32 g_refreshModeInactive = 1;

// GLOBAL: MW2SHELL 0x10063004
MechS32 g_profileFrame = 0;

// GLOBAL: MW2SHELL 0x1007cc90
static LARGE_INTEGER g_profileStart;

// When AdjustWindowSize last switched DirectDraw to a window (timeGetTime), with
// g_windowedSwitchDeadline three seconds later and g_windowedSwitchPending set. Nothing reads
// the three.
// GLOBAL: MW2SHELL 0x100965d0
MechU32 g_windowedSwitchTime;

// GLOBAL: MW2SHELL 0x100965e4
MechS32 g_windowedSwitchPending;

// GLOBAL: MW2SHELL 0x100965e8
MechU32 g_windowedSwitchDeadline;

// GLOBAL: MW2SHELL 0x100965ec
HWND g_gameWindow;

// The shell window's position and size in windowed mode (SetWindowPos arguments, not corners).
// GLOBAL: MW2SHELL 0x10096a50
RECT g_windowedRect;

// GLOBAL: MW2SHELL 0x10096a60
DrawBitmapInfo g_bitmapInfo;

// The frame's size in pixels, p_width * p_height of InitRefreshMode.
// GLOBAL: MW2SHELL 0x10096e88
MechS32 g_refreshModePixelCount;

// GLOBAL: MW2SHELL 0x10096e8c
MechS32 g_refreshModeWidth;

// GLOBAL: MW2SHELL 0x10096e90
MechS32 g_refreshModeHeight;

// Switches to refresh mode p_mode (-1: the first), falling through the later modes while a mode
// is unavailable if p_allowFallback is set. The window covers the screen unless p_width x
// p_height is smaller.
// Stack-slot permutation: mode, backend, screenWidth and unused. The original also compares
// the screen size against p_width and p_height the other way round.
// FUNCTION: MW2SHELL 0x10010a30
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

// FUNCTION: MW2SHELL 0x10010d49
void ShutdownRefreshMode()
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
// FUNCTION: MW2SHELL 0x10010d88
void ProfileRefreshModes()
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
// FUNCTION: MW2SHELL 0x10010f83
void SelectFastestRefreshMode()
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
// fullscreen mode last selected.
// Not 100%: the stack slots of i, mode and backend are permuted.
// FUNCTION: MW2SHELL 0x10011071
void ToggleFullScreen()
{
	MechS32 i;
	RefreshMode* mode;
	DisplayBackend* backend;

	if (g_refreshModeFallback) {
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
			if (!g_paused) {
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
			if (!g_paused) {
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
	if (!g_paused) {
		DebugPrint("ToggleFullScreen(4): pause_timer(FALSE)");
		PauseTimer(0x80, FALSE);
	}
}

// A top-down 8-bit DIB of p_width x p_height.
// FUNCTION: MW2SHELL 0x1001125f
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

// Restyles the shell window for p_backend's window mode, and sets g_windowMode: a mode that
// covers the whole screen counts as fullscreen.
// FUNCTION: MW2SHELL 0x100112da
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
		if (g_paused && !FUN_1003bf90(4)) {
			while (ShowCursor(TRUE) < 0) {
			}
		}
	}
	else if (g_paused) {
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
// Operand order: the original compares i < p_count as `cmp [p_count], eax` and adds p_first + i
// with p_first loaded first; the order follows the unit's symbol table.
// FUNCTION: MW2SHELL 0x10011450
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
