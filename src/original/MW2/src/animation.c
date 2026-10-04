#include "animation.h"

#include "clock.h"
#include "decomp.h"
#include "loadres.h"
#include "mw2prj.h"
#include "render.h"
#include "resource.h"
#include "texpoly.h"
#include "types.h"
#include "vfx3d.h"
#include "window.h"

// Animated textures: up to 0x200 animations, each playing one of 0x200 sets of up to 0x20 CEL
// frames. The frames load on first use; each animation advances with the clock.

DECOMP_SIZE_ASSERT(AnimFrame, 0x08)
DECOMP_SIZE_ASSERT(Animation, 0x0e)

// GLOBAL: MW2 0x100ad288
MechS32 g_animInitialized = -2;

// GLOBAL: MW2 0x100ad28c
MechS32 g_lumaResourceId = 0;

// GLOBAL: MW2 0x100ad290
MechS32 g_currentAnimSet = 0;

// GLOBAL: MW2 0x100ad294
MechS32 g_animSetUsed = 0;

// GLOBAL: MW2 0x100ad298
MechS32 g_animSetIsSequence = 0;

// GLOBAL: MW2 0x100ad29c
MechU16* g_lumaTables = NULL;

// GLOBAL: MW2 0x100ad2a0
MechS32 g_preloadCels[] = {
	562, 563, 564, 565, 566, 567, 568, 569, 570, 571, 572, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582,
	583, 584, 585, 586, 587, 588, 589, 590, 591, 592, 593, 88,  89,  90,  91,  92,  93,  94,  95,  96,  97,
	98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
	119, 428, 429, 430, 431, 432, 433, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237,
	238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, -1,
};

// GLOBAL: MW2 0x100c7610
AnimFrame g_animFrames[0x200][0x20];

// GLOBAL: MW2 0x100e7610
Animation g_animations[0x200];

// GLOBAL: MW2 0x100e9210
WINDOW g_animFrameBuffer;

// Stack-slot permutation: anim, data, height, i, luma, mode and useLuma and width.
// FUNCTION: MW2 0x10068d10
MechS32 DrawAnimatedPolygon(
	MechS32 p_index,
	MechS32 p_count,
	MechU32* p_points,
	MechS32 p_luma,
	MechS32 p_scale,
	MechS32 p_direct
)
{
	MechS16 height;
	MechS32 i;
	MechS32 useLuma;
	AnimFrame* frame;
	MechU16* data;
	Animation* anim;
	MechS16 width;
	MechU16* luma;
	MechS32 mode;

	data = NULL;
	luma = NULL;
	mode = 2;
	if (p_count < 3) {
		return 0;
	}

	if (!p_scale) {
		p_index += 0x100;
	}

	anim = &g_animations[p_index];
	if (anim->m_mode == 0 || anim->m_flags < 0) {
		return 0;
	}

	frame = &g_animFrames[anim->m_set][anim->m_frame];
	if (frame->m_resourceId < 1) {
		return 0;
	}

	frame->m_useCount++;
	data = frame->m_data;
	if (!data) {
		data = LoadCachedResource(g_mw2PrjHandle, frame->m_resourceId, g_resourceTypeTags[c_resTagCel], 0);
		if (!data) {
			return 0;
		}

		frame->m_data = data;
	}

	width = data[0];
	height = data[1];
	data += 2;
	if (p_scale) {
		for (i = 0; i < 4; i++) {
			p_points[i * 6 + 3] *= (MechS16) (width - 1);
			p_points[i * 6 + 4] *= (MechS16) (height - 1);
		}
	}

	if (!(anim->m_flags & 4) && p_luma > -1 && p_luma < 15) {
		useLuma = TRUE;
	}
	else {
		useLuma = FALSE;
	}

	if (!g_lumaTables) {
		g_lumaTables = LoadCachedResource(g_mw2PrjHandle, g_lumaResourceId, g_resourceTypeTags[c_resTagLuma], 0);
	}

	if (p_direct) {
		luma = g_lumaTables + p_luma * 0x80;
		DrawTexturedPolygon(&g_currentPane, (MechU8*) data, width, height, p_count, p_points, useLuma, luma);
	}
	else {
		if (useLuma) {
			luma = g_lumaTables + p_luma * 0x80;
			VFX_map_lookaside(luma);
			mode |= 1;
		}

		g_animFrameBuffer.m_buffer = (undefined*) data;
		g_animFrameBuffer.m_xMax = width - 1;
		g_animFrameBuffer.m_yMax = height - 1;
		VFX_map_polygon(&g_currentPane, p_count, p_points, &g_animFrameBuffer, mode);
	}

	return 1;
}

// Stack-slot permutation: frame, i and now and remainder.
// FUNCTION: MW2 0x10068fb8
void AdvanceAnimations(void)
{
	MechS32 remainder;
	MechS32 j;
	MechS32 now;
	MechS32 i;
	MechU32 frame;
	MechS32 steps;
	Animation* anim;
	MechS32 elapsed;

	now = g_currentClock;
	for (i = 0; i < 0x200; i++) {
		anim = &g_animations[i];
		if (anim->m_flags >= 0 && anim->m_mode != 0 && anim->m_delay > 0) {
			if (anim->m_lastTime == 0) {
				anim->m_lastTime = now;
			}

			elapsed = now - anim->m_lastTime;
			steps = elapsed / anim->m_delay;
			remainder = elapsed - anim->m_delay * steps;
			frame = anim->m_frame;
			if (steps > 0) {
				for (j = 0; j < steps; j++) {
					frame++;
					frame &= 0x1f;
					if (g_animFrames[anim->m_set][frame].m_resourceId < 1) {
						frame = 0;
					}
				}

				if (anim->m_mode == 2 && anim->m_frame > frame) {
					anim->m_mode = 0;
					frame = 0;
				}

				anim->m_frame = frame;
				anim->m_lastTime = now - remainder;
			}
		}
	}
}

