#ifndef OVERLAY_H
#define OVERLAY_H

#include "eyepoint.h"
#include "point.h"
#include "shape.h"
#include "types.h"

// The functions and globals of overlay.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_showPalette;
	extern int32_t g_showFrameRate;
	extern int32_t g_showFrameRateMain;
	extern int32_t g_showSceneInfo;
	extern int32_t g_showSceneInfoMain;
	extern int32_t g_showEyePosition;
	extern int32_t g_showEyePositionMain;
	extern int32_t g_showMemInfo;
	extern int32_t g_showMemInfoMain;
	extern int32_t g_monoEnabled;
	extern char g_blankText[34];
	extern int32_t g_monoLastRow;
	extern int32_t g_frameRateShown;
	extern Point g_frameRateOrigin;
	extern Point g_memInfoOrigin;
	extern Point g_eyePositionOrigin;
	extern int32_t g_sceneInfoShown;
	extern int32_t g_eyePositionShown;
	extern int32_t g_memInfoShown;
	extern int32_t g_showCacheInfo;
	extern int32_t g_cacheInfoShown;
	extern int32_t g_showSpinner;
	extern char g_spinnerArrows[4];
	extern int32_t g_frameRate;
	extern int32_t g_frameRateTenths;
	extern int32_t g_frameRateTime;
	extern int32_t g_frameRateFrames;
	extern int32_t g_spinnerDelay;
	extern int32_t g_spinnerArrow;
	extern int32_t g_monoRow;
	extern int32_t g_monoColumn;
	extern int32_t g_sceneVertexCount;
	extern int32_t g_sceneMemory;
	extern int32_t g_sceneShapeCount;
	extern int32_t g_sceneFaceCount;
	extern char g_monoBlankLine[0x50];
	extern char g_monoBlankLineEnd;

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
	void MonoPrintLine(char* p_text);
	void MonoClear(void);
	void MonoPrint(char* p_text);
	void ScaleOverlayPositions(void);
	void SimEntranceDbug(char* p_mission, int32_t p_memory);

#ifdef __cplusplus
}
#endif

#endif // OVERLAY_H
