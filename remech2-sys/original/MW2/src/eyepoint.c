#include "eyepoint.h"

#include "camerashake.h"
#include "clock.h"
#include "cockpit.h"
#include "collision.h"
#include "decomp.h"
#include "fixedmul.h"
#include "fixedtrig.h"
#include "gridobject.h"
#include "inputmap.h"
#include "integrate.h"
#include "mechclass.h"
#include "muldiv.h"
#include "object.h"
#include "players.h"
#include "polydraw.h"
#include "ramp.h"
#include "render.h"
#include "savedview.h"
#include "shape.h"
#include "shapelists.h"
#include "shots.h"
#include "simmain.h"
#include "soundfx.h"
#include "speech.h"
#include "targeting.h"
#include "transform.h"
#include "types.h"

#include <stdlib.h>

// The track view's distance from the mech; UpdateFreeEyeView also starts the free camera that far
// back from it.
// GLOBAL: MW2 0x100a23ec
MechS32 g_trackDistance = 0;

// The track view's distance limits, height and turn around the mech (UpdateTrackView).

// GLOBAL: MW2 0x100a23f0
MechS32 g_trackMinDistance = 0;

// GLOBAL: MW2 0x100a23f4
MechS32 g_trackMaxDistance = 0;

// GLOBAL: MW2 0x100a23f8
MechS32 g_trackHeight = 0;

// GLOBAL: MW2 0x100a23fc
MechS32 g_trackTurn = 0xb40000;

// GLOBAL: MW2 0x100a2400
MechS32 g_normalFov = 0x10000;

// GLOBAL: MW2 0x100a2404
MechS32 g_zoomFov = 0x10000;

// The view mode the camera was updated in last (c_view...), or -1.
// GLOBAL: MW2 0x100a2408
MechS32 g_viewMode = -1;

// The view mode to return to from the ordinance view.
// GLOBAL: MW2 0x100a240c
MechS32 g_ordinanceReturnMode = -1;

// The view mode FirstEyepoint starts the camera in.
// GLOBAL: MW2 0x100a2410
MechS32 g_initialViewMode = 0;

// The view mode SetViewMode asked for; UpdateEyepoint switches to it.
// GLOBAL: MW2 0x100a2414
MechS32 g_requestedViewMode = 0;

// Set when a mech starts under the autopilot (mechclass.c); nothing reads it.
// GLOBAL: MW2 0x100a2418
MechS32 g_autopilotStart = 1;

// Set while the cockpit view is placed at the eye (GetCockpitEyeView), not shaking: the HUD
// draws the crosshair only then.
// GLOBAL: MW2 0x100a241c
MechS32 g_cockpitEyeSteady = 0;

// Set while the camera is in the cockpit view: the cockpit's own shape (g_cockpitObject) is drawn
// separately, and the 3D sound calls get it.
// GLOBAL: MW2 0x100a2420
undefined4 g_inCockpitView = 0;

// The view mode SetViewMode forces once the local mech is lost (g_localMechLost): the track view.
// GLOBAL: MW2 0x100a2424
MechS32 g_lostViewMode = -1;

// Set while a network game's lost player advances the viewpoint between players; until then the
// track view circles the mech.
// GLOBAL: MW2 0x100a2428
MechS32 g_spectating = 0;

// GLOBAL: MW2 0x100a242c
struct Player* g_localPlayer = NULL;

// The player the camera tracks.
// GLOBAL: MW2 0x100a2430
MechS32 g_trackedPlayer = 0;

// The local mech's cockpit height and torso twist (InitCockpitPanels): the first raises the eye
// (GetPlayerEyeView, the aim ray); nothing reads the second.
// GLOBAL: MW2 0x100a2434
MechS32* g_eyeHeightOffset = NULL;

// GLOBAL: MW2 0x100a2438
MechS32* g_eyeTwist = NULL;

// The drop camera (UpdateDropView): its vertical speed, acceleration and start clock.
// GLOBAL: MW2 0x100a243c
MechS32 g_dropSpeed = 0;

// GLOBAL: MW2 0x100a2440
MechS32 g_dropAcceleration = 0x3ca0;

