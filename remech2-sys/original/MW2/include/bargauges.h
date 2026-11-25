#ifndef BARGAUGES_H
#define BARGAUGES_H

#include "point.h"
#include "ramp.h"
#include "targeting.h"
#include "types.h"

// The functions and globals of bargauges.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern EasedValue g_heatBarLevel;
	extern EasedValue g_throttleBarLevel;
	extern EasedValue g_heatRateBarLevel;
	extern EasedValue g_jumpFuelBarLevel;
	extern Point g_throttleGaugeSize;
	extern int32_t g_throttleFrameLeft;
	extern int32_t g_throttleFrameTop;
	extern int32_t g_throttleFrameRight;
	extern int32_t g_throttleFrameBottom;
	extern int32_t g_throttleBarLeft;
	extern int32_t g_throttleZeroY;
	extern Point g_heatBarPosition;
	extern Point g_heatBarSize;
	extern Point g_heatRateBarPosition;
	extern Point g_heatRateBarSize;
	extern Point g_jumpFuelBarPosition;
	extern Point g_jumpFuelBarSize;

	void ScaleBarGauges(void);
	void DrawHeatBar(PANE* p_target);
	void DrawHeatRateBar(PANE* p_target);
	void DrawThrottleGauge(PANE* p_target);
	void DrawJumpFuelBar(PANE* p_target);
	void DrawVerticalBar(PANE* p_target, int32_t p_x, int32_t p_y, int32_t p_width, int32_t p_height, int32_t p_color);
	void DrawHorizontalBar(
		PANE* p_target,
		int32_t p_x,
		int32_t p_y,
		int32_t p_width,
		int32_t p_height,
		int32_t p_color
	);

#ifdef __cplusplus
}
#endif

#endif // BARGAUGES_H
