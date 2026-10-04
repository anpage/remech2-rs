#include "texpoly.h"

#include "decomp.h"
#include "polydraw.h"
#include "types.h"
#include "vfxrend.h"

DECOMP_SIZE_ASSERT(VFX_TEXTURE, 0xc)

// Draws a polygon of p_count points (six dwords each) textured with the p_width by p_height
// bitmap p_pixels, through the code block's routines.
// The p_width/p_height comparison loads its operands in the opposite order (one attempt at
// swapping them didn't flip it), and stack-slot permutation: i, points, rows and unk0x08.
// FUNCTION: MW2 0x1006dd50
void DrawTexturedPolygon(
	PANE* p_target,
	MechU8* p_pixels,
	MechS16 p_width,
	MechS16 p_height,
	MechS32 p_count,
	MechU32* p_points,
	MechS32 p_useLuma,
	MechU16* p_luma
)
{
	VFX_TEXTURE texture;
	MechS32 mode;
	MechU8* rows[128];
	MechS32 i;
	MechU32* points;
	MechU32 unk0x08;

	mode = 0x11;
	p_useLuma = TRUE;
	if (p_useLuma) {
		mode |= 0x80;
	}

	if (g_renderSettings.m_affineTextures) {
		mode |= 0x400;
	}
	else {
		mode |= 0x600;
	}

	if (p_height > 0x80) {
		p_height = 0x80;
	}

	if (p_width < p_height) {
		p_height = p_width;
	}

	for (i = 0; i < p_height; i++) {
		rows[i] = p_pixels;
		p_pixels += p_width;
	}

	texture.m_width = texture.m_height = p_height;
	texture.m_vAddrs = rows;
	points = p_points;
	unk0x08 = points[2];
	VFX_polygon_clip_XY_and_render(p_target, p_points, p_count, mode, unk0x08, &texture, p_luma, 0);
}
