#include "timedoverlays.h"

#include "clock.h"
#include "loadres.h"
#include "menu.h"
#include "mw2prj.h"
#include "point.h"
#include "render.h"
#include "screenscale.h"
#include "setres.h"
#include "targeting.h"
#include "timedoverlay.h"
#include "types.h"
#include "widescreen.h"

#include <string.h>

PANE g_topMessagePane = {&g_mainPixelBuffer, 0, 0, 0x10000, 0x10000};

PANE g_bottomMessagePane = {&g_mainPixelBuffer, 0, 0, 0x10000, 0x10000};

char g_bottomMessageText[0x100];

char g_topMessageText[0x100];

TimedOverlay g_timedOverlays[2] = {
	{g_topMessageText, {0x28f, 0}, 0, 0, 1, 0x4c, 0, &g_topMessagePane},
	{g_bottomMessageText, {0x28f, 0}, 0, 0, 1, 0x4c, 0, &g_bottomMessagePane}
};

// Lays out the message boxes: scales their rectangles to the screen and to their background
// shapes, moves them to the top (the first) and bottom (the second) edge, and centers the
// text vertically.
void LayoutMessageBoxes(void)
{
	int32_t fontHeight;
	int32_t sign;
	int32_t dy;
	int32_t dx;
	PANE* target;
	void* font;
	int32_t i;
	TimedOverlay* overlay;
	void* shape;
	int32_t height;

	for (i = 0; i < 2; i++) {
		overlay = &g_timedOverlays[i];
		target = overlay->m_target;
		ScaleRectToScreen(&g_mainPixelBuffer, target, target);
		shape = LoadCachedResource(
			g_mw2PrjHandle,
			overlay->m_background + g_artResolution,
			g_resourceTypeTags[c_resTagShp],
			0
		);
		if (shape != NULL) {
			FitRectToShape(target, target, shape, 0);
		}

		if (i == 0) {
			sign = -1;
		}
		else {
			sign = 1;
		}

		dx = -target->m_x0;
		dy = target->m_y0 * sign;
		target->m_x0 += dx;
		target->m_y0 += dy;
		target->m_x1 += dx;
		target->m_y1 += dy;
		MechCenterMessagePane(target);
		ScalePointToFrame(target, &overlay->m_textPos, &overlay->m_textPos);

		font =
			LoadCachedResource(g_mw2PrjHandle, overlay->m_font + g_artResolution, g_resourceTypeTags[c_resTagFont], 0);
		if (font != NULL) {
			height = target->m_y1 - target->m_y0 + 1;
			fontHeight = VFX_font_height(font);
			overlay->m_textPos.m_y = (height - fontHeight) / 2;
		}
	}
}

// Shows p_text in a free message box for p_duration ticks, or replaces the one of the lowest
// priority not above p_priority (of those, the one that expires last). Returns 1 if shown.
int32_t ShowInGameMessage(char* p_text, int32_t p_font, int32_t p_duration, int32_t p_priority)
{
	int32_t result;
	int32_t slot;
	int32_t latest;
	int32_t i;
	int32_t done;
	int32_t lowestPriority;
	TimedOverlay* overlay;

	done = false;
	result = 0;
	slot = -1;
	lowestPriority = 0x29a;
	i = 0;
	while (!done) {
		if (!g_timedOverlays[i].m_active) {
			slot = i;
			done = true;
		}
		else {
			if (g_timedOverlays[i].m_priority <= p_priority) {
				if (g_timedOverlays[i].m_priority < lowestPriority) {
					slot = i;
					lowestPriority = g_timedOverlays[i].m_priority;
					latest = g_timedOverlays[i].m_expireTime;
				}
				else if (g_timedOverlays[i].m_priority == lowestPriority && g_timedOverlays[i].m_expireTime > latest) {
					slot = i;
					latest = g_timedOverlays[i].m_expireTime;
				}
			}

			i++;
			if (i == 2) {
				done = true;
			}
		}
	}

	if (slot != -1) {
		result = 1;
		overlay = &g_timedOverlays[slot];
		strncpy(overlay->m_text, p_text, 0xff);
		overlay->m_text[0xff] = '\0';
		overlay->m_active = 1;
		overlay->m_priority = p_priority;
		overlay->m_expireTime = p_duration + GetGameClock();
		if (p_font < 1) {
			overlay->m_font = 1;
		}
		else {
			overlay->m_font = p_font;
		}
	}

	return result;
}

