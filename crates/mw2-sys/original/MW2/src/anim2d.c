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
#include <windows.h>

// Frame projections (a table of 96) and 2D animations: up to seven SHP resources whose frames
// play on the clock, once or looping, drawn into a pane.

DECOMP_SIZE_ASSERT(FramePrj, 0x08)
DECOMP_SIZE_ASSERT(Anim2d, 0x1c)

// GLOBAL: MW2 0x100a156c
MechS32 g_framePrjCount = -1;

// GLOBAL: MW2 0x100a158c
MechS32 g_anim2dCount = 0;

// GLOBAL: MW2 0x10179b60
Anim2d* g_anim2ds[7];

// GLOBAL: MW2 0x10179b80
FramePrj g_framePrjs[0x60];

// FUNCTION: MW2 0x10007510
MechS32 LoadFramePrj(MechU16 p_id, MechS16 p_unk0x04, MechS16 p_unk0x06)
{
	FramePrj* prj;

	if (g_framePrjCount == -1) {
		ResetFramePrjs();
	}

	if (g_framePrjCount == -1 || g_framePrjCount >= 0x60) {
		return -1;
	}

	prj = &g_framePrjs[g_framePrjCount];
	prj->m_id = p_id;
	prj->m_unk0x04 = p_unk0x04;
	prj->m_unk0x06 = p_unk0x06;
	return g_framePrjCount++;
}

// FUNCTION: MW2 0x1000759c
void ResetFramePrjs(void)
{
	MechS32 i;
	FramePrj* prj;

	for (i = 0; i < 0x60; i++) {
		prj = &g_framePrjs[i];
		ClearFramePrj(prj);
	}

	g_framePrjCount = 0;
}

// FUNCTION: MW2 0x100075eb
void ClearFramePrj(FramePrj* p_prj)
{
	p_prj->m_id = -1;
	p_prj->m_unk0x04 = 0;
	p_prj->m_unk0x06 = 0;
}

// FUNCTION: MW2 0x10007611
MechS16 LoadAnim2d(MechU32 p_flags, MechS32 p_frameTime, MechS32 p_type, MechS16* p_resourceId)
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
// FUNCTION: MW2 0x100076ea
void FreeAnim2ds(MechS32 p_index)
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

// FUNCTION: MW2 0x1000773a
void FreeAnim2d(MechS32 p_index)
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
// Stack-slot permutation: frame and anim.
// FUNCTION: MW2 0x100077b3
void DrawAnim2d(PANE* p_target, MechS32 p_index, MechS32 p_x, MechS32 p_y)
{
	MechS32 frame;
	Anim2d* anim;
	MechU16 shown;

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
// FUNCTION: MW2 0x10007987
void RestartAnim2d(MechS32 p_index)
{
	if (p_index < 0 && p_index >= 7) {
		return;
	}

	g_anim2ds[p_index]->m_state = c_anim2dNew;
	g_anim2ds[p_index]->m_startTime = 0;
}
