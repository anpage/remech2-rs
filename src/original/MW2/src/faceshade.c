#include "faceshade.h"

#include "compat.h"
#include "decomp.h"
#include "face.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "objectanim.h"
#include "polydraw.h"
#include "ray.h"
#include "rendersettings.h"
#include "shape.h"
#include "transform.h"
#include "types.h"
#include "vertex.h"

// Set by the world stream (BwdExecuteStream): GetFaceColor brightens damaged shapes instead of
// dimming them.
// GLOBAL: MW2 0x100a555c
MechS32 g_brightenDamage = 0;

// The eyepoint's ambient light (SelectEyepoint): ComputeShade's base shade, out of 0x80.
// GLOBAL: MW2 0x1010b540
MechS32 g_ambientLight;

MechS32 ComputeShade(MechS32 p_light, MechS32 p_value, MechS32 p_distance);

// Returns the color word of face p_face for the draw mode in bits 12-14 of p_color: its shade
// (ComputeShade, from the face's light and p_distance) with the color bits of p_color, or in the
// wireframe views (m_wireframe) a fixed color by the shape's type. Textured faces of the kinds in
// m_untexturedKinds are drawn in a flat color instead.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x10036230
MechU32 GetFaceColor(Face* p_face, Vertex* p_vertices, MechU32 p_color, MechS32 p_distance)
{
	MechS32 shade;
	MechU32 kind;
	MechU32 high;
	MechU32 low;
	MechS32 mode;
	MechS32 value;
	MechU32 result;
	MechS32 level;
	MechS32 detail;

	low = 0;
	shade = -1;
	mode = p_color & 0x7000;
	kind = p_face->m_shape->m_kind;
	if (g_renderSettings.m_wireframe) {
		if (g_renderSettings.m_wireframeColors == 1) {
			switch (p_face->m_shape->m_collisionType) {
			case 0:
				return 0xd;
			case 1:
				return 3;
			case 2:
				return 6;
			case 3:
				return 2;
			case 5:
				return 8;
			case 6:
				return 7;
			case 7:
				return 0xf;
			default:
				return 0xff;
			}
		}
		else if (g_renderSettings.m_wireframeColors == 2) {
			switch (p_face->m_shape->m_flags & 0x10f) {
			case 0:
				return 0xd;
			case 1:
				return 6;
			case 2:
				return 8;
			case 4:
				return 1;
			case 0x100:
				return 0xf;
			case 0x101:
				return 7;
			case 0x102:
				return 0xb;
			case 0x104:
				return 3;
			default:
				return 0xff;
			}
		}
		else if (kind & 0x200) {
			return 7;
		}
		else if (kind & 0x400) {
			return 0xb;
		}
		else if (kind & 0x100) {
			level = (p_face->m_shape->m_flags & 0xf0) >> 4;
			if (level < 1) {
				return 7;
			}
			else if (level < 12) {
				return 3;
			}
			else {
				return 0xb;
			}
		}
		else {
			return 8;
		}
	}

	switch (mode) {
	case 0:
		value = (p_color & 0xf0) >> 4;
		if (g_renderSettings.m_greyscale) {
			value >>= 2;
			high = 0xf0;
		}
		else {
			high = (p_color & 0xf00) >> 4;
		}

		p_color = high | value;
		return p_color;
	case 0x2000:
		result = (p_color & 0xff0) >> 4;
		return result | mode;
	case 0x3000:
		if (g_renderSettings.m_untexturedKinds) {
			if (kind & g_renderSettings.m_untexturedKinds) {
				mode = 0x1000;
				value = p_color & 0xf0;
			}
			else {
				return p_color;
			}
		}
		else {
			return p_color;
		}
		break;
	case 0x5000:
	case 0x6000:
	case 0x7000:
		value = 0xff;
		if (g_renderSettings.m_untexturedKinds) {
			if ((g_renderSettings.m_untexturedKinds & 0x100) && ((kind & 0x100) || (kind & 0xf0) == 0x50)) {
				mode = 0x1000;
				value = 0xa0;
			}
			else if (kind & g_renderSettings.m_untexturedKinds) {
				mode = 0x1000;
				if (kind & 0x200) {
					value = 0xd0;
				}
				else if (kind & 0x400) {
					value = 0xd0;
				}
				else {
					value = 0xd0;
				}
			}
			else {
				low = p_color & 0xff;
			}
		}
		else {
			low = p_color & 0xff;
		}
		break;
	default:
		value = p_color & 0xff;
		break;
	}

	shade = ComputeShade(GetFaceShade(p_face, p_vertices), value, p_distance);
	if ((kind & 0x100) || (kind & 0xf0) == 0x50) {
		detail = (p_face->m_shape->m_flags & 0xf0) >> 4;
		if (detail > 0) {
			if (g_brightenDamage) {
				shade += FixedMul16(detail, FixedDiv16(15 - shade, 15));
			}
			else {
				detail = (16 - detail) << 12;
				shade = FixedMul16(detail, shade) & 0xf;
			}
		}
	}

	if (mode == 0x7000 || mode == 0x5000 || mode == 0x6000) {
		shade <<= 8;
		high = 0;
	}
	else if (g_renderSettings.m_greyscale) {
		high = 0xf0;
	}
	else {
		high = (p_color & 0xf00) >> 4;
	}

	result = shade | high | low | mode;
	return result;
}

// Returns a shade from 0 to 15 for a light level p_light (out of 0x80) and a brightness
// p_value, dimmed with the distance p_distance.
// FUNCTION: MW2 0x100367c5
MechS32 ComputeShade(MechS32 p_light, MechS32 p_value, MechS32 p_distance)
{
	MechS32 shade;

	shade = (((0x80 - g_ambientLight) * p_light >> 7) + g_ambientLight) * (p_value >> 1) / 0x440;
	if (g_renderSettings.m_fadeDistance) {
		shade -= (p_distance << 4) / g_renderSettings.m_fadeDistance >> 4;
	}

	if (shade < 1) {
		shade = 0;
	}
	else if (shade > 15) {
		shade = 15;
	}

	return shade;
}

// Switches the texture maps of the shape kinds in p_flags on or off (the TOGGLE_..._TEXT_MAPS
// keys).
// FUNCTION: MW2 0x10036853
void ToggleTextureMaps(MechU32 p_flags)
{
	g_renderSettings.m_untexturedKinds ^= p_flags;
}

// FUNCTION: MW2 0x10036867
MechS32 AreTextureMapsOn(MechU32 p_flags)
{
	return !(p_flags & g_renderSettings.m_untexturedKinds);
}

// FUNCTION: MW2 0x10036891
void EnableTextureMaps(MechU32 p_flags, MechS32 p_enable)
{
	if (p_enable) {
		g_renderSettings.m_untexturedKinds &= ~p_flags;
	}
	else {
		g_renderSettings.m_untexturedKinds |= p_flags;
	}
}

// The display detail setting: textures with perspective correction.
// FUNCTION: MW2 0x100368bf
MechS32 ArePerspectiveTexturesOn(undefined4 p_unk0x00)
{
	return !g_renderSettings.m_affineTextures;
}

// FUNCTION: MW2 0x100368e8
void EnablePerspectiveTextures(undefined4 p_unk0x00, MechS32 p_enable)
{
	if (!p_enable) {
		g_renderSettings.m_affineTextures = TRUE;
	}
	else {
		g_renderSettings.m_affineTextures = FALSE;
	}
}
