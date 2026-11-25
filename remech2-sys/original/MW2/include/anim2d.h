#ifndef ANIM2D_H
#define ANIM2D_H

#include "decomp.h"
#include "targeting.h"
#include "types.h"

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

typedef struct Anim2d {
	int32_t m_state;      // 0x00
	int32_t m_frameCount; // 0x04
	int32_t m_frameTime;  // 0x08 — clock ticks per frame
	int32_t m_startTime;  // 0x0c — 0: not started
	uint32_t m_flags;     // 0x10
	int32_t m_resourceId; // 0x14 — a SHP resource, relative to g_artResolution
	void* m_shape;        // 0x18 — loaded on first draw
} Anim2d;

// The functions and globals of anim2d.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_anim2dCount;
	extern Anim2d* g_anim2ds[7];

	int16_t LoadAnim2d(uint32_t p_flags, int32_t p_frameTime, int32_t p_type, int16_t* p_resourceId);
	void FreeAnim2ds(int32_t p_index);
	void FreeAnim2d(int32_t p_index);
	void DrawAnim2d(PANE* p_target, int32_t p_index, int32_t p_x, int32_t p_y);
	void RestartAnim2d(int32_t p_index);

#ifdef __cplusplus
}
#endif

#endif // ANIM2D_H
