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
	extern MechChar g_blankText[34];
	extern MechS32 g_unk0x100a94a0;
	extern MechS32 g_monoLastRow;
	extern MechS32 g_unk0x100a94a8;
	extern MechChar g_unk0x100a94ac[4];
	extern MechS32 g_frameRateShown;
	extern MechS32 g_unk0x100a94bc;
	extern Point g_frameRateOrigin;
	extern Point g_memInfoOrigin;
	extern Point g_eyePositionOrigin;
	extern MechS32 g_sceneInfoShown;
	extern MechS32 g_eyePositionShown;
	extern MechS32 g_memInfoShown;
	extern MechS32 g_showCacheInfo;
	extern MechS32 g_cacheInfoShown;
	extern MechS32 g_showSpinner;
	extern MechChar g_spinnerArrows[4];
	extern MechS32 g_nextCacheDump;
	extern MechS32 g_dumpCacheRepeat;
	extern MechS32 g_unk0x100a9514;
	extern MechS32 g_unk0x100a9518;
	extern MechS32 g_frameRate;
	extern MechS32 g_frameRateTenths;
	extern MechS32 g_frameRateTime;
	extern MechS32 g_frameRateFrames;
	extern MechS32 g_spinnerDelay;
	extern MechS32 g_spinnerArrow;
	extern MechS32 g_monoRow;
	extern MechS32 g_monoColumn;
	extern MechS32 g_sceneVertexCount;
	extern MechS32 g_sceneMemory;
	extern MechS32 g_sceneShapeCount;
	extern MechS32 g_sceneFaceCount;
	extern MechChar g_monoBlankLine[0x50];
	extern MechChar g_monoBlankLineEnd;

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
