#ifndef VFX3D_H
#define VFX3D_H

#include "decomp.h"
#include "pane.h"
#include "types.h"
#include "window.h"

// VFX3D, Miles Design VFX's polygon fillers, reimplemented in C (common/src/vfx3d.c), which both
// DLLs link.
#ifdef __cplusplus
extern "C"
{
#endif

	void VFX_flat_polygon(PANE* p_pane, MechS32 p_vcnt, MechU32* p_vlist);
	void VFX_Gouraud_polygon(PANE* p_pane, MechS32 p_vcnt, MechU32* p_vlist);
	void VFX_dithered_Gouraud_polygon(PANE* p_pane, MechS32 p_ditherAmount, MechS32 p_vcnt, MechU32* p_vlist);
	void VFX_illuminate_polygon(PANE* p_pane, MechS32 p_ditherAmount, MechS32 p_vcnt, MechU32* p_vlist);
	void VFX_translate_polygon(PANE* p_pane, MechS32 p_vcnt, MechU32* p_vlist, MechU8* p_lookaside);
	void VFX_map_lookaside(MechU16* p_table);
	void VFX_map_polygon(PANE* p_pane, MechS32 p_vcnt, MechU32* p_vlist, WINDOW* p_texture, MechS32 p_flags);

#ifdef __cplusplus
}
#endif

#endif // VFX3D_H
