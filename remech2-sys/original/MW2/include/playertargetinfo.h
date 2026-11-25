#ifndef PLAYERTARGETINFO_H
#define PLAYERTARGETINFO_H

#include "types.h"
#include "vector3.h"

#pragma pack(push, 1)

// What a player is aiming at: the target and the distance, bearing and pitch to it
// (GetBearingAndRange).
typedef struct PlayerTargetInfo {
	int32_t m_distance; // to the target along the ground
	int32_t m_range;    // to the target in a straight line (the target panel's)
	Vector3 m_position; // the target's
	int32_t m_heading;  // 16.16 degrees, towards the target
	int32_t m_pitch;    // 16.16 degrees, towards the target
	int32_t m_target;   // an AI target id (see ai.h)
} PlayerTargetInfo;

#pragma pack(pop)

#endif // PLAYERTARGETINFO_H
