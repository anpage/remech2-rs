#ifndef BANDPOLY_H
#define BANDPOLY_H

#include "point.h"
#include "types.h"

// The functions and globals of bandpoly.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern Point g_bandTexCoords[4];

	void DrawBandPolygon(uint32_t p_flags, int32_t p_count, uint32_t* p_points, int32_t p_unk0x0c);
	int32_t DrawBand(uint32_t p_color, int32_t p_count, int32_t* p_points, int32_t p_luma, int32_t p_mode);
	int32_t DrawBandWithFlash(uint32_t p_unk0x00, int32_t p_unk0x04, uint32_t* p_unk0x08);

#ifdef __cplusplus
}
#endif

#endif // BANDPOLY_H
