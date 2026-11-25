#ifndef VFXA_H
#define VFXA_H

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
	extern void (*VFX_area_wipe)(int32_t p_x0, int32_t p_y0, int32_t p_x1, int32_t p_y1, int32_t p_color);
	extern void (*VFX_wait_vblank)(void);
	extern void (*VFX_wait_vblank_leading)(void);
	extern void (*VFX_window_refresh)(WINDOW* p_target, int32_t p_x0, int32_t p_y0, int32_t p_x1, int32_t p_y1);
	extern void (*VFX_window_read)(WINDOW* p_destination, int32_t p_x0, int32_t p_y0, int32_t p_x1, int32_t p_y1);
	extern void (*VFX_DAC_read)(int32_t p_colorNumber, uint8_t* p_triplet);
	extern void (*VFX_DAC_write)(int32_t p_colorNumber, uint8_t* p_triplet);
	extern void (*VFX_bank_reset)(void);
	extern void (*VFX_pane_refresh)(PANE* p_target, int32_t p_x0, int32_t p_y0, int32_t p_x1, int32_t p_y1);
	extern void (*VFX_line_address)(int32_t p_x, int32_t p_y, uint8_t** p_addr, uint32_t* p_nbytes);

	char* VFX_driver_name(char* (**p_vfxScanDll)(void) );
	void VFX_register_driver(char* (**p_dllBase)(void) );
	int32_t VFX_pixel_write(PANE* p_pane, int32_t p_x, int32_t p_y, uint32_t p_color);
	int32_t VFX_pixel_read(PANE* p_pane, int32_t p_x, int32_t p_y);
	int32_t VFX_line_draw(
		PANE* p_pane,
		int32_t p_x0,
		int32_t p_y0,
		int32_t p_x1,
		int32_t p_y1,
		int32_t p_mode,
		int32_t p_parm
	);
	void VFX_rectangle_hash(PANE* p_pane, int32_t p_x0, int32_t p_y0, int32_t p_x1, int32_t p_y1, uint8_t p_color);
	void DrawShapeUnclipped(PANE* p_pane, void* p_shape, int32_t p_hotX, int32_t p_hotY, uint32_t p_cpW);
	int32_t XlatShapeUnclipped(PANE* p_pane, void* p_shape, int32_t p_hotX, int32_t p_hotY, uint32_t p_cpW);
	void VFX_shape_draw(PANE* p_pane, void* p_shapeTable, int32_t p_shapeNumber, int32_t p_hotX, int32_t p_hotY);
	void VFX_shape_lookaside(uint8_t* p_table);
	int32_t VFX_shape_translate_draw(
		PANE* p_pane,
		void* p_shapeTable,
		int32_t p_shapeNumber,
		int32_t p_hotX,
		int32_t p_hotY
	);
	int32_t VFX_shape_remap_colors(void* p_shapeTable, int32_t p_shapeNumber);
	int32_t VFX_shape_visible_rectangle(
		void* p_shapeTable,
		int32_t p_shapeNumber,
		int32_t p_hotX,
		int32_t p_hotY,
		uint32_t p_mirror,
		int32_t* p_rectangle
	);
	int32_t VFX_shape_bounds(void* p_shapeTable, int32_t p_shapeNum);
	int32_t VFX_shape_origin(void* p_shapeTable, int32_t p_shapeNum);
	int32_t VFX_shape_resolution(void* p_shapeTable, int32_t p_shapeNum);
	int32_t VFX_shape_minxy(void* p_shapeTable, int32_t p_shapeNum);
	void VFX_shape_palette(void* p_shapeTable, int32_t p_shapeNum, uint8_t* p_palette);
	int32_t VFX_shape_colors(void* p_shapeTable, int32_t p_shapeNum, uint32_t* p_colors);
	int32_t VFX_shape_set_colors(void* p_shapeTable, int32_t p_shapeNumber, uint32_t* p_colors);
	int32_t VFX_shape_count(void* p_shapeTable);
	int32_t VFX_shape_list(void* p_shapeTable, int32_t* p_indexList);
	int32_t VFX_shape_palette_list(void* p_shapeTable, int32_t* p_indexList);
	void VFX_pane_wipe(PANE* p_pane, int32_t p_color);
	int32_t VFX_pane_copy(
		PANE* p_source,
		int32_t p_sx,
		int32_t p_sy,
		PANE* p_target,
		int32_t p_tx,
		int32_t p_ty,
		int32_t p_fill
	);
	int32_t VFX_pane_scroll(PANE* p_pane, int32_t p_dx, int32_t p_dy, int32_t p_mode, int32_t p_parm);
	void VFX_ellipse_draw(PANE* p_pane, int32_t p_xc, int32_t p_yc, int32_t p_width, int32_t p_height, int32_t p_color);
	void VFX_ellipse_fill(PANE* p_pane, int32_t p_xc, int32_t p_yc, int32_t p_width, int32_t p_height, int32_t p_color);
	void VFX_Cos_Sin(int32_t p_angle, int32_t* p_cos, int32_t* p_sin);
	void VFX_fixed_mul(int32_t p_m1, int32_t p_m2, int32_t* p_result);
	void VFX_point_transform(
		int32_t* p_in,
		int32_t* p_out,
		int32_t* p_origin,
		int32_t p_rot,
		int32_t p_xScale,
		int32_t p_yScale
	);
	int32_t VFX_font_height(void* p_font);
	int32_t VFX_character_width(void* p_font, int32_t p_character);
	int32_t VFX_character_draw(
		PANE* p_pane,
		int32_t p_x,
		int32_t p_y,
		void* p_font,
		int32_t p_character,
		void* p_colorTranslate
	);
	void VFX_string_draw(PANE* p_pane, int32_t p_x, int32_t p_y, void* p_font, char* p_string, void* p_colorTranslate);
	int32_t VFX_line_to_pane(PANE* p_target, int32_t p_y, uint8_t* p_lineBuffer, int32_t p_lineLength);
	uint8_t* find_ILBM_property(char* p_tag, uint8_t* p_iff);
	int32_t VFX_ILBM_draw(PANE* p_pane, uint8_t* p_ilbmBuffer);
	void VFX_ILBM_palette(uint8_t* p_ilbmBuffer, uint8_t* p_palette);
	int32_t VFX_ILBM_resolution(uint8_t* p_ilbmBuffer);
	int32_t VFX_PCX_draw(PANE* p_pane, uint8_t* p_pcxBuffer);
	void VFX_PCX_palette(uint8_t* p_pcxBuffer, int32_t p_pcxFileSize, void* p_palette);
	int32_t VFX_GIF_draw(PANE* p_pane, uint8_t* p_gifBuffer, uint8_t* p_gifScratch);
	void VFX_GIF_palette(uint8_t* p_gifBuffer, uint8_t* p_palette);
	int32_t VFX_GIF_resolution(void* p_gifBuffer);
	int32_t VFX_shape_transform(
		PANE* p_pane,
		void* p_shapeTable,
		int32_t p_shapeNumber,
		int32_t p_hotX,
		int32_t p_hotY,
		uint8_t* p_buffer,
		int32_t p_rot,
		int32_t p_xScale,
		int32_t p_yScale,
		uint32_t p_flags
	);
	int32_t VFX_shape_scan(PANE* p_pane, uint8_t p_transparentColor, int32_t p_hotX, int32_t p_hotY, uint8_t* p_buffer);
	int32_t VFX_pixel_fade(PANE* p_source, PANE* p_destination, int32_t p_intervals, int32_t p_rnd);
	void VFX_window_fade(WINDOW* p_buffer, uint8_t* p_palette, int32_t p_intervals);
	int32_t VFX_color_scan(PANE* p_pane, uint32_t* p_colors);

#ifdef __cplusplus
}
#endif

#endif // VFXA_H
