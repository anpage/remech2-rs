#include "palfade.h"

#include "decomp.h"
#include "palcycle.h"
#include "refreshmode.h"
#include "simmain.h"
#include "types.h"

int32_t InitPaletteCycle(PaletteCycle* p_cycle, uint8_t* p_palette, uint8_t p_first, int32_t p_count)
{
	uint8_t* working;
	int32_t i;
	uint8_t* dst;

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

void RotatePaletteCycle(PaletteCycle* p_cycle)
{
	uint8_t* p;
	uint8_t first[3];
	int32_t i;
	uint8_t* start;

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

void FreePaletteCycle(PaletteCycle* p_cycle)
{
	if (p_cycle->m_working) {
		MechHeapFree(g_primaryHeap, p_cycle->m_working);
		p_cycle->m_working = NULL;
	}

	g_currentDisplayBackend->m_setPaletteWithBrightness((PaletteColor*) p_cycle->m_original);
}

int32_t InitPaletteFade(
	PaletteFade* p_fade,
	uint8_t* p_from,
	uint8_t* p_to,
	int32_t p_first,
	int32_t p_count,
	int32_t p_steps
)
{
	uint8_t* palette;
	int16_t* accum;
	int32_t i;
	int8_t* delta;
	uint8_t* block;

	block = MechHeapAlloc(g_primaryHeap, p_count * 12);
	if (block == NULL) {
		return -1;
	}

	p_fade->m_palette = block;
	p_fade->m_delta = (int8_t*) (block + 0x300);
	p_fade->m_accum = (int16_t*) (block + 0x600);

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

void StepPaletteFade(PaletteFade* p_fade, int32_t p_count)
{
	uint8_t* palette;
	int16_t* accum;
	int32_t i;
	int8_t* delta;

	if (p_fade->m_step >= p_fade->m_steps) {
		return;
	}

	while (p_count-- > 0 && p_fade->m_step < p_fade->m_steps) {
		for (i = 0, accum = p_fade->m_accum, delta = p_fade->m_delta, palette = p_fade->m_palette + p_fade->m_first;
			 i < p_fade->m_count * 3;
			 i++, accum++, delta++, palette++) {
			*accum += *delta;
			*palette += *accum / p_fade->m_steps;
			*accum %= p_fade->m_steps;
		}

		p_fade->m_step++;
	}

	g_currentDisplayBackend->m_setPaletteWithBrightness((PaletteColor*) p_fade->m_palette);

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
