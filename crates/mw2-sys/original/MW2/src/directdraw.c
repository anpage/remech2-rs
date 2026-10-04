#include "directdraw.h"

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

#include <ddraw.h>
#include <string.h>
#include <windows.h>

// The DirectDraw back end of the refresh modes (refreshmode.c), the simulator's copy of the
// shell's. Its functions are named after their debug messages, "DDRAW_Flip" and so on: the frame
// is drawn into g_ddrawBuffer, an offscreen surface (or the back buffer when the refresh mode
// flips), then blitted or flipped to the primary surface. Without /Ob1, the __inline helpers
// DdrawUnlock and DdrawRestore aren't expanded as in the shell: the compiler emits them after the
// unit's functions, each in its own 16-byte aligned section.

void DdrawStop(void);
void DdrawDestroySurfaces(void);
MechS32 DdrawCreateSurfaces(MechS32 p_width, MechS32 p_height);
MechS32 DdrawInit(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height);
MechS32 DdrawFlip(void);
MechS32 DdrawBlit(void);
MechS32 DdrawBlitFlip(void);
MechS32 DdrawStretchBlit(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 DdrawStretchBlit320(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 DdrawBlitRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom);
MechS32 DdrawWritePaletteEntries(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors);
MechS32 DdrawWritePaletteGamma(PaletteColor* p_palette);
MechS32 DdrawPaletteFade(PaletteColor* p_palette, MechS32 p_steps);
MechS32 DdrawLockBuffer(void);
__inline static HRESULT DdrawUnlock(void);
__inline static HRESULT DdrawRestore(void);

// GLOBAL: MW2 0x100b1cb0
LPDIRECTDRAW g_ddraw = NULL;

// GLOBAL: MW2 0x100b1cb4
LPDIRECTDRAWSURFACE g_ddrawPrimary = NULL;

// GLOBAL: MW2 0x100b1cb8
LPDIRECTDRAWSURFACE g_ddrawBack = NULL;

// GLOBAL: MW2 0x100b1cbc
LPDIRECTDRAWSURFACE g_ddrawBuffer = NULL;

// The second back buffer of the 320x200 mode, which stretches the frame into it and flips.
// GLOBAL: MW2 0x100b1cc0
LPDIRECTDRAWSURFACE g_ddrawStretch = NULL;

// Never accessed: there is nothing to name it after.
// GLOBAL: MW2 0x100b1cc4
undefined4 g_unk0x100b1cc4 = 0;

// GLOBAL: MW2 0x100b1cc8
LPDIRECTDRAWPALETTE g_ddrawPalette = NULL;

// TRUE while g_ddrawBuffer is locked for drawing.
// GLOBAL: MW2 0x100b1ccc
MechS32 g_ddrawLocked = FALSE;

// GLOBAL: MW2 0x100b1cd0
MechS32 g_ddrawInitialized = FALSE;

// GLOBAL: MW2 0x100b1cd8
DisplayBackend g_directDrawBackend = {
	c_displayBackendDirectDraw,
	c_windowModeFullscreen,
	WS_POPUP,
	DdrawInit,
	(MechS32 (*)()) DdrawStop,
	DdrawWritePaletteEntries,
	DdrawWritePaletteGamma,
	DdrawPaletteFade,
	DdrawLockBuffer,
	0
};

// Draws into the back buffer and flips.
// GLOBAL: MW2 0x100b1d00
RefreshMode g_ddrawFlipRefreshMode = {
	0,
	c_displayBackendDirectDraw,
	1,
	0,
	DdrawInit,
	(MechS32 (*)()) DdrawDestroySurfaces,
	DdrawFlip,
	DdrawBlitRect,
	DdrawStretchBlit
};

// Draws into an offscreen surface in system memory, blits it to the back buffer and flips.
// GLOBAL: MW2 0x100b1d28
RefreshMode g_ddrawBlitFlipRefreshMode = {
	1,
	c_displayBackendDirectDraw,
	1,
	0,
	DdrawInit,
	(MechS32 (*)()) DdrawDestroySurfaces,
	DdrawBlitFlip,
	DdrawBlitRect,
	DdrawStretchBlit
};

// Draws into an offscreen surface in video memory and blits it to the primary surface.
// GLOBAL: MW2 0x100b1d50
RefreshMode g_ddrawVideoMemoryRefreshMode = {
	2,
	c_displayBackendDirectDraw,
	1,
	0,
	DdrawInit,
	(MechS32 (*)()) DdrawDestroySurfaces,
	DdrawBlit,
	DdrawBlitRect,
	DdrawStretchBlit
};

// Draws into an offscreen surface in system memory and blits it to the primary surface.
// GLOBAL: MW2 0x100b1d78
RefreshMode g_ddrawSystemMemoryRefreshMode = {
	3,
	c_displayBackendDirectDraw,
	1,
	0,
	DdrawInit,
	(MechS32 (*)()) DdrawDestroySurfaces,
	DdrawBlit,
	DdrawBlitRect,
	DdrawStretchBlit
};

// GLOBAL: MW2 0x100c26b0
RECT g_ddrawScreenRect;

// GLOBAL: MW2 0x100c26c0
HRESULT g_ddrawResult;

// GLOBAL: MW2 0x100c26d0
DDBLTFX g_ddrawBltFx;

// GLOBAL: MW2 0x100c2740
DDSURFACEDESC g_ddrawPrimaryDesc;

// GLOBAL: MW2 0x100c27b0
DDSURFACEDESC g_ddrawBackDesc;

// GLOBAL: MW2 0x100c281c
DDSCAPS g_ddrawCaps;

// GLOBAL: MW2 0x100c2820
DDSURFACEDESC g_ddrawBufferDesc;

// FUNCTION: MW2 0x10077850
void DdrawStop(void)
{
	g_ddrawInitialized = FALSE;
	if (g_ddraw != NULL) {
		DdrawDestroySurfaces();
		IDirectDraw_SetCooperativeLevel(g_ddraw, g_gameWindow, DDSCL_NORMAL);
		IDirectDraw_RestoreDisplayMode(g_ddraw);
		if (g_ddrawPalette != NULL) {
			IDirectDrawPalette_Release(g_ddrawPalette);
			g_ddrawPalette = NULL;
		}
		IDirectDraw_Release(g_ddraw);
		g_ddraw = NULL;
	}
}

// FUNCTION: MW2 0x100778e0
void DdrawDestroySurfaces(void)
{
	g_refreshModeBuffer->m_buffer = NULL;
	if (g_ddrawPrimary != NULL) {
		IDirectDrawSurface_Release(g_ddrawPrimary);
		g_ddrawPrimary = NULL;
		g_ddrawBack = NULL;
		g_ddrawStretch = NULL;
		if (g_currentRefreshMode->m_index == 0) {
			g_ddrawBuffer = NULL;
		}
	}

	if (g_ddrawBuffer != NULL && (g_refreshModeWidth != 320 || g_refreshModeHeight != 200)) {
		IDirectDrawSurface_Release(g_ddrawBuffer);
		g_ddrawBuffer = NULL;
	}
}

// Creates the primary surface and g_ddrawBuffer: the 320x200 mode flips through two back
// buffers, the flipping modes (0 and 1) through one, and the blitting modes (2 and 3) draw into
// an offscreen surface.
// FUNCTION: MW2 0x10077990
MechS32 DdrawCreateSurfaces(MechS32 p_width, MechS32 p_height)
{
	MechS32 flip = FALSE;

	if (p_width == 320 && p_height == 200) {
		if (g_currentRefreshMode->m_index == 2 || g_currentRefreshMode->m_index == 3) {
			return -1;
		}

		g_ddrawPrimaryDesc.dwSize = sizeof(DDSURFACEDESC);
		g_ddrawPrimaryDesc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		g_ddrawPrimaryDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
		g_ddrawPrimaryDesc.dwBackBufferCount = 2;
		g_ddrawResult = IDirectDraw_CreateSurface(g_ddraw, &g_ddrawPrimaryDesc, &g_ddrawPrimary, NULL);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_CreateSurfaces CreateSurface(Primary): %d\n", g_ddrawResult & 0xfff);
			return g_ddrawResult;
		}

		g_ddrawCaps.dwCaps = DDSCAPS_BACKBUFFER;
		g_ddrawResult = IDirectDrawSurface_GetAttachedSurface(g_ddrawPrimary, &g_ddrawCaps, &g_ddrawBack);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_CreateSurfaces GetAttachedSurface(Back): %d\n", g_ddrawResult & 0xfff);
			return g_ddrawResult;
		}

		g_ddrawBuffer = g_ddrawBack;
		g_ddrawCaps.dwCaps = DDSCAPS_FLIP;
		g_ddrawResult = IDirectDrawSurface_GetAttachedSurface(g_ddrawPrimary, &g_ddrawCaps, &g_ddrawStretch);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_CreateSurfaces GetAttachedSurface(Stretch): %d\n", g_ddrawResult & 0xfff);
			return g_ddrawResult;
		}
	}
	else {
		if (g_currentRefreshMode->m_index == 0 || g_currentRefreshMode->m_index == 1) {
			flip = TRUE;
		}

		g_ddrawPrimaryDesc.dwSize = sizeof(DDSURFACEDESC);
		g_ddrawPrimaryDesc.dwFlags = DDSD_CAPS;
		g_ddrawPrimaryDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;
		if (flip) {
			g_ddrawPrimaryDesc.dwFlags |= DDSD_BACKBUFFERCOUNT;
			g_ddrawPrimaryDesc.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX;
			g_ddrawPrimaryDesc.dwBackBufferCount = 2;
		}

		g_ddrawResult = IDirectDraw_CreateSurface(g_ddraw, &g_ddrawPrimaryDesc, &g_ddrawPrimary, NULL);
		if (g_ddrawResult != DD_OK) {
			if (flip) {
				DebugPrint("DDRAW_CreateSurfaces primary failed; trying 1 back buffer\n");
				g_ddrawPrimaryDesc.dwBackBufferCount = 1;
				g_ddrawResult = IDirectDraw_CreateSurface(g_ddraw, &g_ddrawPrimaryDesc, &g_ddrawPrimary, NULL);
			}
			else if (g_currentRefreshMode->m_index == 3) {
				DebugPrint("DDRAW_CreateSurfaces primary failed; trying system memory\n");
				g_ddrawPrimaryDesc.ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
				g_ddrawResult = IDirectDraw_CreateSurface(g_ddraw, &g_ddrawPrimaryDesc, &g_ddrawPrimary, NULL);
			}
		}

		if (g_ddrawResult != DD_OK) {
			return g_ddrawResult;
		}

		if (flip) {
			g_ddrawCaps.dwCaps = DDSCAPS_BACKBUFFER;
			g_ddrawResult = IDirectDrawSurface_GetAttachedSurface(g_ddrawPrimary, &g_ddrawCaps, &g_ddrawBack);
			if (g_ddrawResult != DD_OK) {
				DebugPrint("DDRAW_CreateSurfaces GetAttachedSurface(): %d\n", g_ddrawResult & 0xfff);
				return g_ddrawResult;
			}
		}

		if (g_currentRefreshMode->m_index == 0) {
			g_ddrawBuffer = g_ddrawBack;
		}
		else {
			g_ddrawBufferDesc.dwSize = sizeof(DDSURFACEDESC);
			g_ddrawBufferDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
			g_ddrawBufferDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
			g_ddrawBufferDesc.dwWidth = p_width;
			g_ddrawBufferDesc.dwHeight = p_height;
			if (g_currentRefreshMode->m_index == 2) {
				g_ddrawBufferDesc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
			}
			else {
				g_ddrawBufferDesc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
			}

			g_ddrawResult = IDirectDraw_CreateSurface(g_ddraw, &g_ddrawBufferDesc, &g_ddrawBuffer, NULL);
			if (g_ddrawResult != DD_OK) {
				DebugPrint("DDRAW_CreateSurfaces CreateSurface(offscreen): %d\n", g_ddrawResult & 0xfff);
			}
		}
	}

	return g_ddrawResult;
}

