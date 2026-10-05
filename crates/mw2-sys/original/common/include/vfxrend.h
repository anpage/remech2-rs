#ifndef VFXREND_H
#define VFXREND_H

#include "decomp.h"
#include "pane.h"
#include "types.h"

// The texture a polygon maps (VFXREND.H's VFX_TEXTURE): the table of row ("V-baseline") addresses
// and the tiling limits (DrawTexturedPolygon sets both to the texture's height).
// SIZE 0xc
typedef struct VFX_TEXTURE {
	MechU8** m_vAddrs; // 0x00
	MechS32 m_width;   // 0x04
	MechS32 m_height;  // 0x08
} VFX_TEXTURE;

// VFXREND, Miles Design VFX's polygon renderer (3rdparty/vfx/VFXREND.ASM; common/src/vfxrend.c's
// portable C), which both DLLs link. p_cueing and p_translucency are lookaside
// tables: p_cueing of 256 bytes for flat shading, of 256 rows of 256 for Gouraud shading (a row
// per shade), p_translucency of 256 bytes. MW2 draws its textured polygons through
// VFX_polygon_clip_XY_and_render (DrawTexturedPolygon), and render.c asks GetCodeBlock for the range to
// make writable. In MW2SHELL the object is believed to be dead: nothing outside it refers into it,
// and .text is read-only with no VirtualProtect import, so its routines could not patch themselves.
#ifdef __cplusplus
extern "C"
{
#endif

	void VFX_set_Gouraud_dither_level(MechS32 p_dither1, MechS32 p_dither2);
	MechS32 GetCodeBlock(undefined4* p_start, undefined4* p_selector);
	void VFX_polygon_render(
		PANE* p_pane,
		MechU32* p_vlist,
		MechS32 p_nvertices,
		MechS32 p_operation,
		undefined4 p_color,
		VFX_TEXTURE* p_texture,
		void* p_cueing,
		void* p_translucency
	);
	MechS32 F16_div_to_F30(MechS32 p_dividend, MechS32 p_divisor);
	MechS32 F30_reciprocal(MechS32 p_value);
	MechS32 mul_F30(MechS32 p_m1, MechS32 p_m2);
	void VFX_polygon_clip_XY_and_render(
		PANE* p_pane,
		MechU32* p_vlist,
		MechS32 p_nvertices,
		MechS32 p_operation,
		undefined4 p_color,
		VFX_TEXTURE* p_texture,
		void* p_cueing,
		void* p_translucency
	);

#ifdef __cplusplus
}
#endif

// VFXREND's routines and data. reccmp reads annotations from C sources only, so they're here, by
// name. The table of primitives (poly_vectors) is indexed by the operation flags; the primitives
// (poly_<flags>, in decimal) are the ones 3rdparty/vfx/RENDOPTS.INC builds, and patch their own
// operands.

// GLOBAL: MW2SHELL 0x10017a7c
// GLOBAL: MW2 0x10021cf4
// poly_vectors

// FUNCTION: MW2SHELL 0x10019a7c
// FUNCTION: MW2 0x10023cf4
// poly_1681

// FUNCTION: MW2SHELL 0x1001a686
// FUNCTION: MW2 0x100248fe
// poly_1553

// FUNCTION: MW2SHELL 0x1001b18e
// FUNCTION: MW2 0x10025406
// poly_1169

// FUNCTION: MW2SHELL 0x1001bafb
// FUNCTION: MW2 0x10025d73
// poly_1041

// FUNCTION: MW2SHELL 0x1001c366
// FUNCTION: MW2 0x100265de
// poly_1696

// FUNCTION: MW2SHELL 0x1001cd7c
// FUNCTION: MW2 0x10026ff4
// poly_1664

// FUNCTION: MW2SHELL 0x1001d853
// FUNCTION: MW2 0x10027acb
// poly_1687

// FUNCTION: MW2SHELL 0x1001e687
// FUNCTION: MW2 0x100288ff
// poly_1673

// FUNCTION: MW2SHELL 0x1001f491
// FUNCTION: MW2 0x10029709
// poly_1713

// FUNCTION: MW2SHELL 0x1001ff8e
// FUNCTION: MW2 0x1002a206
// poly_1680

// FUNCTION: MW2SHELL 0x10020b5c
// FUNCTION: MW2 0x1002add4
// poly_1536

// FUNCTION: MW2SHELL 0x100214f5
// FUNCTION: MW2 0x1002b76d
// poly_1552

// FUNCTION: MW2SHELL 0x10021fc1
// FUNCTION: MW2 0x1002c239
// poly_1616

// FUNCTION: MW2SHELL 0x10022ec2
// FUNCTION: MW2 0x1002d13a
// poly_1175

// FUNCTION: MW2SHELL 0x10023a55
// FUNCTION: MW2 0x1002dccd
// poly_1168

// FUNCTION: MW2SHELL 0x10024386
// FUNCTION: MW2 0x1002e5fe
// poly_1024

// FUNCTION: MW2SHELL 0x10024a82
// FUNCTION: MW2 0x1002ecfa
// poly_1040

// FUNCTION: MW2SHELL 0x100252b1
// FUNCTION: MW2 0x1002f529
// poly_1032

// FUNCTION: MW2SHELL 0x10025ce7
// FUNCTION: MW2 0x1002ff5f
// poly_1104

// FUNCTION: MW2SHELL 0x10026947
// FUNCTION: MW2 0x10030bbf
// poly_128

// FUNCTION: MW2SHELL 0x10026cd6
// FUNCTION: MW2 0x10030f4e
// poly_640

// FUNCTION: MW2SHELL 0x10026fc9
// FUNCTION: MW2 0x10031241
// poly_64

// FUNCTION: MW2SHELL 0x1002763d
// FUNCTION: MW2 0x100318b5
// poly_768

// FUNCTION: MW2SHELL 0x10027aa6
// FUNCTION: MW2 0x10031d1e
// poly_704

// FUNCTION: MW2SHELL 0x10028018
// FUNCTION: MW2 0x10032290
// poly_576

// FUNCTION: MW2SHELL 0x100286a6
// FUNCTION: MW2 0x1003291e
// VFX_set_Gouraud_dither_level

// FUNCTION: MW2SHELL 0x100286c3
// FUNCTION: MW2 0x1003293b
// GetCodeBlock

// FUNCTION: MW2SHELL 0x100286eb
// FUNCTION: MW2 0x10032963
// VFX_polygon_render

// FUNCTION: MW2SHELL 0x1002875c
// FUNCTION: MW2 0x100329d4
// F16_div_to_F30

// FUNCTION: MW2SHELL 0x10028794
// FUNCTION: MW2 0x10032a0c
// F30_reciprocal

// FUNCTION: MW2SHELL 0x100287c4
// FUNCTION: MW2 0x10032a3c
// mul_F30

// FUNCTION: MW2SHELL 0x100287e0
// FUNCTION: MW2 0x10032a58
// VFX_polygon_clip_XY_and_render

#endif // VFXREND_H
