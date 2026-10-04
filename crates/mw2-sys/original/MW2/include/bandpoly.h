#ifndef BANDPOLY_H
#define BANDPOLY_H

#include "decomp.h"
#include "point.h"
#include "types.h"

// The functions and globals of bandpoly.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern Point g_bandTexCoords[4];

	void DrawBandPolygon(MechU32 p_flags, MechS32 p_count, MechU32* p_points, MechS32 p_unk0x0c);
	MechS32 DrawBand(MechU32 p_color, MechS32 p_count, MechS32* p_points, MechS32 p_luma, MechS32 p_mode);
	MechS32 DrawBandWithFlash(undefined4 p_unk0x00, MechS32 p_unk0x04, undefined4* p_unk0x08);

#ifdef __cplusplus
}
#endif

#endif // BANDPOLY_H