// Stack-slot permutation: caps and entries.
// FUNCTION: MW2 0x10077d75
MechS32 DdrawInit(WINDOW* p_buffer, MechS32 p_width, MechS32 p_height)
{
	LPPALETTEENTRY entries;
	DDCAPS caps;

	if (g_currentDisplayBackend->m_id != c_displayBackendDirectDraw) {
		g_currentDisplayBackend->m_end();
		g_currentDisplayBackend = g_displayBackends[c_displayBackendDirectDraw];
		if (g_currentDisplayBackend->m_windowMode != g_windowMode) {
			AdjustWindowSize(g_currentDisplayBackend);
		}
	}

	if (g_ddraw == NULL) {
		g_ddrawResult = DirectDrawCreate(NULL, &g_ddraw, NULL);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_Init DirectDrawCreate(): %d\n", g_ddrawResult & 0xfff);
			return 1;
		}

		g_ddrawResult =
			IDirectDraw_SetCooperativeLevel(g_ddraw, g_gameWindow, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | 0x40);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_Init SetCooperativeLevel(): %d\n", g_ddrawResult & 0xfff);
			DdrawStop();
			return 1;
		}

		g_ddrawResult = IDirectDraw_SetDisplayMode(g_ddraw, p_width, p_height, 8);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_Init SetDisplayMode(): %d\n", g_ddrawResult & 0xfff);
			DdrawStop();
			return 1;
		}
	}

	memset(&caps, 0, sizeof(caps));
	caps.dwSize = sizeof(caps);
	if (IDirectDraw_GetCaps(g_ddraw, &caps, NULL) == DD_OK && (caps.dwCaps & DDCAPS_BANKSWITCHED) &&
		(g_currentRefreshMode->m_index == 0 || g_currentRefreshMode->m_index == 2)) {
		DebugPrint("DDRAW_Init: bank-switched video card.\n");
		return 1;
	}

	if (DdrawCreateSurfaces(p_width, p_height)) {
		DdrawDestroySurfaces();
		return 1;
	}

	g_ddrawBltFx.dwSize = sizeof(DDBLTFX);
	if (DdrawFill(0, 0, p_width - 1, p_height - 1, 0)) {
		DdrawDestroySurfaces();
		return 1;
	}

	if (g_currentRefreshMode->m_flip()) {
		DdrawDestroySurfaces();
		return 1;
	}

	memset(&g_ddrawPrimaryDesc, 0, sizeof(DDSURFACEDESC));
	g_ddrawPrimaryDesc.dwSize = sizeof(DDSURFACEDESC);
	g_ddrawResult = IDirectDrawSurface_GetSurfaceDesc(g_ddrawPrimary, &g_ddrawPrimaryDesc);
	memset(&g_ddrawBufferDesc, 0, sizeof(DDSURFACEDESC));
	g_ddrawBufferDesc.dwSize = sizeof(DDSURFACEDESC);
	g_ddrawResult = IDirectDrawSurface_GetSurfaceDesc(g_ddrawBuffer, &g_ddrawBufferDesc);
	if (g_ddrawBack != NULL) {
		memset(&g_ddrawBackDesc, 0, sizeof(DDSURFACEDESC));
		g_ddrawBackDesc.dwSize = sizeof(DDSURFACEDESC);
		g_ddrawResult = IDirectDrawSurface_GetSurfaceDesc(g_ddrawBack, &g_ddrawBackDesc);
	}

	g_ddrawScreenRect.left = 0;
	g_ddrawScreenRect.top = 0;
	g_ddrawScreenRect.right = p_width;
	g_ddrawScreenRect.bottom = p_height;

	p_buffer->m_buffer = NULL;
	p_buffer->m_xMax = g_ddrawBackDesc.lPitch - 1;
	p_buffer->m_yMax = p_height - 1;
	p_buffer->m_shadow = 0;
	p_buffer->m_bitmapInfo = &g_bitmapInfo;

	if (g_ddrawPalette == NULL) {
		if ((entries = MechHeapAllocZeroed(g_primaryHeap, 0x100 * sizeof(PALETTEENTRY))) !=
			NULL) {
			g_ddrawResult =
				IDirectDraw_CreatePalette(g_ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256, entries, &g_ddrawPalette, NULL);
			if (g_ddrawResult != DD_OK) {
				DebugPrint("DDraw CreatePalette(): %d\n", g_ddrawResult & 0xfff);
				DdrawStop();
				return -1;
			}
		}
		else {
			return -1;
		}
	}

	g_ddrawResult = IDirectDrawSurface_SetPalette(g_ddrawPrimary, g_ddrawPalette);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDraw SetPalette(Primary): %d\n", g_ddrawResult & 0xfff);
	}

	if (p_width == 320 && p_height == 200) {
		g_currentRefreshMode->m_stretchBlit = DdrawStretchBlit320;
	}

	g_ddrawInitialized = TRUE;
	return 0;
}

