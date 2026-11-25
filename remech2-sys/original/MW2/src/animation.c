#include "animation.h"

#include "clock.h"
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

int32_t g_animInitialized = -2;

int32_t g_lumaResourceId = 0;

int32_t g_currentAnimSet = 0;

int32_t g_animSetUsed = 0;

int32_t g_animSetIsSequence = 0;

uint16_t* g_lumaTables = NULL;

int32_t g_preloadCels[] = {
	562, 563, 564, 565, 566, 567, 568, 569, 570, 571, 572, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582,
	583, 584, 585, 586, 587, 588, 589, 590, 591, 592, 593, 88,  89,  90,  91,  92,  93,  94,  95,  96,  97,
	98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
	119, 428, 429, 430, 431, 432, 433, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237,
	238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, -1,
};

AnimFrame g_animFrames[0x200][0x20];

Animation g_animations[0x200];

WINDOW g_animFrameBuffer;

int32_t DrawAnimatedPolygon(
	int32_t p_index,
	int32_t p_count,
	uint32_t* p_points,
	int32_t p_luma,
	int32_t p_scale,
	int32_t p_direct
)
{
	int16_t height;
	int32_t i;
	int32_t useLuma;
	AnimFrame* frame;
	uint16_t* data;
	Animation* anim;
	int16_t width;
	uint16_t* luma;
	int32_t mode;

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
		data = LoadCachedResource(g_mw2PrjHandle, frame->m_resourceId, g_resourceTypeTags[c_resTagCel]);
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
			p_points[i * 6 + 3] *= (int16_t) (width - 1);
			p_points[i * 6 + 4] *= (int16_t) (height - 1);
		}
	}

	if (!(anim->m_flags & 4) && p_luma > -1 && p_luma < 15) {
		useLuma = true;
	}
	else {
		useLuma = false;
	}

	if (!g_lumaTables) {
		g_lumaTables = LoadCachedResource(g_mw2PrjHandle, g_lumaResourceId, g_resourceTypeTags[c_resTagLuma]);
	}

	if (p_direct) {
		luma = g_lumaTables + p_luma * 0x80;
		DrawTexturedPolygon(&g_currentPane, (uint8_t*) data, width, height, p_count, p_points, useLuma, luma);
	}
	else {
		if (useLuma) {
			luma = g_lumaTables + p_luma * 0x80;
			VFX_map_lookaside(luma);
			mode |= 1;
		}

		g_animFrameBuffer.m_buffer = (uint8_t*) data;
		g_animFrameBuffer.m_xMax = width - 1;
		g_animFrameBuffer.m_yMax = height - 1;
		VFX_map_polygon(&g_currentPane, p_count, p_points, &g_animFrameBuffer, mode);
	}

	return 1;
}

void AdvanceAnimations(void)
{
	int32_t remainder;
	int32_t j;
	int32_t now;
	int32_t i;
	uint32_t frame;
	int32_t steps;
	Animation* anim;
	int32_t elapsed;

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

int32_t AddAnimFrame(int32_t p_resourceId, int32_t p_set)
{
	int32_t found;
	int32_t slot;

	slot = 0;
	found = false;
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
				found = true;
			}
		}
	}
	else {
		g_animSetIsSequence = 0;
	}

	g_animFrames[p_set][slot].m_resourceId = p_resourceId;
	return 0;
}

void PreloadAnimCels(void)
{
	int32_t i;

	for (i = 0; g_preloadCels[i] != -1; i++) {
		PreloadResource(g_preloadCels[i], g_resourceTypeTags[c_resTagCel]);
	}
}

int32_t StartAnimation(int32_t p_index, int32_t p_set)
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

void InitAnimations(void)
{
	int32_t j;
	int32_t i;
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

void SetAnimMode(int16_t p_index, int16_t p_mode)
{
	if (g_animations[p_index].m_flags != -2) {
		g_animations[p_index].m_mode = p_mode;
	}

	if (g_animations[p_index].m_delay == 0) {
		g_animations[p_index].m_delay = 0x2d;
	}
}

void SetAnimFrame(int16_t p_index, uint16_t p_frame)
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

void SetAnimDelay(int16_t p_index, uint16_t p_delay)
{
	g_animations[p_index].m_delay = p_delay;
}

void FreeAnimations(void)
{
	int32_t j;
	int32_t i;
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
