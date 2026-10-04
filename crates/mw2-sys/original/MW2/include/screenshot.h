#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include "targeting.h"
#include "types.h"

// The functions and globals of screenshot.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_screenshotState;
	extern PANE* g_screenshotTarget;

	MechS32 ScreenshotBegin(const MechChar* p_filename);
	void ScreenshotWritePalette(void);
	MechS32 ScreenshotWriteImage(PANE* p_target);
	void ScreenshotEnd(void);
	MechS32 ScreenshotGetPixel(MechS32 p_x, MechS32 p_y);

#ifdef __cplusplus
}
#endif

#endif // SCREENSHOT_H
