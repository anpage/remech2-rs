#ifndef PLAYERTARGETINFO_H
#define PLAYERTARGETINFO_H

#include "types.h"
#include "vector3.h"

#pragma pack(push, 1)

// What a player is aiming at: the target and the distance, bearing and pitch to it
// (GetBearingAndRange).
typedef struct PlayerTargetInfo {
	int32_t m_distance; // 0x00 — to the target along the ground
	int32_t m_range;    // 0x04 — to the target in a straight line (the target panel's)
	Vector3 m_position; // 0x08 — the target's
	int32_t m_heading;  // 0x14 — 16.16 degrees, towards the target
	int32_t m_pitch;    // 0x18 — 16.16 degrees, towards the target
	int32_t m_target;   // 0x1c — an AI target id (see ai.h)
} PlayerTargetInfo;

#pragma pack(pop)

#endif // PLAYERTARGETINFO_H