// FUNCTION: MW2 0x100781c0
MechS32 DdrawFlip(void)
{
	g_ddrawResult = DdrawUnlock();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_Flip Unlock(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = DdrawRestore();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_Flip Restore(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = IDirectDrawSurface_Flip(g_ddrawPrimary, NULL, DDFLIP_WAIT);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_Flip Flip(): %d\n", g_ddrawResult & 0xfff);
	}

	return g_ddrawResult == DD_OK ? 0 : -1;
}

// FUNCTION: MW2 0x10078285
MechS32 DdrawBlit(void)
{
	g_ddrawResult = DdrawUnlock();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_Blit Unlock(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = DdrawRestore();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_Blit Restore(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = IDirectDrawSurface_BltFast(g_ddrawPrimary, 0, 0, g_ddrawBuffer, NULL, DDBLTFAST_WAIT);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_Blit BltFast(): %d\n", g_ddrawResult & 0xfff);
	}

	return g_ddrawResult == DD_OK ? 0 : -1;
}

// FUNCTION: MW2 0x10078354
MechS32 DdrawBlitFlip(void)
{
	g_ddrawResult = DdrawUnlock();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_BlitFlip Unlock(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = DdrawRestore();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_BlitFlip Restore(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = IDirectDrawSurface_BltFast(g_ddrawBack, 0, 0, g_ddrawBuffer, NULL, DDBLTFAST_WAIT);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_BlitFlip BltFast(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = IDirectDrawSurface_Flip(g_ddrawPrimary, NULL, DDFLIP_WAIT);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_BlitFlip Flip(): %d\n", g_ddrawResult & 0xfff);
	}

	return g_ddrawResult == DD_OK ? 0 : -1;
}

// FUNCTION: MW2 0x10078461
MechS32 DdrawStretchBlit(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	RECT rect;

	g_ddrawResult = DdrawUnlock();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_StretchBlit Unlock(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = DdrawRestore();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_StretchBlit Restore(): %d\n", g_ddrawResult & 0xfff);
	}

	rect.left = p_left;
	rect.top = p_top;
	rect.right = p_right;
	rect.bottom = p_bottom;
	g_ddrawResult = IDirectDrawSurface_Blt(g_ddrawPrimary, &g_ddrawScreenRect, g_ddrawBuffer, &rect, DDBLT_WAIT, NULL);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_StretchBlit Blt(): %d\n", g_ddrawResult & 0xfff);
	}

	return g_ddrawResult == DD_OK ? 0 : -1;
}

// FUNCTION: MW2 0x10078553
MechS32 DdrawStretchBlit320(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	RECT rect;

	g_ddrawResult = DdrawUnlock();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_StretchBlit320 Unlock(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = DdrawRestore();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_StretchBlit320 Restore(): %d\n", g_ddrawResult & 0xfff);
	}

	rect.left = p_left;
	rect.top = p_top;
	rect.right = p_right;
	rect.bottom = p_bottom;
	g_ddrawResult = IDirectDrawSurface_Blt(g_ddrawStretch, &g_ddrawScreenRect, g_ddrawBuffer, &rect, DDBLT_WAIT, NULL);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_StretchBlit320 Blt(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = IDirectDrawSurface_Flip(g_ddrawPrimary, g_ddrawStretch, DDFLIP_WAIT);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_Flip Flip(): %d\n", g_ddrawResult & 0xfff);
	}

	return g_ddrawResult == DD_OK ? 0 : -1;
}

// FUNCTION: MW2 0x10078687
MechS32 DdrawBlitRect(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom)
{
	RECT rect;

	if (g_currentRefreshMode->m_index == 0) {
		return DdrawFlip();
	}

	g_ddrawResult = DdrawUnlock();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_BlitRect Unlock(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = DdrawRestore();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_BlitRect Restore(): %d\n", g_ddrawResult & 0xfff);
	}

	rect.left = p_left;
	rect.top = p_top;
	rect.right = p_right;
	rect.bottom = p_bottom;
	if (g_currentRefreshMode->m_index == 1) {
		g_ddrawResult = IDirectDrawSurface_BltFast(g_ddrawBack, 0, 0, g_ddrawBuffer, &rect, DDBLTFAST_WAIT);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_BlitRect BltFast(): %d\n", g_ddrawResult & 0xfff);
		}

		g_ddrawResult = IDirectDrawSurface_Flip(g_ddrawPrimary, NULL, DDFLIP_WAIT);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_BlitRect Flip(): %d\n", g_ddrawResult & 0xfff);
		}
	}
	else {
		g_ddrawResult = IDirectDrawSurface_BltFast(g_ddrawPrimary, 0, 0, g_ddrawBuffer, &rect, DDBLTFAST_WAIT);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_BlitRect BltFast(): %d\n", g_ddrawResult & 0xfff);
		}
	}

	return g_ddrawResult == DD_OK ? 0 : -1;
}