// Stack-slot permutation: found and slot.
// FUNCTION: MW2 0x10069124
MechS32 AddAnimFrame(MechS32 p_resourceId, MechS32 p_set)
{
	MechS32 found;
	MechS32 slot;

	slot = 0;
	found = FALSE;
	if (g_animInitialized == -2) {
		InitAnimations();
	}

	if (p_set == -1) {
		if (g_animSetUsed) {
			g_currentAnimSet++;
			g_animSetUsed = 0;
		}

		p_set = g_currentAnimSet;
	}

	if (p_resourceId <= 0 || p_set < 0 || p_set >= 0x200) {
		return -1;
	}

	if (g_animFrames[p_set][0].m_resourceId > 0) {
		g_animSetIsSequence = 1;
		while (!found) {
			slot++;
			if (slot >= 0x20) {
				return -1;
			}

			if (g_animFrames[p_set][slot].m_resourceId == -1) {
				found = TRUE;
			}
		}
	}
	else {
		g_animSetIsSequence = 0;
	}

	g_animFrames[p_set][slot].m_resourceId = p_resourceId;
	return 0;
}

// FUNCTION: MW2 0x1006923c
void PreloadAnimCels(void)
{
	MechS32 i;

	for (i = 0; g_preloadCels[i] != -1; i++) {
		PreloadResource(g_preloadCels[i], g_resourceTypeTags[c_resTagCel]);
	}
}

// FUNCTION: MW2 0x10069288
MechS32 StartAnimation(MechS32 p_index, MechS32 p_set)
{
	if (p_set == -1) {
		p_set = g_currentAnimSet;
		g_animSetUsed = 1;
	}

	if (p_set < 0 || p_set >= 0x200 || p_index < 0 || p_index >= 0x200) {
		return 0;
	}

	g_animations[p_index].m_set = p_set;
	g_animations[p_index].m_flags = 1;
	g_animations[p_index].m_mode = 1;
	g_animations[p_index].m_frame = 0;
	if (g_animSetIsSequence) {
		g_animations[p_index].m_delay = 0x2d;
	}

	return 1;
}

// FUNCTION: MW2 0x10069360
void InitAnimations(void)
{
	MechS32 j;
	MechS32 i;
	AnimFrame* frame;
	Animation* anim;

	for (i = 0; i < 0x200; i++) {
		for (j = 0; j < 0x20; j++) {
			frame = &g_animFrames[i][j];
			frame->m_resourceId = -1;
			frame->m_useCount = 0;
			frame->m_data = NULL;
		}
	}

	for (i = 0; i < 0x200; i++) {
		anim = &g_animations[i];
		anim->m_set = -1;
		anim->m_frame = 0;
		anim->m_delay = 0;
		anim->m_mode = 0;
		anim->m_flags = -2;
		anim->m_lastTime = -1;
	}

	g_animInitialized = 0;
	g_currentAnimSet = 0;
	g_animSetUsed = 0;
	g_animSetIsSequence = 0;
}

// FUNCTION: MW2 0x1006946f
void SetAnimMode(MechS16 p_index, MechS16 p_mode)
{
	if (g_animations[p_index].m_flags != -2) {
		g_animations[p_index].m_mode = p_mode;
	}

	if (g_animations[p_index].m_delay == 0) {
		g_animations[p_index].m_delay = 0x2d;
	}
}

// FUNCTION: MW2 0x100694df
void SetAnimFrame(MechS16 p_index, MechU16 p_frame)
{
	Animation* anim;

	if (p_frame >= 0x20) {
		p_frame = 0;
	}

	anim = &g_animations[p_index];
	if (anim->m_flags < 0) {
		return;
	}

	if (g_animFrames[anim->m_set][p_frame].m_resourceId > 0) {
		anim->m_frame = p_frame;
		anim->m_lastTime = 0;
	}
}

// FUNCTION: MW2 0x10069564
void SetAnimDelay(MechS16 p_index, MechU16 p_delay)
{
	g_animations[p_index].m_delay = p_delay;
}

// FUNCTION: MW2 0x10069586
void FUN_10069586(void)
{
}

// FUNCTION: MW2 0x10069591
void FUN_10069591(void)
{
}

// Index order: &g_animFrames[i][j] loads i first in the original (InitAnimations matches with the
// same statement).
// FUNCTION: MW2 0x1006959c
void FreeAnimations(void)
{
	MechS32 j;
	MechS32 i;
	AnimFrame* frame;
	Animation* anim;

	for (i = 0; i < 0x200; i++) {
		for (j = 0; j < 0x20; j++) {
			frame = &g_animFrames[i][j];
			if (frame->m_data) {
				UnlockCachedResource(frame->m_resourceId, g_resourceTypeTags[c_resTagCel]);
				frame->m_data = NULL;
			}

			frame->m_resourceId = 0;
			frame->m_useCount = 0;
		}
	}

	UnlockCachedResource(g_lumaResourceId, g_resourceTypeTags[c_resTagLuma]);
	for (i = 0; i < 0x200; i++) {
		anim = &g_animations[i];
		anim->m_set = -1;
		anim->m_frame = 0;
		anim->m_delay = 0;
		anim->m_mode = 0;
		anim->m_flags = -2;
		anim->m_lastTime = -1;
	}
}
