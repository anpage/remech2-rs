#ifndef MAPPOINT_H
#define MAPPOINT_H

#include "point.h"
#include "types.h"

// A point of the map view: a world position until ProjectMapPoint projects it in place, then its
// position in the view and its depth.
typedef struct MapPoint {
	Point m_xy;  // 0x00
	int32_t m_z; // 0x08
} MapPoint;

#endif // MAPPOINT_H
