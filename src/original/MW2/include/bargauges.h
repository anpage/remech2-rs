#ifndef BARGAUGES_H
#define BARGAUGES_H

#include "targeting.h"
#include "types.h"

// The functions and globals of bargauges.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

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