// FUNCTION: MW2 0x10078826
MechS32 DdrawFill(MechS32 p_left, MechS32 p_top, MechS32 p_right, MechS32 p_bottom, MechU32 p_color)
{
	RECT rect;

	g_ddrawResult = DdrawUnlock();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_Fill Unlock(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = DdrawRestore();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_Fill Restore(): %d\n", g_ddrawResult & 0xfff);
	}

	rect.left = p_left;
	rect.top = p_top;
	rect.right = p_right;
	rect.bottom = p_bottom;
	g_ddrawBltFx.dwFillColor = p_color;
	g_ddrawResult =
		IDirectDrawSurface_Blt(g_ddrawBuffer, &rect, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &g_ddrawBltFx);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_Fill Blt(): %d\n", g_ddrawResult & 0xfff);
		return -1;
	}

	return 0;
}

// Stack-slot permutation: entries, relock and i. The original also compares i < p_count as
// `cmp [i], ...` and adds p_first + i with i loaded first; the declaration order doesn't flip it.
// FUNCTION: MW2 0x1007890f
MechS32 DdrawWritePaletteEntries(MechS32 p_first, MechS32 p_count, PaletteColor* p_palette, MechS32 p_allColors)
{
	MechS32 i;
	MechS32 relock;
	PALETTEENTRY entries[0x100];

	relock = FALSE;
	if (!g_ddrawInitialized) {
		return -1;
	}

	if (p_palette == NULL || p_first < 0 || p_first > 0xff || p_count <= 0 || p_count > 0x100 - p_first) {
		return -1;
	}

	for (i = 0; i < p_count; i++) {
		g_paletteColors[p_first + i] = p_palette[i];
		entries[i].peRed = p_palette[i].m_red * 4;
		entries[i].peGreen = p_palette[i].m_green * 4;
		entries[i].peBlue = p_palette[i].m_blue * 4;
	}

	if (g_ddrawLocked) {
		relock = TRUE;
		g_ddrawResult = DdrawUnlock();
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_WritePaletteEntries Unlock(): %d\n", g_ddrawResult & 0xfff);
		}
	}

	g_ddrawResult = DdrawRestore();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_WritePaletteEntries Restore(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = IDirectDrawPalette_SetEntries(g_ddrawPalette, 0, p_first, p_count, entries);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_WritePaletteEntries SetEntries(): %d\n", g_ddrawResult & 0xfff);
		return -1;
	}

	if (relock && (g_ddrawResult = DdrawLockBuffer()) != DD_OK) {
		DebugPrint("DDRAW_WritePaletteEntries LockBuffer(): %d\n", g_ddrawResult & 0xfff);
		return -1;
	}

	return 0;
}

