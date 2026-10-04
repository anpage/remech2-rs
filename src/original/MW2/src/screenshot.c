#include "screenshot.h"

#include "gifsave.h"
#include "palettecolor.h"
#include "refreshmode.h"
#include "render.h"
#include "targeting.h"
#include "types.h"

// Screenshots: the steps of writing the screen to a GIF file, which must come in order.
// ScreenshotBegin creates the file, ScreenshotWritePalette its color table,
// ScreenshotWriteImage the image from a pane, and ScreenshotEnd closes it.

enum ScreenshotState {
	c_screenshotCreated = 0,
	c_screenshotPaletteWritten = 1,
	c_screenshotImageWritten = 2,
	c_screenshotIdle = 3
};

// GLOBAL: MW2 0x100ad498
MechS32 g_screenshotState = c_screenshotIdle;

// GLOBAL: MW2 0x100c75e0
PANE* g_screenshotTarget;

// FUNCTION: MW2 0x1006cdc0
MechS32 ScreenshotBegin(const MechChar* p_filename)
{
	MechS32 result = c_gifErrCreate;

	if (p_filename && g_screenshotState == c_screenshotIdle) {
		result = GifCreate(
			p_filename,
			g_gameWindowGeometry->m_width,
			g_gameWindowGeometry->m_height,
			g_gameWindowGeometry->m_numColors,
			6
		);
		g_screenshotState = c_screenshotCreated;
	}

	return result;
}

// Stack-slot permutation: color and i.
// FUNCTION: MW2 0x1006ce29
void ScreenshotWritePalette(void)
{
	PaletteColor color;
	MechS32 i;
	MechS32 numColors;

	numColors = g_gameWindowGeometry->m_numColors;
	if (g_screenshotState == c_screenshotCreated) {
		for (i = 0; i < numColors; i++) {
			GetPaletteColors(i, 1, &color);
			GifSetColor(i, color.m_red, color.m_green, color.m_blue);
		}

		g_screenshotState = c_screenshotPaletteWritten;
	}
}

// Stack-slot permutation: width and result.
// FUNCTION: MW2 0x1006cea9
MechS32 ScreenshotWriteImage(PANE* p_target)
{
	MechS32 width;
	MechS32 result = c_gifErrWrite;
	MechS32 height;

	if (p_target && g_screenshotState == c_screenshotPaletteWritten) {
		g_screenshotTarget = p_target;
		width = p_target->m_x1 - p_target->m_x0 + 1;
		height = p_target->m_y1 - p_target->m_y0 + 1;
		result = GifCompressImage(p_target->m_x0, p_target->m_y0, width, height, ScreenshotGetPixel);
		g_screenshotState = c_screenshotImageWritten;
	}

	return result;
}

// FUNCTION: MW2 0x1006cf35
void ScreenshotEnd(void)
{
	g_screenshotState = c_screenshotIdle;
	GifClose();
	return;
}

// FUNCTION: MW2 0x1006cf54
MechS32 ScreenshotGetPixel(MechS32 p_x, MechS32 p_y)
{
	return VFX_pixel_read(g_screenshotTarget, p_x, p_y);
}
