#ifndef VFXREND_H
#define VFXREND_H

#include "decomp.h"
#include "pane.h"
#include "types.h"

// The texture a polygon maps (VFXREND.H's VFX_TEXTURE): the table of row ("V-baseline") addresses
// and the tiling limits (DrawTexturedPolygon sets both to the texture's height).
typedef struct VFX_TEXTURE {
	uint8_t** m_vAddrs; // 0x00
	int32_t m_width;    // 0x04
	int32_t m_height;   // 0x08
} VFX_TEXTURE;

// VFXREND, Miles Design VFX's polygon renderer, reimplemented in C (common/src/vfxrend.c), which
// both DLLs link. p_cueing and p_translucency are lookaside
// tables: p_cueing of 256 bytes for flat shading, of 256 rows of 256 for Gouraud shading (a row
// per shade), p_translucency of 256 bytes. MW2 draws its textured polygons through
// VFX_polygon_clip_XY_and_render (DrawTexturedPolygon), and render.c asks GetCodeBlock for the range to
// make writable. In MW2SHELL the object is believed to be dead: nothing outside it refers into it,
// and .text is read-only with no VirtualProtect import, so its routines could not patch themselves.
#ifdef __cplusplus
extern "C"
{
#endif

	void VFX_set_Gouraud_dither_level(int32_t p_dither1, int32_t p_dither2);
	int32_t GetCodeBlock(undefined4* p_start, undefined4* p_selector);
	void VFX_polygon_render(
		PANE* p_pane,
		uint32_t* p_vlist,
		int32_t p_nvertices,
		int32_t p_operation,
		undefined4 p_color,
		VFX_TEXTURE* p_texture,
		void* p_cueing,
		void* p_translucency
	);
	int32_t F16_div_to_F30(int32_t p_dividend, int32_t p_divisor);
	int32_t F30_reciprocal(int32_t p_value);
	int32_t mul_F30(int32_t p_m1, int32_t p_m2);
	void VFX_polygon_clip_XY_and_render(
		PANE* p_pane,
		uint32_t* p_vlist,
		int32_t p_nvertices,
		int32_t p_operation,
		undefined4 p_color,
		VFX_TEXTURE* p_texture,
		void* p_cueing,
		void* p_translucency
	);

#ifdef __cplusplus
}
#endif

#endif // VFXREND_H