// Stack-slot permutation: entries, relock and i.
// FUNCTION: MW2 0x10078b38
MechS32 DdrawWritePaletteGamma(PaletteColor* p_palette)
{
	MechS32 i;
	MechS32 relock;
	PALETTEENTRY entries[0x100];

	relock = FALSE;
	if (p_palette == NULL || g_ddrawPalette == NULL) {
		return -1;
	}

	for (i = 0; i < 0x100; i++) {
		g_paletteColorsPreBrightness[i] = p_palette[i];
		CopyPaletteColorWithBrightness(&p_palette[i], &g_paletteColors[i]);
		entries[i].peRed = g_paletteColors[i].m_red * 4;
		entries[i].peGreen = g_paletteColors[i].m_green * 4;
		entries[i].peBlue = g_paletteColors[i].m_blue * 4;
	}

	if (g_ddrawLocked) {
		relock = TRUE;
		g_ddrawResult = DdrawUnlock();
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_WritePaletteGamma Unlock(): %d\n", g_ddrawResult & 0xfff);
		}
	}

	g_ddrawResult = DdrawRestore();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_WritePaletteGamma Restore(): %d\n", g_ddrawResult & 0xfff);
	}

	g_ddrawResult = IDirectDrawPalette_SetEntries(g_ddrawPalette, 0, 0, 0x100, entries);
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_WritePaletteGamma SetEntries(): %d\n", g_ddrawResult & 0xfff);
		return -1;
	}

	if (relock && (g_ddrawResult = DdrawLockBuffer()) != DD_OK) {
		DebugPrint("DDRAW_WritePaletteGamma LockBuffer(): %d\n", g_ddrawResult & 0xfff);
		return -1;
	}

	return 0;
}

