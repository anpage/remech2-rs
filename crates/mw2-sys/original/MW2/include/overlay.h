#ifndef OVERLAY_H
#define OVERLAY_H

#include "eyepoint.h"
#include "shape.h"
#include "types.h"

// The functions and globals of overlay.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_showPalette;
	extern MechS32 g_showFrameRate;
	extern MechS32 g_showFrameRateMain;
	extern MechS32 g_showSceneInfo;
	extern MechS32 g_showSceneInfoMain;
	extern MechS32 g_showEyePosition;
	extern MechS32 g_showEyePositionMain;
	extern MechS32 g_showMemInfo;
	extern MechS32 g_showMemInfoMain;
	extern MechS32 g_monoEnabled;

	void DrawDebugOverlays(void);
	void DrawPaletteGrid(void);
	void InitializeMono(void);
	void ShowFrameRate(void);
	void HideFrameRate(void);
	void ShowSceneInfo(void);
	void HideSceneInfo(void);
	void CountSceneShape(Shape* p_shape);
	void ShowEyePosition(Eyepoint* p_eyepoint);
	void HideEyePosition(void);
	void ShowCacheInfo(void);
	void HideCacheInfo(void);
	void ShowMemInfo(void);
	void HideMemInfo(void);
	void StepSpinner(void);
	void MonoPrintLine(MechChar* p_text);
	void MonoClear(void);
	void MonoPrint(MechChar* p_text);
	void ScaleOverlayPositions(void);
	void SimEntranceDbug(MechChar* p_mission, MechS32 p_memory);

#ifdef __cplusplus
}
#endif

#endif // OVERLAY_H
