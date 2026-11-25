#include "texpoly.h"

#include "polydraw.h"
#include "types.h"
#include "vfxrend.h"

// Draws a polygon of p_count points (six dwords each) textured with the p_width by p_height
// bitmap p_pixels, through the code block's routines.
void DrawTexturedPolygon(
	PANE* p_target,
	uint8_t* p_pixels,
	int16_t p_width,
	int16_t p_height,
	int32_t p_count,
	uint32_t* p_points,
	int32_t p_useLuma,
	uint16_t* p_luma
)
{
	VFX_TEXTURE texture;
	int32_t mode;
	uint8_t* rows[128];
	int32_t i;
	uint32_t* points;
	uint32_t unk0x08;

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
