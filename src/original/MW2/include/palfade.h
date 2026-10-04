#ifndef PALFADE_H
#define PALFADE_H

#include "decomp.h"
#include "palcycle.h"
#include "types.h"

// A frame-driven fade between two palettes (palette.c's g_paletteFade). InitPaletteFade
// allocates the three buffers as one block; StepPaletteFade adds m_delta / m_steps to each
// channel, carrying the remainders in m_accum, and frees the block after the last step.
#pragma pack(push, 1)
// SIZE 0x19
typedef struct PaletteFade {
	MechU8* m_palette; // 0x00
	MechS8* m_delta;   // 0x04
	MechS16* m_accum;  // 0x08
	MechS32 m_steps;   // 0x0c
	MechS32 m_step;    // 0x10
	MechU8 m_first;    // 0x14
	MechS32 m_count;   // 0x15
} PaletteFade;
#pragma pack(pop)

// The functions and globals of palfade.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 InitPaletteCycle(PaletteCycle* p_cycle, MechU8* p_palette, MechU8 p_first, MechS32 p_count);
	void RotatePaletteCycle(PaletteCycle* p_cycle);
	void FreePaletteCycle(PaletteCycle* p_cycle);
	MechS32 InitPaletteFade(
		PaletteFade* p_fade,
		MechU8* p_from,
		MechU8* p_to,
		MechS32 p_first,
		MechS32 p_count,
		MechS32 p_steps
	);
	void StepPaletteFade(PaletteFade* p_fade);

#ifdef __cplusplus
}
#endif

#endif // PALFADE_H
