#ifndef TEXPOLY_H
#define TEXPOLY_H

#include "targeting.h"
#include "types.h"

// The functions and globals of texpoly.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void DrawTexturedPolygon(
		PANE* p_target,
		uint8_t* p_pixels,
		int16_t p_width,
		int16_t p_height,
		int32_t p_count,
		uint32_t* p_points,
		int32_t p_useLuma,
		uint16_t* p_luma
	);

#ifdef __cplusplus
}
#endif

#endif // TEXPOLY_H
