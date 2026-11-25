#ifndef PLAYERTARGETINFO_H
#define PLAYERTARGETINFO_H

#include "decomp.h"
#include "types.h"
#include "vector3.h"

#pragma pack(push, 1)

// What a player is aiming at: the target and the distance, bearing and pitch to it
// (GetBearingAndRange).
typedef struct PlayerTargetInfo {
	MechS32 m_distance; // 0x00 — to the target along the ground
	MechS32 m_range;    // 0x04 — to the target in a straight line (the target panel's)
	Vector3 m_position; // 0x08 — the target's
	MechS32 m_heading;  // 0x14 — 16.16 degrees, towards the target
	MechS32 m_pitch;    // 0x18 — 16.16 degrees, towards the target
	MechS32 m_target;   // 0x1c — an AI target id (see ai.h)
} PlayerTargetInfo;

#pragma pack(pop)

#endif // PLAYERTARGETINFO_H
