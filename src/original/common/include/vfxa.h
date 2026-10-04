#ifndef VFXA_H
#define VFXA_H

#include "decomp.h"
#include "pane.h"
#include "types.h"
#include "window.h"

// VFXA, Miles Design VFX's 2D primitives (3rdparty/vfx/VFXA.ASM; its portable C, common/src/vfxa.c,
// in COMPAT_MODE), which both DLLs link.
#ifdef __cplusplus
extern "C"
{
#endif

	// The registered display driver's entry points (VFX.H's hardware-specific functions), in the
	// order VFX_register_driver copies them from the driver's table.
	extern void* (*VFX_describe_driver)(void);
	extern void (*VFX_init_driver)(void);
	extern void (*VFX_shutdown_driver)(void);
	extern void (*VFX_area_wipe)(MechS32 p_x0, MechS32 p_y0, MechS32 p_x1, MechS32 p_y1, MechS32 p_color);
	extern void (*VFX_wait_vblank)(void);
	extern void (*VFX_wait_vblank_leading)(void);
	extern void (*VFX_window_refresh)(WINDOW* p_target, MechS32 p_x0, MechS32 p_y0, MechS32 p_x1, MechS32 p_y1);
	extern void (*VFX_window_read)(WINDOW* p_destination, MechS32 p_x0, MechS32 p_y0, MechS32 p_x1, MechS32 p_y1);
	extern void (*VFX_DAC_read)(MechS32 p_colorNumber, MechU8* p_triplet);
	extern void (*VFX_DAC_write)(MechS32 p_colorNumber, MechU8* p_triplet);
	extern void (*VFX_bank_reset)(void);
	extern void (*VFX_pane_refresh)(PANE* p_target, MechS32 p_x0, MechS32 p_y0, MechS32 p_x1, MechS32 p_y1);
	extern void (*VFX_line_address)(MechS32 p_x, MechS32 p_y, MechU8** p_addr, MechU32* p_nbytes);

	MechChar* VFX_driver_name(MechChar* (**p_vfxScanDll)(void) );
	void VFX_register_driver(MechChar* (**p_dllBase)(void) );
	MechS32 VFX_pixel_write(PANE* p_pane, MechS32 p_x, MechS32 p_y, MechU32 p_color);
	MechS32 VFX_pixel_read(PANE* p_pane, MechS32 p_x, MechS32 p_y);
	MechS32 VFX_line_draw(
		PANE* p_pane,
		MechS32 p_x0,
		MechS32 p_y0,
		MechS32 p_x1,
		MechS32 p_y1,
		MechS32 p_mode,
		MechS32 p_parm
	);
	void VFX_rectangle_hash(PANE* p_pane, MechS32 p_x0, MechS32 p_y0, MechS32 p_x1, MechS32 p_y1, MechU8 p_color);
	void DrawShapeUnclipped(PANE* p_pane, void* p_shape, MechS32 p_hotX, MechS32 p_hotY, undefined4 p_cpW);
	MechS32 XlatShapeUnclipped(PANE* p_pane, void* p_shape, MechS32 p_hotX, MechS32 p_hotY, undefined4 p_cpW);
	void VFX_shape_draw(PANE* p_pane, void* p_shapeTable, MechS32 p_shapeNumber, MechS32 p_hotX, MechS32 p_hotY);
	void VFX_shape_lookaside(MechU8* p_table);
	MechS32 VFX_shape_translate_draw(
		PANE* p_pane,
		void* p_shapeTable,
		MechS32 p_shapeNumber,
		MechS32 p_hotX,
		MechS32 p_hotY
	);
	MechS32 VFX_shape_remap_colors(void* p_shapeTable, MechS32 p_shapeNumber);
	MechS32 VFX_shape_visible_rectangle(
		void* p_shapeTable,
		MechS32 p_shapeNumber,
		MechS32 p_hotX,
		MechS32 p_hotY,
		undefined4 p_mirror,
		MechS32* p_rectangle
	);
	MechS32 VFX_shape_bounds(void* p_shapeTable, MechS32 p_shapeNum);
	MechS32 VFX_shape_origin(void* p_shapeTable, MechS32 p_shapeNum);
	MechS32 VFX_shape_resolution(void* p_shapeTable, MechS32 p_shapeNum);
	MechS32 VFX_shape_minxy(void* p_shapeTable, MechS32 p_shapeNum);
	void VFX_shape_palette(void* p_shapeTable, MechS32 p_shapeNum, MechU8* p_palette);
	MechS32 VFX_shape_colors(void* p_shapeTable, MechS32 p_shapeNum, MechU32* p_colors);
	MechS32 VFX_shape_set_colors(void* p_shapeTable, MechS32 p_shapeNumber, MechU32* p_colors);
	MechS32 VFX_shape_count(void* p_shapeTable);
	MechS32 VFX_shape_list(void* p_shapeTable, MechS32* p_indexList);
	MechS32 VFX_shape_palette_list(void* p_shapeTable, MechS32* p_indexList);
	void VFX_pane_wipe(PANE* p_pane, MechS32 p_color);
	MechS32 VFX_pane_copy(
		PANE* p_source,
		MechS32 p_sx,
		MechS32 p_sy,
		PANE* p_target,
		MechS32 p_tx,
		MechS32 p_ty,
		MechS32 p_fill
	);
	MechS32 VFX_pane_scroll(PANE* p_pane, MechS32 p_dx, MechS32 p_dy, MechS32 p_mode, MechS32 p_parm);
	void VFX_ellipse_draw(PANE* p_pane, MechS32 p_xc, MechS32 p_yc, MechS32 p_width, MechS32 p_height, MechS32 p_color);
	void VFX_ellipse_fill(PANE* p_pane, MechS32 p_xc, MechS32 p_yc, MechS32 p_width, MechS32 p_height, MechS32 p_color);
	void VFX_Cos_Sin(MechS32 p_angle, MechS32* p_cos, MechS32* p_sin);
	void VFX_fixed_mul(MechS32 p_m1, MechS32 p_m2, MechS32* p_result);
	void VFX_point_transform(
		MechS32* p_in,
		MechS32* p_out,
		MechS32* p_origin,
		MechS32 p_rot,
		MechS32 p_xScale,
		MechS32 p_yScale
	);
	MechS32 VFX_font_height(void* p_font);
	MechS32 VFX_character_width(void* p_font, MechS32 p_character);
	MechS32 VFX_character_draw(
		PANE* p_pane,
		MechS32 p_x,
		MechS32 p_y,
		void* p_font,
		MechS32 p_character,
		void* p_colorTranslate
	);
	void VFX_string_draw(
		PANE* p_pane,
		MechS32 p_x,
		MechS32 p_y,
		void* p_font,
		MechChar* p_string,
		void* p_colorTranslate
	);
	MechS32 VFX_line_to_pane(PANE* p_target, MechS32 p_y, MechU8* p_lineBuffer, MechS32 p_lineLength);
	MechU8* find_ILBM_property(MechChar* p_tag, MechU8* p_iff);
	MechS32 VFX_ILBM_draw(PANE* p_pane, MechU8* p_ilbmBuffer);
	void VFX_ILBM_palette(MechU8* p_ilbmBuffer, MechU8* p_palette);
	MechS32 VFX_ILBM_resolution(MechU8* p_ilbmBuffer);
	MechS32 VFX_PCX_draw(PANE* p_pane, MechU8* p_pcxBuffer);
	void VFX_PCX_palette(MechU8* p_pcxBuffer, MechS32 p_pcxFileSize, void* p_palette);
	MechS32 VFX_PCX_resolution(MechU8* p_pcxBuffer);
	MechS32 VFX_GIF_draw(PANE* p_pane, MechU8* p_gifBuffer, MechU8* p_gifScratch);
	void VFX_GIF_palette(MechU8* p_gifBuffer, MechU8* p_palette);
	MechS32 VFX_GIF_resolution(void* p_gifBuffer);
	MechS32 VFX_shape_transform(
		PANE* p_pane,
		void* p_shapeTable,
		MechS32 p_shapeNumber,
		MechS32 p_hotX,
		MechS32 p_hotY,
		MechU8* p_buffer,
		MechS32 p_rot,
		MechS32 p_xScale,
		MechS32 p_yScale,
		MechU32 p_flags
	);
	MechS32 VFX_shape_scan(PANE* p_pane, MechU8 p_transparentColor, MechS32 p_hotX, MechS32 p_hotY, MechU8* p_buffer);
	MechS32 VFX_pixel_fade(PANE* p_source, PANE* p_destination, MechS32 p_intervals, MechS32 p_rnd);
	void VFX_window_fade(WINDOW* p_buffer, MechU8* p_palette, MechS32 p_intervals);
	MechS32 VFX_color_scan(PANE* p_pane, MechU32* p_colors);

#ifdef __cplusplus
}
#endif