// Fades the palette from g_paletteColors to p_palette in p_steps steps, a step every 16 ms.
// Stack-slot permutation: relock, i, j, deltas and entries.
// FUNCTION: MW2 0x10078d38
MechS32 DdrawPaletteFade(PaletteColor* p_palette, MechS32 p_steps)
{
	PALETTEENTRY entries[0x100];
	MechS32 i;
	MechS32 j;
	MechDouble deltas[0x100][3];
	MechS32 relock;

	relock = FALSE;
	if (g_ddrawPalette == NULL || p_palette == NULL) {
		return -1;
	}

	if (g_ddrawLocked) {
		relock = TRUE;
		g_ddrawResult = DdrawUnlock();
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_PaletteFade Unlock(): %d\n", g_ddrawResult & 0xfff);
		}
	}

	g_ddrawResult = DdrawRestore();
	if (g_ddrawResult != DD_OK) {
		DebugPrint("DDRAW_PaletteFade Restore(): %d\n", g_ddrawResult & 0xfff);
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
			entries[j].peRed = (MechU8) ((p_steps - i) * deltas[j][0]) + g_paletteColors[j].m_red * 4;
			entries[j].peGreen = (MechU8) ((p_steps - i) * deltas[j][1]) + g_paletteColors[j].m_green * 4;
			entries[j].peBlue = (MechU8) ((p_steps - i) * deltas[j][2]) + g_paletteColors[j].m_blue * 4;
			if (i == 0) {
				g_paletteColors[j].m_red = entries[j].peRed >> 2;
				g_paletteColors[j].m_green = entries[j].peGreen >> 2;
				g_paletteColors[j].m_blue = entries[j].peBlue >> 2;
			}
		}

		g_ddrawResult = IDirectDrawPalette_SetEntries(g_ddrawPalette, 0, 0, 0x100, entries);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_PaletteFade SetEntries(): %d\n", g_ddrawResult & 0xfff);
			return -1;
		}

		Sleep(0x10);
	}

	if (relock && (g_ddrawResult = DdrawLockBuffer()) != DD_OK) {
		DebugPrint("DDRAW_PaletteFade LockBuffer(): %d\n", g_ddrawResult & 0xfff);
		return -1;
	}

	return 0;
}

