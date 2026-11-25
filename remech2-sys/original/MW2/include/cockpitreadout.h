#ifndef COCKPITREADOUT_H
#define COCKPITREADOUT_H

#include "point.h"
#include "types.h"

// A text readout of the cockpit (g_cockpitReadout): a font, a label and the text formatted after
// it, at a place in 16.16 fractions of the screen.
typedef struct CockpitReadout {
	int32_t m_font;           // its font, from g_artResolution
	int32_t m_formattedValue; // the value it last formatted
	char* m_label;
	char* m_text;
	Point m_position;
} CockpitReadout;

#endif // COCKPITREADOUT_H