// VFXA's routines and data. reccmp reads annotations from C sources only, so they're here, by
// name.

// FUNCTION: MW2SHELL 0x10032250
// FUNCTION: MW2 0x100604f4
// VFX_driver_name

// FUNCTION: MW2SHELL 0x10032279
// FUNCTION: MW2 0x1006051d
// VFX_register_driver

// FUNCTION: MW2SHELL 0x10032298
// FUNCTION: MW2 0x1006053c
// VFX_pixel_write

// FUNCTION: MW2SHELL 0x10032373
// FUNCTION: MW2 0x10060617
// VFX_pixel_read

// FUNCTION: MW2SHELL 0x10032449
// FUNCTION: MW2 0x100606ed
// VFX_line_draw

// FUNCTION: MW2SHELL 0x10032e4b
// FUNCTION: MW2 0x100610ef
// VFX_rectangle_hash

// FUNCTION: MW2SHELL 0x10032f84
// FUNCTION: MW2 0x10061228
// VFX_shape_draw

// FUNCTION: MW2SHELL 0x100333f8
// FUNCTION: MW2 0x1006169c
// DrawShapeUnclipped

// FUNCTION: MW2SHELL 0x100334fb
// FUNCTION: MW2 0x1006179f
// VFX_shape_lookaside

// FUNCTION: MW2SHELL 0x1003351a
// FUNCTION: MW2 0x100617be
// VFX_shape_translate_draw