void DrawTimedOverlays(void)
{
	int32_t i;
	TimedOverlay* overlay;
	void* font;
	void* background;

	for (i = 0; i < 2; i++) {
		overlay = &g_timedOverlays[i];
		if (overlay->m_active) {
			if (GetGameClock() < overlay->m_expireTime) {
				font = LoadCachedResource(
					g_mw2PrjHandle,
					overlay->m_font + g_artResolution,
					g_resourceTypeTags[c_resTagFont],
					0
				);
				if (font != NULL) {
					background = LoadCachedResource(
						g_mw2PrjHandle,
						overlay->m_background + g_artResolution,
						g_resourceTypeTags[c_resTagShp],
						0
					);
					if (background != NULL) {
						VFX_shape_draw(overlay->m_target, background, 0, 0, 0);
						VFX_string_draw(
							overlay->m_target,
							overlay->m_textPos.m_x,
							overlay->m_textPos.m_y,
							font,
							overlay->m_text,
							g_textColors
						);
					}
				}
			}
			else {
				g_timedOverlays[i].m_active = 0;
			}
		}
	}
}

// Draws text in a box sized to it, on a tiled background shape (p_background, -1 for none),
// at p_x, p_y or centered on the screen along an axis where that is negative.
void DrawTextBox(int32_t p_background, int32_t p_font, char* p_text, int32_t p_x, int32_t p_y)
{
	PANE centered;
	void* background;
	PANE rect;
	void* font;

	background = NULL;
	if (p_text == NULL) {
		return;
	}

	if (p_background != -1) {
		background =
			LoadCachedResource(g_mw2PrjHandle, p_background + g_artResolution, g_resourceTypeTags[c_resTagShp], 0);
	}

	font = LoadCachedResource(g_mw2PrjHandle, p_font + g_artResolution, g_resourceTypeTags[c_resTagFont], 0);
	if (font != NULL) {
		rect.m_window = &g_mainPixelBuffer;
		FitRectToText(p_text, font, &rect);
		if (p_x >= 0) {
			rect.m_x0 += p_x;
			rect.m_x1 += p_x;
		}
		else {
			centered = rect;
			CenterRectOnScreen(&g_mainPixelBuffer, &centered, &centered);
			rect.m_x0 = centered.m_x0;
			rect.m_x1 = centered.m_x1;
		}

		if (p_y >= 0) {
			rect.m_y0 += p_y;
			rect.m_y1 += p_y;
		}
		else {
			centered = rect;
			CenterRectOnScreen(&g_mainPixelBuffer, &centered, &centered);
			rect.m_y0 = centered.m_y0;
			rect.m_y1 = centered.m_y1;
		}

		if (background != NULL) {
			TilePane(&rect, background, 0);
		}

		DrawWrappedText(&rect, p_text, font);
	}
}

// Draws a TEXT resource, stored with each character negated (mod 256), as DrawTextBox does.
void DrawTextResourceBox(int32_t p_background, int32_t p_font, int32_t p_id, int32_t p_x, int32_t p_y)
{
	char* text;
	char* c;
	int32_t ch;

	text = LoadCachedResource(g_mw2PrjHandle, p_id, g_resourceTypeTags[c_resTagText], 0);
	if (text != NULL) {
		c = text;
		while (*c != '\0') {
			ch = *c;
			ch = 0x100 - ch;
			*c = ch;
			c++;
		}

		DrawTextBox(p_background, p_font, text, p_x, p_y);
	}
}