// GLOBAL: MW2 0x100a2444
MechS32 g_dropStartClock = 0;

// Set while no glance key is held (UpdateCockpitView).
// GLOBAL: MW2 0x100a2448
MechS8 g_glanceReleased = 0;

// The view UpdateOrdinanceView saves when it enters the ordinance view.
// GLOBAL: MW2 0x10176f10
MechS32 g_ordinanceSavedView[7];

// The track view's height limits (UpdateTrackView).
// GLOBAL: MW2 0x10176f2c
MechS32 g_trackMaxHeight;

// The track view's eased offsets from the mech and its eased heading and pitch.
// GLOBAL: MW2 0x10176f30
Ramp g_trackOffsetZ;

// GLOBAL: MW2 0x10176f40
WrappedRamp g_trackPitch;

// GLOBAL: MW2 0x10176f60
WrappedRamp g_trackHeading;

// GLOBAL: MW2 0x10176f74
MechS32 g_trackMinHeight;

// The cockpit view's tilt, eased toward g_sinkPilotTilt.
// GLOBAL: MW2 0x10176f80
Ramp g_pilotTilt;

// GLOBAL: MW2 0x10176f90
Ramp g_trackOffsetY;

// GLOBAL: MW2 0x10176fa0
SavedView g_savedViews[5];

// The cockpit view's pan, eased toward g_sinkPilotPan.
// GLOBAL: MW2 0x10177030
Ramp g_pilotPan;

// The free camera's forward speed.
// GLOBAL: MW2 0x10177040
Ramp g_freeEyeSpeed;

// GLOBAL: MW2 0x10177050
Ramp g_trackOffsetX;

// Resets the camera: its ramps, the saved views and the view scale, follows the local player
// and resets the camera shake and the zoom.
// FUNCTION: MW2 0x10010ee0
void FirstEyepoint(void)
{
	MechS32 i;

	StartRamp(&g_trackOffsetX, 0, 0, 0.5);
	StartRamp(&g_trackOffsetZ, 0, 0, 0.5);
	StartRamp(&g_trackOffsetY, 0, 0, 0.7);
	StartRamp(&g_freeEyeSpeed, 0, 0, 1.0);
	StartRamp(&g_pilotPan, 0, 0, 0.2);
	StartRamp(&g_pilotTilt, 0, 0, 0.2);
	StartWrappedRamp(&g_trackHeading, 0, 0, 0.2, 0x1680000);
	StartWrappedRamp(&g_trackPitch, 0, 0, 0.2, 0x1680000);
	g_sinkZoomFactor = 0x10000;
	for (i = 0; i < 5; i++) {
		g_savedViews[i].m_x = g_savedViews[i].m_y = g_savedViews[i].m_z = 0;
		g_savedViews[i].m_heading = g_savedViews[i].m_pitch = g_savedViews[i].m_roll = 0;
		g_savedViews[i].m_set = 0;
	}

	if (g_players[g_localPlayerId]) {
		g_localPlayer = g_players[g_localPlayerId];
		g_trackedPlayer = g_localPlayerId;
	}

	ResetCameraShake();
	SetViewMode(g_initialViewMode);
}

