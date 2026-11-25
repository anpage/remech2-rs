#ifndef VFX3D_H
#define VFX3D_H

#include "pane.h"
#include "types.h"
#include "window.h"

// VFX3D, Miles Design VFX's polygon fillers, reimplemented in C (common/src/vfx3d.c), which both
// DLLs link.
#ifdef __cplusplus
extern "C"
{
#endif

	void VFX_flat_polygon(PANE* p_pane, int32_t p_vcnt, uint32_t* p_vlist);
	void VFX_Gouraud_polygon(PANE* p_pane, int32_t p_vcnt, uint32_t* p_vlist);
	void VFX_dithered_Gouraud_polygon(PANE* p_pane, int32_t p_ditherAmount, int32_t p_vcnt, uint32_t* p_vlist);
	void VFX_illuminate_polygon(PANE* p_pane, int32_t p_ditherAmount, int32_t p_vcnt, uint32_t* p_vlist);
	void VFX_translate_polygon(PANE* p_pane, int32_t p_vcnt, uint32_t* p_vlist, uint8_t* p_lookaside);
	void VFX_map_lookaside(uint16_t* p_table);
	void VFX_map_polygon(PANE* p_pane, int32_t p_vcnt, uint32_t* p_vlist, WINDOW* p_texture, int32_t p_flags);

#ifdef __cplusplus
}
#endif

#endif // VFX3D_H
