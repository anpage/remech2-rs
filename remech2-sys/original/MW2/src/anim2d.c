#include "anim2d.h"

#include "clock.h"
#include "decomp.h"
#include "loadres.h"
#include "mw2prj.h"
#include "setres.h"
#include "simmain.h"
#include "targeting.h"
#include "types.h"

#include <string.h>

// 2D animations: up to seven SHP resources whose frames play on the clock, once or looping,
// drawn into a pane.

int32_t g_anim2dCount = 0;

Anim2d* g_anim2ds[7];

int16_t LoadAnim2d(uint32_t p_flags, int32_t p_frameTime, int32_t p_type, int16_t* p_resourceId)
{
	Anim2d* anim;

	if (g_anim2dCount >= 7) {
		return -1;
	}

	if (p_type != 1) {
		return -1;
	}

	if (*p_resourceId < 1) {
		return -1;
	}

	anim = MechHeapAlloc(g_primaryHeap, sizeof(Anim2d));
	if (!anim) {
		return -1;
	}

	memset(anim, 0, sizeof(Anim2d));
	g_anim2ds[g_anim2dCount] = anim;
	anim->m_state = c_anim2dNew;
	anim->m_startTime = 0;
	anim->m_flags = p_flags;
	anim->m_frameTime = p_frameTime;
	anim->m_resourceId = *p_resourceId;
	return g_anim2dCount++;
}

// Frees one animation, or all of them for a negative index.
void FreeAnim2ds(int32_t p_index)
{
	if (p_index >= 0) {
		FreeAnim2d(p_index);
	}
	else {
		for (p_index = 0; p_index < 7; p_index++) {
			FreeAnim2d(p_index);
		}
	}
}

void FreeAnim2d(int32_t p_index)
{
	if (g_anim2ds[p_index]) {
		if (g_anim2ds[p_index]->m_shape) {
			UnlockCachedResource(g_anim2ds[p_index]->m_resourceId + g_artResolution, g_resourceTypeTags[c_resTagShp]);
		}

		MechHeapFree(g_primaryHeap, g_anim2ds[p_index]);
		g_anim2ds[p_index] = NULL;
	}
}

// Draws the animation's current frame, loading its shape on first use. The index test never
// rejects anything (the original tests p_index < 0 && p_index >= 7).
void DrawAnim2d(PANE* p_target, int32_t p_index, int32_t p_x, int32_t p_y)
{
	int32_t frame;
	Anim2d* anim;
	uint16_t shown;

	if (p_index < 0 && p_index >= 7) {
		return;
	}

	anim = g_anim2ds[p_index];
	if (!anim) {
		return;
	}

	if (anim->m_state == c_anim2dDone) {
		return;
	}

	if (!anim->m_shape) {
		anim->m_shape = LoadCachedResource(
			g_mw2PrjHandle,
			anim->m_resourceId + g_artResolution,
			g_resourceTypeTags[c_resTagShp],
			0
		);
		if (!anim->m_shape) {
			FreeAnim2ds(p_index);
			return;
		}

		anim->m_frameCount = VFX_shape_count(anim->m_shape);
	}

	if (anim->m_state == c_anim2dNew) {
		anim->m_state = c_anim2dPlaying;
	}

	if (anim->m_startTime) {
		frame = (g_currentClock - anim->m_startTime) / anim->m_frameTime;
		if (frame < 0) {
			frame = 0;
		}
	}
	else {
		frame = 0;
		anim->m_startTime = g_currentClock;
	}

	if (anim->m_flags & c_anim2dOnce) {
		if (frame >= anim->m_frameCount) {
			if (anim->m_flags & c_anim2dHoldLast) {
				shown = anim->m_frameCount - 1;
				anim->m_state = c_anim2dHeld;
			}
			else {
				anim->m_state = c_anim2dDone;
			}
		}
		else {
			shown = frame;
		}
	}
	else {
		shown = frame % anim->m_frameCount;
	}

	if (anim->m_state != c_anim2dDone) {
		VFX_shape_draw(p_target, anim->m_shape, shown, p_x, p_y);
	}
	else if (anim->m_flags & c_anim2dFreeWhenDone) {
		FreeAnim2ds(p_index);
	}
}

// Restarts the animation. As in DrawAnim2d, the index test never rejects anything.
void RestartAnim2d(int32_t p_index)
{
	if (p_index < 0 && p_index >= 7) {
		return;
	}

	g_anim2ds[p_index]->m_state = c_anim2dNew;
	g_anim2ds[p_index]->m_startTime = 0;
}
