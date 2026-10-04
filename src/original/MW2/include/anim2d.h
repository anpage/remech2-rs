#ifndef ANIM2D_H
#define ANIM2D_H

#include "decomp.h"
#include "targeting.h"
#include "types.h"

// SIZE 0x08
typedef struct FramePrj {
	MechS32 m_id;      // 0x00 — -1: free
	MechS16 m_unk0x04; // 0x04
	MechS16 m_unk0x06; // 0x06
} FramePrj;

// Anim2d::m_state.
enum {
	c_anim2dNew = 0,
	c_anim2dDone = 1,
	c_anim2dPlaying = 2,
	c_anim2dHeld = 3
};

// Anim2d::m_flags.
enum {
	c_anim2dOnce = 0x1,
	c_anim2dHoldLast = 0x2,
	c_anim2dFreeWhenDone = 0x8
};

// SIZE 0x1c
typedef struct Anim2d {
	MechS32 m_state;      // 0x00
	MechS32 m_frameCount; // 0x04
	MechS32 m_frameTime;  // 0x08 — clock ticks per frame
	MechS32 m_startTime;  // 0x0c — 0: not started
	MechU32 m_flags;      // 0x10
	MechS32 m_resourceId; // 0x14 — a SHP resource, relative to g_artResolution
	void* m_shape;        // 0x18 — loaded on first draw
} Anim2d;

// The functions and globals of anim2d.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_framePrjCount;
	extern MechS32 g_anim2dCount;
	extern Anim2d* g_anim2ds[7];
	extern FramePrj g_framePrjs[0x60];

	MechS32 LoadFramePrj(MechU16 p_id, MechS16 p_unk0x04, MechS16 p_unk0x06);
	void ResetFramePrjs(void);
	void ClearFramePrj(FramePrj* p_prj);
	MechS16 LoadAnim2d(MechU32 p_flags, MechS32 p_frameTime, MechS32 p_type, MechS16* p_resourceId);
	void FreeAnim2ds(MechS32 p_index);
	void FreeAnim2d(MechS32 p_index);
	void DrawAnim2d(PANE* p_target, MechS32 p_index, MechS32 p_x, MechS32 p_y);
	void RestartAnim2d(MechS32 p_index);

#ifdef __cplusplus
}
#endif

#endif // ANIM2D_H
