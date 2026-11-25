#include "camerashake.h"

#include "clock.h"
#include "decomp.h"
#include "eyepoint.h"
#include "polydraw.h"
#include "ramp.h"
#include "types.h"

#include <string.h>

// The camera shake: a list of up to ten keys, each an offset of the eyepoint's position and
// orientation reached over a duration. The keys play in order, ramping from one offset to the
// next, on top of the eyepoint's base values.

MechS32 g_cameraShakeKeyCount = 10;

MechS32 g_cameraShakeActive = 0;

Ramp g_cameraShakeHeading;

MechS32 g_cameraShakeKey;

Ramp g_cameraShakeZ;

MechS32 g_cameraShakeKeyTime;

Ramp g_cameraShakeRoll;

Ramp g_cameraShakeX;

Ramp g_cameraShakePitch;

CameraShakeKey g_cameraShakeKeys[10];

Ramp g_cameraShakeY;

void ResetCameraShake(void)
{
	ClearCameraShakeKeys();
}

// Moves the eyepoint by the shake's current offset. Returns FALSE when no shake is playing.
MechS32 UpdateCameraShake(void)
{
	MechS32 base10;
	MechS32 base0c;
	MechS32 base14;
	MechS32 x;
	MechS32 y;
	MechS32 z;

	if (g_cameraShakeKey < 0 || g_cameraShakeKey >= g_cameraShakeKeyCount) {
		g_cameraShakeActive = FALSE;
	}

	if (!g_cameraShakeActive) {
		return FALSE;
	}

	g_cameraShakeKeyTime += g_deltaTime;
	if (g_cameraShakeKeyTime >= g_cameraShakeKeys[g_cameraShakeKey].m_duration) {
		g_cameraShakeKeyTime = 0;
		g_cameraShakeKey++;
		if (g_cameraShakeKey < g_cameraShakeKeyCount) {
			StartCameraShakeKey(g_cameraShakeKey);
		}
	}

	GetCockpitEyeView(&base10, &base0c, &base14, &x, &y, &z);
	g_eyepoint->m_x = x + UpdateRamp(&g_cameraShakeX);
	g_eyepoint->m_y = y + UpdateRamp(&g_cameraShakeY);
	g_eyepoint->m_z = z + UpdateRamp(&g_cameraShakeZ);
	g_eyepoint->m_pitch = base10 + UpdateRamp(&g_cameraShakePitch);
	g_eyepoint->m_heading = base0c + UpdateRamp(&g_cameraShakeHeading);
	g_eyepoint->m_roll = base14 + UpdateRamp(&g_cameraShakeRoll);
	return TRUE;
}

void StartCameraShake(void)
{
	g_cameraShakeKey = 0;
	g_cameraShakeKeyTime = 0;
	if (g_cameraShakeKeyCount > 0) {
		g_cameraShakeActive = TRUE;
		StartCameraShakeKey(0);
	}
}

// Starts the ramps toward a key, from the eyepoint's current offset (the first key) or from the
// previous key.
void StartCameraShakeKey(MechS32 p_key)
{
	MechS32 x;
	MechDouble seconds;
	MechS32 off14;
	MechS32 off0c;
	MechS32 off10;
	MechS32 z;
	CameraShakeKey* key;
	MechS32 y;

	if (p_key >= g_cameraShakeKeyCount || p_key >= 10 || p_key < 0) {
		return;
	}

	if (p_key == 0) {
		GetCockpitEyeView(&off10, &off0c, &off14, &x, &y, &z);
		x = g_eyepoint->m_x - x;
		y = g_eyepoint->m_y - y;
		z = g_eyepoint->m_z - z;
		off10 = g_eyepoint->m_pitch - off10;
		off0c = g_eyepoint->m_heading - off0c;
		off14 = g_eyepoint->m_roll - off14;
	}
	else {
		x = g_cameraShakeX.m_value;
		y = g_cameraShakeY.m_value;
		z = g_cameraShakeZ.m_value;
		off10 = g_cameraShakePitch.m_value;
		off0c = g_cameraShakeHeading.m_value;
		off14 = g_cameraShakeRoll.m_value;
	}

	key = &g_cameraShakeKeys[p_key];
	seconds = key->m_duration / 181.0;
	StartRamp(&g_cameraShakeX, key->m_x, x, seconds);
	StartRamp(&g_cameraShakeY, key->m_y, y, seconds);
	StartRamp(&g_cameraShakeZ, key->m_z, z, seconds);
	StartRamp(&g_cameraShakePitch, key->m_pitch, off10, seconds);
	StartRamp(&g_cameraShakeHeading, key->m_heading, off0c, seconds);
	StartRamp(&g_cameraShakeRoll, key->m_roll, off14, seconds);
}

void ClearCameraShakeKeys(void)
{
	memset(g_cameraShakeKeys, 0, sizeof(g_cameraShakeKeys));
	g_cameraShakeKeyCount = 0;
	g_cameraShakeKey = 0;
	g_cameraShakeKeyTime = 0;
	g_cameraShakeActive = FALSE;
}

void AddCameraShakeKey(
	MechS32 p_x,
	MechS32 p_y,
	MechS32 p_z,
	MechS32 p_pitch,
	MechS32 p_heading,
	MechS32 p_roll,
	MechDouble p_seconds
)
{
	CameraShakeKey* key;

	if (g_cameraShakeKeyCount >= 10) {
		return;
	}

	key = &g_cameraShakeKeys[g_cameraShakeKeyCount];
	key->m_x = p_x;
	key->m_y = p_y;
	key->m_z = p_z;
	key->m_pitch = p_pitch;
	key->m_heading = p_heading;
	key->m_roll = p_roll;
	key->m_duration = p_seconds * 181.0;
	g_cameraShakeKeyCount++;
}

MechS32 IsCameraShaking(void)
{
	return g_cameraShakeActive;
}
