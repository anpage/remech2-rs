#ifndef VFXA_H
#define VFXA_H

#include "decomp.h"
#include "pane.h"
#include "types.h"
#include "window.h"

// VFXA, Miles Design VFX's 2D primitives, reimplemented in C (common/src/vfxa.c), which both DLLs
// link.
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

#endif // VFXA_H
