#ifndef NAVPOINT_H
#define NAVPOINT_H

#include "types.h"

struct SceneObject;

// A nav point (AI target type 0x100): mission navs, and the ones AI players place for
// themselves.
typedef struct NavPoint {
	uint32_t m_used;           // nonzero when in use
	struct SceneObject* m_obj; // the object it follows, or NULL
	uint32_t m_owner;          // the AI target id (player | 0x200) that placed it
	int32_t m_team;
	int32_t m_radius;
	int32_t m_heading; // the heading of a team placed at it (DoFirstObjtv)
	int32_t m_position[3];
	int16_t m_flags;
	int16_t m_teamsReached; // a bit per team
	char m_name[0x3e - 0x28];
	char m_shortName[0x54 - 0x3e]; // a short name, for the target panel
} NavPoint;

#endif // NAVPOINT_H
