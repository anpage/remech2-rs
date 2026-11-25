#ifndef EYEPOINT_H
#define EYEPOINT_H

#include "ramp.h"
#include "savedview.h"
#include "transform.h"
#include "types.h"

// The view the scene is drawn from (g_eyepoint): its position and rotation, the light, the view
// rectangle on the pane (SelectPane sizes it to the pane; GetViewCenter reads its centre, moved
// by m_offsetX/m_offsetY), the clip planes and the projection UpdateProjection derives from them.
// The rotation is in 16.16 degrees: m_pitch about x, m_heading about y, m_roll about z, as
// BuildMatrix takes them (the overlay's eye position readout prints them in that order).
typedef struct Eyepoint {
	int32_t m_x;                // 0x00
	int32_t m_y;                // 0x04
	int32_t m_z;                // 0x08
	int32_t m_heading;          // 0x0c
	int32_t m_pitch;            // 0x10 — clamped to +-90 degrees by the free camera
	int32_t m_roll;             // 0x14
	int32_t m_fovX;             // 0x18 — 16.16
	int32_t m_lightX;           // 0x1c — the light's position (an effect's flash moves it)
	int32_t m_lightY;           // 0x20
	int32_t m_lightZ;           // 0x24
	int16_t m_directionalLight; // 0x28 — nonzero: light from the origin's direction
	int16_t m_ambientLight;     // 0x2a — out of 0x80 (ComputeShade)
	int32_t m_viewLeft;         // 0x2c — the view rectangle, in pixels of the pane
	int32_t m_viewRight;        // 0x30
	int32_t m_viewTop;          // 0x34
	int32_t m_viewBottom;       // 0x38
	int32_t m_nearPlane;        // 0x3c
	int32_t m_farPlane;         // 0x40
	int32_t m_pixelAspect;      // 0x44 — pixel width / height, 16.16
	int32_t m_offsetX;          // 0x4c — moves the projection centre off the view's
	int32_t m_offsetY;          // 0x50
	Matrix m_viewMatrix;        // 0x54 — rows 0-2 the view rotation, row 3 the eye position
	int32_t m_halfWidth;        // 0x84
	int32_t m_halfHeight;       // 0x88
	int32_t m_centerX;          // 0x8c
	int32_t m_centerY;          // 0x90
	int32_t m_projectScaleX;    // 0x94 — half width times the horizontal field of view
	int32_t m_projectScaleY;    // 0x98
	int32_t m_projectScaleX16;  // 0x9c — m_projectScaleX in 16 bits, shifted by m_projectShiftX
	int32_t m_projectScaleY16;  // 0xa0
	int16_t m_projectShiftX;    // 0xa4
	int16_t m_projectShiftY;    // 0xa6
	int32_t m_frustumScaleX;    // 0xa8 — the side planes' scale for the visibility tests
	int32_t m_frustumScaleY;    // 0xac — the top and bottom planes'
	int32_t m_fovY;             // 0xb0
	int32_t m_cullDistance;     // 0xb4 — the far plane, or 0x7fffffff for an unlimited one
	int32_t m_detailScale;      // 0xb8 — scales distances for the level of detail
} Eyepoint;

// The camera's view modes (SetViewMode), after the game keys that pick them.
enum {
	c_viewCockpit = 0,   // COCKPIT_VIEW
	c_viewTrack = 1,     // TRACK_VIEW: behind a tracked player
	c_viewFreeEye = 2,   // FREEEYE_VIEW
	c_viewOrdinance = 3, // ORDINANCE_VIEW: follows the local player's last shot
	c_viewDrop = 4,      // after ejecting: the camera drops past the mech
	c_viewSatellite = 6  // the satellite view's render, from the cockpit
};

// The functions of eyepoint.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_normalFov;
	extern int32_t g_zoomFov;
	extern int32_t g_requestedViewMode;
	extern int32_t g_initialViewMode;
	extern uint32_t g_inCockpitView;
	extern int32_t g_lostViewMode;
	extern int32_t g_spectating;
	extern struct Player* g_localPlayer;
	extern int32_t* g_eyeHeightOffset;
	extern int32_t* g_eyeTwist;
	extern int32_t g_trackDistance;
	extern int32_t g_trackMinDistance;
	extern int32_t g_trackMaxDistance;
	extern int32_t g_trackHeight;
	extern int32_t g_trackTurn;
	extern int32_t g_viewMode;
	extern int32_t g_cockpitEyeSteady;
	extern int8_t g_glanceReleased;
	extern int32_t g_dropSpeed;
	extern int32_t g_dropAcceleration;
	extern int32_t g_dropStartClock;
	extern int32_t g_ordinanceReturnMode;
	extern int32_t g_trackedPlayer;
	extern int32_t g_autopilotStart;
	extern int32_t g_ordinanceSavedView[7];
	extern int32_t g_trackMaxHeight;
	extern Ramp g_trackOffsetZ;
	extern WrappedRamp g_trackPitch;
	extern WrappedRamp g_trackHeading;
	extern int32_t g_trackMinHeight;
	extern Ramp g_pilotTilt;
	extern Ramp g_trackOffsetY;
	extern SavedView g_savedViews[5];
	extern Ramp g_pilotPan;
	extern Ramp g_freeEyeSpeed;
	extern Ramp g_trackOffsetX;
	void FirstEyepoint(void);
	void UpdateCockpitView(void);
	void GetCockpitEyeView(
		int32_t* p_pitch,
		int32_t* p_heading,
		int32_t* p_roll,
		int32_t* p_x,
		int32_t* p_y,
		int32_t* p_z
	);
	int32_t GetViewMode(void);
	int32_t RestoreView(Eyepoint* p_eyepoint, int32_t* p_view);
	void CycleTrackedPlayer(int32_t p_next, int32_t p_home);
	void GetPlayerEyeView(
		int32_t* p_pitch,
		int32_t* p_heading,
		int32_t* p_roll,
		int32_t* p_x,
		int32_t* p_y,
		int32_t* p_z
	);
	void UpdateOrdinanceView(void);
	void SetViewMode(int32_t p_zoom);
	int32_t SaveView(Eyepoint* p_eyepoint, int32_t* p_view);
	void UpdateEyepoint(void);
	void ApplyCameraFov(int32_t p_reset);
	void UpdateTrackView(int32_t p_distance, int32_t p_height, int32_t p_tilt, int32_t p_turn);
	void UpdateDropView(void);
	void UpdateFreeEyeView(int32_t p_climb, int32_t p_speed, int32_t p_strafe, int32_t p_turn, int32_t p_pitch);
	void TurnBillboards(void);

	// Implemented in Rust (src/sim/timing.rs)
	int32_t SinkStep(int32_t p_delta, int32_t p_shift);

#ifdef __cplusplus
}
#endif

#endif // EYEPOINT_H
