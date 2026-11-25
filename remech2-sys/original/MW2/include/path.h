#ifndef PATH_H
#define PATH_H

#include "types.h"

// A path's waypoint: a position and three 16.16 values the path record gives as integers.
typedef struct PathPoint {
	int32_t m_x;
	int32_t m_y;
	int32_t m_z;
	int32_t m_pitch;
	int32_t m_heading;
	int32_t m_roll;
	int32_t m_duration;
} PathPoint;

// A named path of up to 64 waypoints, loaded from a mission's path records.
typedef struct Path {
	char m_name[0x40];
	int32_t m_count;
	PathPoint m_points[0x40];
} Path;

#endif // PATH_H
