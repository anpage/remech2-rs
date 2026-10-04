#ifndef GAUGEQUADRANT_H
#define GAUGEQUADRANT_H

#include "types.h"

// The direction of a gauge needle from the center of its rectangle, as GetRectEdgeAtSlope and
// GetEllipseEdgeAtAngle switch on it: the needle points left (x at or left of the center), up, and
// too steeply for a 16.16 slope.
// SIZE 0x4
typedef union GaugeQuadrant {
	struct {
		MechU32 m_left : 1;  // 0x00 — bit 0
		MechU32 m_up : 1;    // 0x00 — bit 1
		MechU32 m_steep : 1; // 0x00 — bit 2
	} m_bits;
	MechS32 m_value; // 0x00
} GaugeQuadrant;

#endif // GAUGEQUADRANT_H