// Updates the camera for the frame in the view mode GetViewMode picks (the free camera without a
// local player): the cockpit, tracking, external, drop or free camera, driven by the
// eyepoint and track sinks.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x100110f7
void UpdateEyepoint(void)
{
	MechS32 mode;
	MechS32 climb;
	MechS32 strafe;
	MechS32 turn;
	MechS32 speed;
	MechS32 pitch;
	MechS32 pan;

	g_normalFov = g_sinkZoomFactor;
	ApplyCameraFov(0);
	g_inCockpitView = g_cockpitEyeSteady = 0;
	if (!g_playerCount) {
		mode = c_viewFreeEye;
		SetViewMode(c_viewFreeEye);
		g_localPlayer = NULL;
	}
	else {
		mode = GetViewMode();
		if (!mode) {
			g_localPlayer = g_players[g_localPlayerId];
		}

		if (!g_localPlayer) {
			mode = c_viewFreeEye;
		}
	}

	g_renderSettings.m_blankScene = 0;
	switch (mode) {
	case c_viewOrdinance:
		UpdateOrdinanceView();
		break;
	case c_viewTrack:
		speed = MulDiv64(g_sinkTrackDistanceDelta, g_deltaTime, 0xb5) >> 16;
		strafe = MulDiv64(g_sinkTrackHeightDelta, g_deltaTime, 0xb5) >> 16;
		turn = g_sinkEyepointTilt;
		pan = MulDiv64(g_sinkEyepointPanDelta, g_deltaTime, 0xb5);
		if (g_localMechLost && !g_spectating) {
			pan = g_deltaTime << 14;
		}

		UpdateTrackView(speed, strafe, turn, pan);
		break;
	case c_viewCockpit:
	case c_viewSatellite:
		UpdateCockpitView();
		break;
	case c_viewDrop:
		UpdateDropView();
		break;
	default:
		climb = MulDiv64(g_sinkTrackHeightDelta, g_deltaTime, 0xb5) >> 15;
		speed = MulDiv64(g_sinkTrackDistanceDelta, g_deltaTime, 0xb5) >> 15;
		strafe = MulDiv64(g_sinkEyepointSlideDelta, g_deltaTime, 0xb5) >> 16;
		turn = MulDiv64(g_sinkEyepointPanDelta, g_deltaTime, 0xb5);
		if (g_sinkEyepointTilt > 0) {
			pitch = g_deltaTime * 0x2d0000 / 0xb5;
		}
		else if (g_sinkEyepointTilt < 0) {
			pitch = -(g_deltaTime * 0x2d0000) / 0xb5;
		}
		else {
			pitch = 0;
		}

		g_sinkEyepointTiltReset = 1;
		UpdateFreeEyeView(climb, speed, strafe, turn, pitch);
		break;
	}

	g_viewMode = mode;
	UpdateGridObject();
	TurnBillboards();
}

// FUNCTION: MW2 0x100113af
MechS32 GetCameraFloor(Eyepoint* p_eyepoint)
{
	MechS32 height;

	height = GetHighestSurface(p_eyepoint->m_x, p_eyepoint->m_y, p_eyepoint->m_z);
	if (height > 0) {
		height += 500;
	}
	else {
		height += 200;
	}

	return height;
}

// FUNCTION: MW2 0x10011401
void SetViewMode(MechS32 p_zoom)
{
	if (g_localMechLost) {
		if (g_lostViewMode == -1) {
			g_lostViewMode = 1;
		}

		p_zoom = g_lostViewMode;
	}

	g_requestedViewMode = p_zoom;
}

// FUNCTION: MW2 0x10011440
MechS32 GetViewMode(void)
{
	return g_requestedViewMode;
}

// ApplyCameraFov is implemented on the Rust side (src/sim/camera.rs). In widescreen it widens the
// field of view to the frame (Hor+).

// Saves the eyepoint's position and orientation (0x00-0x14) to p_view, marking it (p_view[6])
// as set. Returns 0 without both.
// FUNCTION: MW2 0x100114ea
MechS32 SaveView(Eyepoint* p_eyepoint, MechS32* p_view)
{
	if (p_view == NULL || p_eyepoint == NULL) {
		return 0;
	}

	p_view[0] = p_eyepoint->m_x;
	p_view[1] = p_eyepoint->m_y;
	p_view[2] = p_eyepoint->m_z;
	p_view[3] = p_eyepoint->m_heading;
	p_view[4] = p_eyepoint->m_pitch;
	p_view[5] = p_eyepoint->m_roll;
	p_view[6] = 1;
	return 1;
}

// Restores the eyepoint's position and orientation from p_view, if SaveView set it. Returns
// 0 without both, or when the view is not set.
// FUNCTION: MW2 0x1001156a
MechS32 RestoreView(Eyepoint* p_eyepoint, MechS32* p_view)
{
	if (p_view == NULL || p_eyepoint == NULL) {
		return 0;
	}

	if (!p_view[6]) {
		return 0;
	}

	p_eyepoint->m_x = p_view[0];
	p_eyepoint->m_y = p_view[1];
	p_eyepoint->m_z = p_view[2];
	p_eyepoint->m_heading = p_view[3];
	p_eyepoint->m_pitch = p_view[4];
	p_eyepoint->m_roll = p_view[5];
	return 1;
}