// FUNCTION: MW2SHELL 0x10033980
// FUNCTION: MW2 0x10061c24
// XlatShapeUnclipped

// FUNCTION: MW2SHELL 0x10033a76
// FUNCTION: MW2 0x10061d1a
// VFX_shape_transform

// FUNCTION: MW2SHELL 0x10034622
// FUNCTION: MW2 0x100628c6
// VFX_shape_visible_rectangle

// FUNCTION: MW2SHELL 0x1003479a
// FUNCTION: MW2 0x10062a3e
// VFX_shape_scan

// FUNCTION: MW2SHELL 0x10034a1d
// FUNCTION: MW2 0x10062cc1
// VFX_shape_remap_colors

// FUNCTION: MW2SHELL 0x10034aaf
// FUNCTION: MW2 0x10062d53
// ScanLine

// FUNCTION: MW2SHELL 0x10034c38
// FUNCTION: MW2 0x10062edc
// FlushPacket

// FUNCTION: MW2SHELL 0x10034e15
// FUNCTION: MW2 0x100630b9
// VFX_pane_wipe

// FUNCTION: MW2SHELL 0x10034f18
// FUNCTION: MW2 0x100631bc
// VFX_pane_copy

// FUNCTION: MW2SHELL 0x100352b4
// FUNCTION: MW2 0x10063558
// VFX_pane_scroll

// FUNCTION: MW2SHELL 0x100354b1
// FUNCTION: MW2 0x10063755
// VFX_ellipse_draw

// FUNCTION: MW2SHELL 0x100357f2
// FUNCTION: MW2 0x10063a96
// VFX_ellipse_fill

// GLOBAL: MW2SHELL 0x10035af0
// GLOBAL: MW2 0x10063d94
// CosTable

// FUNCTION: MW2SHELL 0x10036904
// FUNCTION: MW2 0x10064ba8
// VFX_Cos_Sin

// FUNCTION: MW2SHELL 0x100369bc
// FUNCTION: MW2 0x10064c60
// VFX_fixed_mul

// FUNCTION: MW2SHELL 0x100369e2
// FUNCTION: MW2 0x10064c86
// VFX_point_transform

// FUNCTION: MW2SHELL 0x10036aa9
// FUNCTION: MW2 0x10064d4d
// VFX_font_height

// FUNCTION: MW2SHELL 0x10036abc
// FUNCTION: MW2 0x10064d60
// VFX_character_width

// FUNCTION: MW2SHELL 0x10036adc
// FUNCTION: MW2 0x10064d80
// VFX_character_draw

// FUNCTION: MW2SHELL 0x10036c67
// FUNCTION: MW2 0x10064f0b
// VFX_string_draw

// FUNCTION: MW2SHELL 0x10036c9e
// FUNCTION: MW2 0x10064f42
// VFX_line_to_pane

// GLOBAL: MW2SHELL 0x10036da1
// GLOBAL: MW2 0x10065045
// BMHD_prop

// GLOBAL: MW2SHELL 0x10036da5
// GLOBAL: MW2 0x10065049
// CMAP_prop

// GLOBAL: MW2SHELL 0x10036da9
// GLOBAL: MW2 0x1006504d
// BODY_prop

// FUNCTION: MW2SHELL 0x10036dad
// FUNCTION: MW2 0x10065051
// find_ILBM_property

// FUNCTION: MW2SHELL 0x10036def
// FUNCTION: MW2 0x10065093
// VFX_ILBM_draw

// FUNCTION: MW2SHELL 0x10036fb6
// FUNCTION: MW2 0x1006525a
// VFX_ILBM_palette

// FUNCTION: MW2SHELL 0x10036fe7
// FUNCTION: MW2 0x1006528b
// VFX_ILBM_resolution

// FUNCTION: MW2SHELL 0x10037014
// FUNCTION: MW2 0x100652b8
// VFX_PCX_draw

// FUNCTION: MW2SHELL 0x10037096
// FUNCTION: MW2 0x1006533a
// VFX_PCX_palette

// FUNCTION: MW2SHELL 0x100370c1
// FUNCTION: MW2 0x10065365
// VFX_PCX_resolution