// Locks g_ddrawBuffer and points the output buffer at it.
// FUNCTION: MW2 0x10079170
MechS32 DdrawLockBuffer(void)
{
	if (!g_ddrawLocked) {
		g_ddrawResult = DdrawRestore();
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_LockBuffer Restore(): %d\n", g_ddrawResult & 0xfff);
			return -1;
		}

		g_ddrawResult = IDirectDrawSurface_Lock(g_ddrawBuffer, NULL, &g_ddrawBufferDesc, DDLOCK_WAIT, NULL);
		if (g_ddrawResult != DD_OK) {
			DebugPrint("DDRAW_LockBuffer Lock(): %d\n", g_ddrawResult & 0xfff);
			return -1;
		}

		g_ddrawLocked = TRUE;
	}

	g_refreshModeBuffer->m_buffer = g_ddrawBufferDesc.lpSurface;
	g_refreshModeBuffer->m_xMax = g_ddrawBufferDesc.lPitch - 1;
	return 0;
}

// Releases the pending lock on g_ddrawBuffer.
// FUNCTION: MW2 0x10079240
__inline static HRESULT DdrawUnlock(void)
{
	if (g_ddrawLocked) {
		g_ddrawResult = IDirectDrawSurface_Unlock(g_ddrawBuffer, g_ddrawBufferDesc.lpSurface);
		g_refreshModeBuffer->m_buffer = NULL;
		g_ddrawLocked = FALSE;
		return g_ddrawResult;
	}
	else {
		return DD_OK;
	}
}

// Restores the primary surface and g_ddrawBuffer when they were lost.
// FUNCTION: MW2 0x100792b0
__inline static HRESULT DdrawRestore(void)
{
	if (IDirectDrawSurface_IsLost(g_ddrawPrimary) == DDERR_SURFACELOST) {
		g_ddrawResult = IDirectDrawSurface_Restore(g_ddrawPrimary);
		if (IDirectDrawSurface_IsLost(g_ddrawBuffer) == DDERR_SURFACELOST) {
			g_ddrawResult = IDirectDrawSurface_Restore(g_ddrawBuffer);
		}
		return g_ddrawResult;
	}
	else {
		return DD_OK;
	}
}
