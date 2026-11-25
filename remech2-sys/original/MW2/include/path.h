#ifndef PATH_H
#define PATH_H

#include "types.h"

// A path's waypoint: a position and three 16.16 values the path record gives as integers.
typedef struct PathPoint {
	int32_t m_x;        // 0x00
	int32_t m_y;        // 0x04
	int32_t m_z;        // 0x08
	int32_t m_pitch;    // 0x0c
	int32_t m_heading;  // 0x10
	int32_t m_roll;     // 0x14
	int32_t m_duration; // 0x18
} PathPoint;

// A named path of up to 64 waypoints, loaded from a mission's path records.
typedef struct Path {
	char m_name[0x40];        // 0x00
	int32_t m_count;          // 0x40
	PathPoint m_points[0x40]; // 0x44
} Path;

#endif // PATH_H