// Moves the camera to the next (p_next) or previous player, or back to the local player
// (p_home), skipping players who left or whose mechs are gone.
// FUNCTION: MW2 0x100115f4
void CycleTrackedPlayer(MechS32 p_next, MechS32 p_home)
{
	MechS32 step;

	step = -1;
	if (!g_playerCount) {
		return;
	}

	if (p_home) {
		g_trackedPlayer = g_localPlayerId;
		step = 0;
	}
	else if (p_next) {
		step = 1;
	}

	g_trackedPlayer += step;
	if (g_trackedPlayer < 0) {
		g_trackedPlayer = g_playerCount - 1;
	}
	else if (g_trackedPlayer >= g_playerCount) {
		g_trackedPlayer = 0;
	}

	g_localPlayer = g_players[g_trackedPlayer];
	if (g_localPlayer->m_flags & 0x4800) {
		CycleTrackedPlayer(p_next, p_home);
	}

	g_viewMode = -1;
}

// Returns the camera player's view: its orientation and, from the cockpit, the position of its
// eye object (at half the object's height outside the external views).
// Stack-slot permutation of player, x, y and z.
// FUNCTION: MW2 0x100116c3
void GetPlayerEyeView(MechS32* p_pitch, MechS32* p_heading, MechS32* p_roll, MechS32* p_x, MechS32* p_y, MechS32* p_z)
{
	Player* player;
	MechS32 x;
	MechS32 y;
	MechS32 z;

	*p_pitch = *p_heading = *p_roll = *p_x = *p_y = *p_z = 0;
	player = g_localPlayer;
	if (!player) {
		return;
	}

	*p_pitch = player->m_pitch;
	*p_heading = player->m_heading;
	*p_roll = player->m_roll;
	if (player->m_eyeObj) {
		if (g_eyeHeightOffset) {
			*p_y += *g_eyeHeightOffset;
		}

		TransformPoint(GetObjWorldMatrix(player->m_eyeObj), p_x, p_y, p_z);
		GetObjWorldAngles(player->m_eyeObj, &x, &y, &z);
		if (g_localMechLost) {
			*p_pitch = x;
			*p_heading = y;
			*p_roll = z;
		}
		else {
			*p_pitch += player->m_torsoPitch;
			*p_heading += player->m_torsoTwist;
			*p_roll = z >> 1;
		}
	}
	else {
		*p_x = player->m_position.m_x;
		*p_y = player->m_position.m_y;
		*p_z = player->m_position.m_z;
	}
}

// The ordinance view: saves the view and zooms out the first time, then follows the tracked
// shot; once it is gone, returns to the previous view mode (restoring the saved view for the free
// camera).
// FUNCTION: MW2 0x10011819
void UpdateOrdinanceView(void)
{
	MechS32* camera;

	if (g_viewMode != c_viewOrdinance) {
		g_ordinanceReturnMode = g_viewMode;
		SaveView(g_eyepoint, g_ordinanceSavedView);
		g_zoomFov = 0x20000;
		ApplyCameraFov(0);
	}

	camera = GetTrackedShotView();
	if (camera) {
		RestoreView(g_eyepoint, camera);
	}
	else {
		SetViewMode(g_ordinanceReturnMode);
		if (g_ordinanceReturnMode == c_viewFreeEye) {
			RestoreView(g_eyepoint, g_ordinanceSavedView);
		}
	}
}

