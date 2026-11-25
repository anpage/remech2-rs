#ifndef COCKPITREADOUT_H
#define COCKPITREADOUT_H

#include "decomp.h"
#include "point.h"
#include "types.h"

// A text readout of the cockpit (g_cockpitReadout): a font, a label and the text formatted after
// it, at a place in 16.16 fractions of the screen.
typedef struct CockpitReadout {
	int32_t m_font;           // 0x00 — its font, from g_artResolution
	int32_t m_formattedValue; // 0x08 — the value it last formatted
	char* m_label;            // 0x0c
	char* m_text;             // 0x10
	Point m_position;         // 0x14
} CockpitReadout;

#endif // COCKPITREADOUT_H
