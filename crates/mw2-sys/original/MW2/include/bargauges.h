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
	extern undefined4 g_unk0x100a82fc;
	extern EasedValue g_throttleBarLevel;
	extern undefined4 g_unk0x100a830c;
	extern EasedValue g_heatRateBarLevel;
	extern undefined4 g_unk0x100a831c;
	extern EasedValue g_jumpFuelBarLevel;
	extern undefined4 g_unk0x100a832c;
	extern Point g_throttleGaugeSize;
	extern MechS32 g_throttleFrameLeft;
	extern MechS32 g_throttleFrameTop;
	extern MechS32 g_throttleFrameRight;
	extern MechS32 g_throttleFrameBottom;
	extern MechS32 g_throttleBarLeft;
	extern MechS32 g_throttleZeroY;
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
	void DrawVerticalBar(PANE* p_target, MechS32 p_x, MechS32 p_y, MechS32 p_width, MechS32 p_height, MechS32 p_color);
	void DrawHorizontalBar(
		PANE* p_target,
		MechS32 p_x,
		MechS32 p_y,
		MechS32 p_width,
		MechS32 p_height,
		MechS32 p_color
	);

#ifdef __cplusplus
}
#endif

#endif // BARGAUGES_H
