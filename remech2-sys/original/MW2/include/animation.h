#ifndef ANIMATION_H
#define ANIMATION_H

#include "animframe.h"
#include "pane.h"
#include "types.h"

#pragma pack(push, 1)

// An animated texture: which frame set it plays, where it is, and how fast.
typedef struct Animation {
	int16_t m_set;      // 0x00 — in g_animFrames
	int16_t m_frame;    // 0x02
	uint16_t m_delay;   // 0x04 — ticks per frame; 0 stops it
	uint16_t m_mode;    // 0x06 — 0 stopped, 1 looping, 2 playing once
	int16_t m_flags;    // 0x08 — -2 for a free slot
	int32_t m_lastTime; // 0x0a
} Animation;

#pragma pack(pop)

// The functions and globals of animation.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern AnimFrame g_animFrames[0x200][0x20];
	extern int32_t g_lumaResourceId;
	extern Animation g_animations[0x200];
	extern int32_t g_animInitialized;
	extern int32_t g_currentAnimSet;
	extern int32_t g_animSetUsed;
	extern int32_t g_animSetIsSequence;
	extern uint16_t* g_lumaTables;
	extern int32_t g_preloadCels[103];
	extern WINDOW g_animFrameBuffer;

	int32_t DrawAnimatedPolygon(
		int32_t p_index,
		int32_t p_count,
		uint32_t* p_points,
		int32_t p_luma,
		int32_t p_scale,
		int32_t p_direct
	);
	void AdvanceAnimations(void);
	int32_t AddAnimFrame(int32_t p_resourceId, int32_t p_set);
	void PreloadAnimCels(void);
	int32_t StartAnimation(int32_t p_index, int32_t p_set);
	void InitAnimations(void);
	void SetAnimMode(int16_t p_index, int16_t p_mode);
	void SetAnimFrame(int16_t p_index, uint16_t p_frame);
	void SetAnimDelay(int16_t p_index, uint16_t p_delay);
	void FreeAnimations(void);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_H
