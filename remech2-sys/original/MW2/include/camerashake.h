#ifndef CAMERASHAKE_H
#define CAMERASHAKE_H

#include "ramp.h"
#include "types.h"

// An offset of the eyepoint's first six fields (its position and orientation).
typedef struct CameraShakeKey {
	int32_t m_x;        // 0x00
	int32_t m_y;        // 0x04
	int32_t m_z;        // 0x08
	int32_t m_heading;  // 0x0c
	int32_t m_pitch;    // 0x10
	int32_t m_roll;     // 0x14
	int32_t m_duration; // 0x18 — in clock ticks
} CameraShakeKey;

// The functions and globals of camerashake.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_cameraShakeKeyCount;
	extern int32_t g_cameraShakeActive;
	extern CameraShakeKey g_cameraShakeKeys[10];
	extern Ramp g_cameraShakeHeading;
	extern int32_t g_cameraShakeKey;
	extern Ramp g_cameraShakeZ;
	extern int32_t g_cameraShakeKeyTime;
	extern Ramp g_cameraShakeRoll;
	extern Ramp g_cameraShakeX;
	extern Ramp g_cameraShakePitch;
	extern Ramp g_cameraShakeY;

	void ResetCameraShake(void);
	int32_t UpdateCameraShake(void);
	void StartCameraShake(void);
	void StartCameraShakeKey(int32_t p_key);
	void ClearCameraShakeKeys(void);
	void AddCameraShakeKey(
		int32_t p_x,
		int32_t p_y,
		int32_t p_z,
		int32_t p_pitch,
		int32_t p_heading,
		int32_t p_roll,
		double p_seconds
	);
	int32_t IsCameraShaking(void);

#ifdef __cplusplus
}
#endif

#endif // CAMERASHAKE_H