// The track view: places the camera behind the tracked player's mech: p_distance and p_height move it out and up,
// within limits taken from the mech's size the first time, p_turn turns it around the mech and
// p_tilt tilts it. The first call after another view starts its ramps from the current eyepoint.
// Stack-slot permutation of the locals. The original compares g_trackHeight with
// g_trackMaxHeight in the other operand order.
// FUNCTION: MW2 0x100118bc
void UpdateTrackView(MechS32 p_distance, MechS32 p_height, MechS32 p_tilt, MechS32 p_turn)
{
	MechS32 y;
	MechS32 dx;
	MechS32 z;
	MechS32 dy;
	MechS32 angle;
	MechS32 dz;
	MechS32 turn;
	MechS32 tilt;
	MechS32 floor;
	MechS32 offsetX;
	MechS32 pitch;
	MechS32 offsetY;
	MechS32 sine;
	MechS32 heading;
	MechS32 offsetZ;
	MechS32 roll;
	MechU32 distance;
	MechS32 height;
	MechS32 cosine;
	MechS32 x;
	MechS32 unused;
	Mech* mech;

	GetPlayerEyeView(&pitch, &heading, &roll, &x, &y, &z);
	if (g_viewMode != c_viewTrack) {
		if (!g_localMechLost) {
			PlayCockpitSound(0x11, -1);
		}

		if (g_trackDistance == 0) {
			mech = g_players[0]->m_mech;
			g_trackDistance = mech->m_radius * 3;
			g_trackMinDistance = g_trackDistance >> 1;
			g_trackMaxDistance = g_trackDistance << 2;
			g_trackHeight = g_trackDistance >> 2;
			g_trackMaxHeight = g_trackMaxDistance;
			g_trackMinHeight = -mech->m_height + 200;
		}

		g_trackOffsetX.m_time = g_currentClock;
		g_trackOffsetY.m_time = g_currentClock;
		g_trackOffsetZ.m_time = g_currentClock;
		g_trackHeading.m_time = g_currentClock;
		g_trackPitch.m_time = g_currentClock;
		g_trackOffsetX.m_value = g_eyepoint->m_x - x;
		g_trackOffsetY.m_value = g_eyepoint->m_y - y;
		g_trackOffsetZ.m_value = g_eyepoint->m_z - z;
		g_trackHeading.m_value = g_eyepoint->m_heading;
		g_trackPitch.m_value = g_eyepoint->m_pitch;
		g_eyepoint->m_roll = 0;
		if (g_eyepoint->m_x == x) {
			g_eyepoint->m_x += 10;
		}

		g_zoomFov = 0x10000;
		ApplyCameraFov(0);
	}

	g_trackDistance += p_distance;
	if (g_trackDistance > g_trackMaxDistance) {
		g_trackDistance = g_trackMaxDistance;
	}
	else if (g_trackDistance < g_trackMinDistance) {
		g_trackDistance = g_trackMinDistance;
	}

	height = p_height + g_trackHeight;
	g_trackTurn += p_turn;
	g_trackTurn %= 0x1680000;
	dy = y - g_eyepoint->m_y;
	dz = z - g_eyepoint->m_z;
	dx = x - g_eyepoint->m_x;
	GetBearingAndRange(dx, dy, dz, &turn, &unused, &distance, &tilt);
	SetWrappedRampTarget(&g_trackPitch, -tilt - (p_tilt >> 1));
	g_eyepoint->m_pitch = UpdateWrappedRamp(&g_trackPitch);
	SetWrappedRampTarget(&g_trackHeading, turn);
	g_eyepoint->m_heading = UpdateWrappedRamp(&g_trackHeading);
	angle = heading - g_trackTurn;
	angle %= 0x1680000;
	if (angle < -0xb40000) {
		angle += 0x1680000;
	}
	else if (angle > 0xb40000) {
		angle -= 0x1680000;
	}

	cosine = FixedCos(angle);
	sine = FixedSin(angle);
	offsetX = FixedMul16(g_trackDistance, sine) >> 13;
	offsetZ = FixedMul16(g_trackDistance, cosine) >> 13;
	offsetY = height;
	g_trackOffsetX.m_target = offsetX;
	g_eyepoint->m_x = x + UpdateRamp(&g_trackOffsetX);
	g_trackOffsetZ.m_target = offsetZ;
	g_eyepoint->m_z = z + UpdateRamp(&g_trackOffsetZ);
	floor = GetCameraFloor(g_eyepoint);
	if (y + offsetY < floor) {
		offsetY = floor - y;
	}

	g_trackHeight = height;
	if (g_trackHeight > g_trackMaxHeight) {
		g_trackHeight = g_trackMaxHeight;
	}
	else if (g_trackHeight < g_trackMinHeight) {
		g_trackHeight = g_trackMinHeight;
	}

	g_trackOffsetY.m_target = offsetY;
	g_eyepoint->m_y = y + UpdateRamp(&g_trackOffsetY);
}

