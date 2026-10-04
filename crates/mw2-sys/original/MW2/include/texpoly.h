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
		MechU8* p_pixels,
		MechS16 p_width,
		MechS16 p_height,
		MechS32 p_count,
		MechU32* p_points,
		MechS32 p_useLuma,
		MechU16* p_luma
	);

#ifdef __cplusplus
}
#endif

#endif // TEXPOLY_H
