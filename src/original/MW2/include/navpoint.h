#ifndef NAVPOINT_H
#define NAVPOINT_H

#include "decomp.h"
#include "types.h"

struct SceneObject;

// A nav point (AI target type 0x100): mission navs, and the ones AI players place for
// themselves.
// SIZE 0x54
typedef struct NavPoint {
	undefined4 m_used;                 // 0x00 — nonzero when in use
	struct SceneObject* m_obj;         // 0x04 — the object it follows, or NULL
	MechU32 m_owner;                   // 0x08 — the AI target id (player | 0x200) that placed it
	MechS32 m_team;                    // 0x0c
	MechS32 m_radius;                  // 0x10
	MechS32 m_heading;                 // 0x14 — the heading of a team placed at it (DoFirstObjtv)
	MechS32 m_position[3];             // 0x18
	MechS16 m_flags;                   // 0x24
	MechS16 m_teamsReached;            // 0x26 — a bit per team
	MechChar m_name[0x3e - 0x28];      // 0x28
	MechChar m_shortName[0x54 - 0x3e]; // 0x3e — a short name, for the target panel
} NavPoint;

#endif // NAVPOINT_H