// Updates the cockpit view each frame: resets the pilot's look ramps after an external view, turns
// the view towards a held glance key (or back ahead once all are released), then places the
// eyepoint unless the camera is shaking.
// FUNCTION: MW2 0x10011cb0
void UpdateCockpitView(void)
{
	if (g_viewMode) {
		g_pilotPan.m_time = g_currentClock;
		g_pilotPan.m_value = 0;
		g_pilotTilt.m_time = g_currentClock;
		g_pilotTilt.m_value = 0;
		ClearCameraShakeKeys();
		ApplyCameraFov(0);
	}

	g_inCockpitView = 1;
	g_cockpitEyeSteady = 0;
	if (g_sinkGlanceLeft) {
		g_sinkPilotPan = -0x460000;
	}
	else if (g_sinkGlanceRight) {
		g_sinkPilotPan = 0x460000;
	}
	else if (g_sinkGlanceUp) {
		g_sinkPilotTilt = -0x320000;
	}
	else if (g_sinkGlanceDown) {
		g_sinkPilotTilt = 0x280000;
	}
	else if (!g_glanceReleased) {
		g_sinkPilotPan = 0;
		g_sinkPilotTilt = 0;
	}

	if (!g_sinkGlanceRight && !g_sinkGlanceLeft && !g_sinkGlanceUp && !g_sinkGlanceDown) {
		g_glanceReleased = 1;
	}
	else {
		g_glanceReleased = 0;
	}

	if (!UpdateCameraShake()) {
		GetCockpitEyeView(
			&g_eyepoint->m_pitch,
			&g_eyepoint->m_heading,
			&g_eyepoint->m_roll,
			&g_eyepoint->m_x,
			&g_eyepoint->m_y,
			&g_eyepoint->m_z
		);
	}
}

// GetPlayerEyeView's view, turned from the cockpit by the pilot's pan and tilt.
// Stack-slot permutation: pan and tilt.
// FUNCTION: MW2 0x10011e45
void GetCockpitEyeView(MechS32* p_pitch, MechS32* p_heading, MechS32* p_roll, MechS32* p_x, MechS32* p_y, MechS32* p_z)
{
	MechS32 pan;
	MechS32 tilt;

	pan = 0;
	tilt = 0;
	GetPlayerEyeView(p_pitch, p_heading, p_roll, p_x, p_y, p_z);
	if (!g_localMechLost) {
		g_pilotPan.m_target = g_sinkPilotPan;
		pan = UpdateRamp(&g_pilotPan);
		g_pilotTilt.m_target = g_sinkPilotTilt;
		tilt = UpdateRamp(&g_pilotTilt);
		*p_heading += pan;
		*p_pitch += tilt;
	}

	g_cockpitEyeSteady = 1;
}

// Switches to the drop view (mode 4): the camera starts level at the mech and falls, turning.
// Stack-slot permutation: the six locals.
// FUNCTION: MW2 0x10011edc
void UpdateDropView(void)
{
	MechS32 unk0x10;
	MechS32 unk0x0c;
	MechS32 unk0x14;
	MechS32 x;
	MechS32 y;
	MechS32 z;

	GetPlayerEyeView(&unk0x10, &unk0x0c, &unk0x14, &x, &y, &z);
	if (g_viewMode != c_viewDrop) {
		g_dropSpeed = 0;
		g_eyepoint->m_pitch = 0x5a0000;
		g_dropStartClock = g_currentClock;
		ApplyCameraFov(0);
	}

	IntegrateMidpoint(&g_eyepoint->m_y, &g_dropSpeed, g_dropAcceleration, g_deltaTime);
	g_eyepoint->m_x = x;
	g_eyepoint->m_z = z;
	g_eyepoint->m_heading += (g_currentClock - g_dropStartClock) * 300;
}

