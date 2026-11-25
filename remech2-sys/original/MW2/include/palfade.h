#ifndef PALFADE_H
#define PALFADE_H

#include "palcycle.h"
#include "types.h"

// A frame-driven fade between two palettes (palette.c's g_paletteFade). InitPaletteFade
// allocates the three buffers as one block; StepPaletteFade adds m_delta / m_steps to each
// channel, carrying the remainders in m_accum, and frees the block after the last step.
#pragma pack(push, 1)
typedef struct PaletteFade {
	uint8_t* m_palette; // 0x00
	int8_t* m_delta;    // 0x04
	int16_t* m_accum;   // 0x08
	int32_t m_steps;    // 0x0c
	int32_t m_step;     // 0x10
	uint8_t m_first;    // 0x14
	int32_t m_count;    // 0x15
} PaletteFade;
#pragma pack(pop)

// The functions and globals of palfade.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t InitPaletteCycle(PaletteCycle* p_cycle, uint8_t* p_palette, uint8_t p_first, int32_t p_count);
	void RotatePaletteCycle(PaletteCycle* p_cycle);
	void FreePaletteCycle(PaletteCycle* p_cycle);
	int32_t InitPaletteFade(
		PaletteFade* p_fade,
		uint8_t* p_from,
		uint8_t* p_to,
		int32_t p_first,
		int32_t p_count,
		int32_t p_steps
	);
	void StepPaletteFade(PaletteFade* p_fade, int32_t p_count);

#ifdef __cplusplus
}
#endif

#endif // PALFADE_H