// FUNCTION: MW2SHELL 0x100370e8
// FUNCTION: MW2 0x1006538c
// GIF_init_codetable

// FUNCTION: MW2SHELL 0x10037130
// FUNCTION: MW2 0x100653d4
// GIF_getb

// FUNCTION: MW2SHELL 0x10037149
// FUNCTION: MW2 0x100653ed
// GIF_getbcode

// FUNCTION: MW2SHELL 0x1003718f
// FUNCTION: MW2 0x10065433
// GIF_insertcode

// FUNCTION: MW2SHELL 0x100371d5
// FUNCTION: MW2 0x10065479
// GIF_dopixel

// FUNCTION: MW2SHELL 0x10037252
// FUNCTION: MW2 0x100654f6
// VFX_GIF_draw

// FUNCTION: MW2SHELL 0x1003746b
// FUNCTION: MW2 0x1006570f
// VFX_GIF_palette

// FUNCTION: MW2SHELL 0x100374cc
// FUNCTION: MW2 0x10065770
// VFX_GIF_resolution

// FUNCTION: MW2SHELL 0x10037504
// FUNCTION: MW2 0x100657a8
// VFX_shape_bounds

// FUNCTION: MW2SHELL 0x10037526
// FUNCTION: MW2 0x100657ca
// VFX_shape_origin

// FUNCTION: MW2SHELL 0x10037549
// FUNCTION: MW2 0x100657ed
// VFX_shape_resolution

// FUNCTION: MW2SHELL 0x1003757d
// FUNCTION: MW2 0x10065821
// VFX_shape_minxy

// FUNCTION: MW2SHELL 0x100375a7
// FUNCTION: MW2 0x1006584b
// VFX_shape_palette

// FUNCTION: MW2SHELL 0x100375f2
// FUNCTION: MW2 0x10065896
// VFX_shape_colors

// FUNCTION: MW2SHELL 0x1003763a
// FUNCTION: MW2 0x100658de
// VFX_shape_set_colors

// FUNCTION: MW2SHELL 0x10037684
// FUNCTION: MW2 0x10065928
// VFX_shape_count

// FUNCTION: MW2SHELL 0x10037697
// FUNCTION: MW2 0x1006593b
// VFX_shape_list

// FUNCTION: MW2SHELL 0x100376f9
// FUNCTION: MW2 0x1006599d
// VFX_shape_palette_list

// GLOBAL: MW2SHELL 0x1003775b
// GLOBAL: MW2 0x100659ff
// pf_constants

// FUNCTION: MW2SHELL 0x100377d7
// FUNCTION: MW2 0x10065a7b
// VFX_pixel_fade

// FUNCTION: MW2SHELL 0x10037a4e
// FUNCTION: MW2 0x10065cf2
// VFX_window_fade

// FUNCTION: MW2SHELL 0x10037bd2
// FUNCTION: MW2 0x10065e76
// VFX_color_scan

// GLOBAL: MW2SHELL 0x100687cc
// GLOBAL: MW2 0x100ab100
// VFX_describe_driver

// GLOBAL: MW2SHELL 0x100687d0
// GLOBAL: MW2 0x100ab104
// VFX_init_driver

// GLOBAL: MW2SHELL 0x100687d4
// GLOBAL: MW2 0x100ab108
// VFX_shutdown_driver

// GLOBAL: MW2SHELL 0x100687d8
// GLOBAL: MW2 0x100ab10c
// VFX_area_wipe

// GLOBAL: MW2SHELL 0x100687dc
// GLOBAL: MW2 0x100ab110
// VFX_wait_vblank

// GLOBAL: MW2SHELL 0x100687e0
// GLOBAL: MW2 0x100ab114
// VFX_wait_vblank_leading

// GLOBAL: MW2SHELL 0x100687e4
// GLOBAL: MW2 0x100ab118
// VFX_window_refresh

// GLOBAL: MW2SHELL 0x100687e8
// GLOBAL: MW2 0x100ab11c
// VFX_window_read

// GLOBAL: MW2SHELL 0x100687ec
// GLOBAL: MW2 0x100ab120
// VFX_DAC_read

// GLOBAL: MW2SHELL 0x100687f0
// GLOBAL: MW2 0x100ab124
// VFX_DAC_write

// GLOBAL: MW2SHELL 0x100687f4
// GLOBAL: MW2 0x100ab128
// VFX_bank_reset

// GLOBAL: MW2SHELL 0x100687f8
// GLOBAL: MW2 0x100ab12c
// VFX_pane_refresh

// GLOBAL: MW2SHELL 0x100687fc
// GLOBAL: MW2 0x100ab130
// VFX_line_address

// GLOBAL: MW2SHELL 0x10068800
// GLOBAL: MW2 0x100ab134
// driver_name

#endif // VFXA_H
