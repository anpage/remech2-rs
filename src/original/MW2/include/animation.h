#ifndef ANIMATION_H
#define ANIMATION_H

#include "animframe.h"
#include "types.h"

#pragma pack(push, 1)

// An animated texture: which frame set it plays, where it is, and how fast.
// SIZE 0x0e
typedef struct Animation {
	MechS16 m_set;      // 0x00 — in g_animFrames
	MechS16 m_frame;    // 0x02
	MechU16 m_delay;    // 0x04 — ticks per frame; 0 stops it
	MechU16 m_mode;     // 0x06 — 0 stopped, 1 looping, 2 playing once
	MechS16 m_flags;    // 0x08 — -2 for a free slot
	MechS32 m_lastTime; // 0x0a
} Animation;

#pragma pack(pop)

// The functions and globals of animation.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern AnimFrame g_animFrames[0x200][0x20];
	extern MechS32 g_lumaResourceId;
	extern Animation g_animations[0x200];

	MechS32 DrawAnimatedPolygon(
		MechS32 p_index,
		MechS32 p_count,
		MechU32* p_points,
		MechS32 p_luma,
		MechS32 p_scale,
		MechS32 p_direct
	);
	void AdvanceAnimations(void);
	MechS32 AddAnimFrame(MechS32 p_resourceId, MechS32 p_set);
	void PreloadAnimCels(void);
	MechS32 StartAnimation(MechS32 p_index, MechS32 p_set);
	void InitAnimations(void);
	void SetAnimMode(MechS16 p_index, MechS16 p_mode);
	void SetAnimFrame(MechS16 p_index, MechU16 p_frame);
	void SetAnimDelay(MechS16 p_index, MechU16 p_delay);
	void FUN_10069586(void);
	void FUN_10069591(void);
	void FreeAnimations(void);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_H