// Moves the free camera (mode 2): p_climb raises it, p_speed drives it forward (eased),
// p_strafe moves it sideways, and p_turn and p_pitch turn it. It stays above the ground.
// Stack-slot permutation: sinHeading, cosHeading, cosPitch and speed.
// FUNCTION: MW2 0x10011f9a
void UpdateFreeEyeView(MechS32 p_climb, MechS32 p_speed, MechS32 p_strafe, MechS32 p_turn, MechS32 p_pitch)
{
	MechS32 sinHeading;
	MechS32 cosHeading;
	MechS32 sinPitch;
	MechS32 floor;
	MechS32 speed;
	MechS32 cosPitch;

	sinHeading = FixedSin(g_eyepoint->m_heading);
	cosHeading = FixedCos(g_eyepoint->m_heading);
	sinPitch = FixedSin(g_eyepoint->m_pitch);
	cosPitch = FixedCos(g_eyepoint->m_pitch);
	if (g_viewMode != c_viewFreeEye) {
		if (g_localMechLost) {
			g_eyepoint->m_x -= FixedMul16(g_trackDistance, sinHeading) >> 13;
			g_eyepoint->m_z -= FixedMul16(g_trackDistance, cosHeading) >> 13;
		}

		g_eyepoint->m_roll = 0;
		g_freeEyeSpeed.m_value = 0;
		g_freeEyeSpeed.m_time = g_currentClock;
		g_zoomFov = g_normalFov;
		ApplyCameraFov(0);
	}

	g_eyepoint->m_x += FixedMul16(p_strafe, cosHeading) >> 11;
	g_eyepoint->m_z -= FixedMul16(p_strafe, sinHeading) >> 11;
	g_eyepoint->m_pitch -= p_pitch;
	if (g_eyepoint->m_pitch > 0x5a0000) {
		g_eyepoint->m_pitch = 0x5a0000;
	}
	else if (g_eyepoint->m_pitch < -0x5a0000) {
		g_eyepoint->m_pitch = -0x5a0000;
	}

	g_eyepoint->m_y += p_climb * 4;
	g_freeEyeSpeed.m_target = p_speed * 16;
	speed = UpdateRamp(&g_freeEyeSpeed);
	if (speed < 0x20 && speed > -0x20) {
		speed = 0;
	}

	g_eyepoint->m_heading += p_turn;
	g_eyepoint->m_x -= FixedMul16(FixedMul16(speed, sinHeading) >> 13, cosPitch) >> 13;
	g_eyepoint->m_z -= FixedMul16(FixedMul16(speed, cosHeading) >> 13, cosPitch) >> 13;
	floor = GetCameraFloor(g_eyepoint);
	if (g_eyepoint->m_y < floor) {
		g_eyepoint->m_y = MECH_MAX(g_eyepoint->m_y, floor);
	}
}

// Turns the world's shapes of types 0x10 and 0x60 to face the eyepoint, or to a fixed angle when
// IsSatelliteView is set.
// FUNCTION: MW2 0x1001220a
void TurnBillboards(void)
{
	MechS32 pitch;
	Shape* shape;
	SceneObject* obj;
	MechS32 z;
	MechS32 y;
	MechS32 x;
	MechS32 heading;

	for (shape = g_sceneShapes->m_next; shape; shape = shape->m_next) {
		if ((shape->m_kind & 0xf0) == 0x10 || (shape->m_kind & 0xf0) == 0x60) {
			obj = shape->m_object;
			if (obj) {
				GetObjPosition(obj, &x, &y, &z);
				if (IsSatelliteView()) {
					heading = 0xb40000;
					pitch = -0x2d0000;
				}
				else {
					heading = FixedAtan2(g_eyepoint->m_x - x, g_eyepoint->m_z - z);
					pitch = 0;
				}

				SetObjRotation(obj, pitch, heading, 0, 0);
				UpdateObj(obj);
			}
		}
	}
}
