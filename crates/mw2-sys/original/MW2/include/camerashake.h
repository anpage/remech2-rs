#ifndef CAMERASHAKE_H
#define CAMERASHAKE_H

#include "ramp.h"
#include "types.h"

// An offset of the eyepoint's first six fields (its position and orientation).
// SIZE 0x1c
typedef struct CameraShakeKey {
	MechS32 m_x;        // 0x00
	MechS32 m_y;        // 0x04
	MechS32 m_z;        // 0x08
	MechS32 m_heading;  // 0x0c
	MechS32 m_pitch;    // 0x10
	MechS32 m_roll;     // 0x14
	MechS32 m_duration; // 0x18 — in clock ticks
} CameraShakeKey;

// The functions and globals of camerashake.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_cameraShakeKeyCount;
	extern MechS32 g_cameraShakeActive;
	extern CameraShakeKey g_cameraShakeKeys[10];
	extern Ramp g_cameraShakeHeading;
	extern MechS32 g_cameraShakeKey;
	extern Ramp g_cameraShakeZ;
	extern MechS32 g_cameraShakeKeyTime;
	extern Ramp g_cameraShakeRoll;
	extern Ramp g_cameraShakeX;
	extern Ramp g_cameraShakePitch;
	extern Ramp g_cameraShakeY;

	void ResetCameraShake(void);
	MechS32 UpdateCameraShake(void);
	void StartCameraShake(void);
	void StartCameraShakeKey(MechS32 p_key);
	void ClearCameraShakeKeys(void);
	void AddCameraShakeKey(
		MechS32 p_x,
		MechS32 p_y,
		MechS32 p_z,
		MechS32 p_pitch,
		MechS32 p_heading,
		MechS32 p_roll,
		MechDouble p_seconds
	);
	MechS32 IsCameraShaking(void);

#ifdef __cplusplus
}
#endif

#endif // CAMERASHAKE_H
