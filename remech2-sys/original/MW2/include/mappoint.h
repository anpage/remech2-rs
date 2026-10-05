#ifndef MAPPOINT_H
#define MAPPOINT_H

#include "point.h"
#include "types.h"

// A point of the map view: a world position until ProjectMapPoint projects it in place, then its
// position in the view and its depth.
// SIZE 0xc
typedef struct MapPoint {
	Point m_xy;  // 0x00
	MechS32 m_z; // 0x08
} MapPoint;

#endif // MAPPOINT_H
