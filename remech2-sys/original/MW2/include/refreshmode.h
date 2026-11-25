#ifndef REFRESHMODE_H
#define REFRESHMODE_H

#include "displaybackend.h"
#include "palettecolor.h"
#include "types.h"
#include "window.h"

#pragma pack(1)
// A refresh mode (the original's debug strings name them): how the framebuffer reaches the
// screen through a display back end. The original had six, several to a back end.
struct RefreshMode {
	int32_t m_index;
	int32_t m_backend;   // DisplayBackendId
	int32_t m_available; // cleared when m_begin fails
	uint32_t m_profileTime;
	int32_t (*m_begin)(WINDOW* p_buffer, int32_t p_width, int32_t p_height);
	int32_t (*m_end)(void);
	int32_t (*m_flip)(void);
	int32_t (*m_blitRect)(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);
	int32_t (*m_stretchBlit)(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);
};
typedef struct RefreshMode RefreshMode;
#pragma pack()

// The functions and globals of refreshmode.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern DisplayBackend* g_currentDisplayBackend;
	extern RefreshMode* g_currentRefreshMode;
	extern WINDOW* g_refreshModeBuffer;
	extern PaletteColor g_paletteColors[0x100];
	extern int32_t g_windowMode;
	extern int32_t g_refreshModePixelCount;
	extern int32_t g_refreshModeWidth;
	extern int32_t g_refreshModeHeight;
	extern DisplayBackend g_displayBackend;
	extern RefreshMode g_refreshMode;
	extern uint8_t* g_frame;

	int32_t InitRefreshMode(
		int32_t p_mode,
		int32_t p_allowFallback,
		WINDOW* p_buffer,
		int32_t p_width,
		int32_t p_height,
		int32_t p_menu
	);
	void ShutdownRefreshMode(void);
	int32_t GetPaletteColors(int32_t p_first, int32_t p_count, PaletteColor* p_palette);

#ifdef __cplusplus
}
#endif

#endif // REFRESHMODE_H
