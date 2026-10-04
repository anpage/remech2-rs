#include "palfade.h"

#include "decomp.h"
#include "palcycle.h"
#include "refreshmode.h"
#include "simmain.h"
#include "types.h"

DECOMP_SIZE_ASSERT(PaletteCycle, 0x0d)
DECOMP_SIZE_ASSERT(PaletteFade, 0x19)

// Matches except for the stack slots of i and dst (a consistent permutation).
// FUNCTION: MW2 0x10010ad0
MechS32 InitPaletteCycle(PaletteCycle* p_cycle, MechU8* p_palette, MechU8 p_first, MechS32 p_count)
{
	MechU8* working;
	MechS32 i;
	MechU8* dst;

	working = MechHeapAlloc(g_primaryHeap, p_count * 3);
	if (working == NULL) {
		return 0;
	}

	for (i = 0, dst = working; i < p_count * 3; i++) {
		*dst++ = p_palette[i];
	}

	p_cycle->m_original = p_palette;
	p_cycle->m_working = working;
	p_cycle->m_first = p_first;
	p_cycle->m_count = p_count;
	return 1;
}

// FUNCTION: MW2 0x10010b6e
void RotatePaletteCycle(PaletteCycle* p_cycle)
{
	MechU8* p;
	MechU8 first[3];
	MechS32 i;
	MechU8* start;

	p = p_cycle->m_working + p_cycle->m_first * 3;
	start = p;
	for (i = 0; i < 3; i++) {
		first[i] = *p++;
	}

	p = start;
	for (i = 0; i < (p_cycle->m_count - 1) * 3; i++) {
		*p = p[3];
		p++;
	}

	for (i = 0; i < 3; i++) {
		*p++ = first[i];
	}

	g_currentDisplayBackend->m_setPaletteWithBrightness((PaletteColor*) p_cycle->m_working);
}

// FUNCTION: MW2 0x10010c3d
void FreePaletteCycle(PaletteCycle* p_cycle)
{
	if (p_cycle->m_working) {
		MechHeapFree(g_primaryHeap, p_cycle->m_working);
		p_cycle->m_working = NULL;
	}

	g_currentDisplayBackend->m_setPaletteWithBrightness((PaletteColor*) p_cycle->m_original);
}

// Matches except for the stack slots of i and accum (a consistent permutation).
// FUNCTION: MW2 0x10010c85
MechS32 InitPaletteFade(
	PaletteFade* p_fade,
	MechU8* p_from,
	MechU8* p_to,
	MechS32 p_first,
	MechS32 p_count,
	MechS32 p_steps
)
{
	MechU8* palette;
	MechS16* accum;
	MechS32 i;
	MechS8* delta;
	MechU8* block;

	block = MechHeapAlloc(g_primaryHeap, p_count * 12);
	if (block == NULL) {
		return -1;
	}

	p_fade->m_palette = block;
	p_fade->m_delta = (MechS8*) (block + 0x300);
	p_fade->m_accum = (MechS16*) (block + 0x600);

	for (i = p_first * 3, delta = p_fade->m_delta, palette = p_fade->m_palette, accum = p_fade->m_accum;
		 i < p_count * 3 + p_first * 3;
		 i++) {
		*palette++ = p_from[i];
		*delta++ = p_to[i] - p_from[i];
		*accum++ = 0;
	}

	p_fade->m_steps = p_steps;
	p_fade->m_step = 0;
	p_fade->m_first = p_first;
	p_fade->m_count = p_count;
	return 1;
}

// Matches except for the stack slots of i and accum (a consistent permutation).
// FUNCTION: MW2 0x10010d99
void StepPaletteFade(PaletteFade* p_fade)
{
	MechU8* palette;
	MechS16* accum;
	MechS32 i;
	MechS8* delta;

	if (p_fade->m_step >= p_fade->m_steps) {
		return;
	}

	for (i = 0, accum = p_fade->m_accum, delta = p_fade->m_delta, palette = p_fade->m_palette + p_fade->m_first;
		 i < p_fade->m_count * 3;
		 i++, accum++, delta++, palette++) {
		*accum += *delta;
		*palette += *accum / p_fade->m_steps;
		*accum %= p_fade->m_steps;
	}

	g_currentDisplayBackend->m_setPaletteWithBrightness((PaletteColor*) p_fade->m_palette);
	p_fade->m_step++;
	if (p_fade->m_step >= p_fade->m_steps && p_fade->m_palette) {
		if (p_fade->m_palette) {
			MechHeapFree(g_primaryHeap, p_fade->m_palette);
			p_fade->m_palette = NULL;
		}

		if (p_fade->m_delta) {
			p_fade->m_delta = NULL;
		}

		if (p_fade->m_accum) {
			p_fade->m_accum = NULL;
		}
	}
}
