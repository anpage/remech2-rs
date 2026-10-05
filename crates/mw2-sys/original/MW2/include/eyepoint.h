#ifndef EYEPOINT_H
#define EYEPOINT_H

#include "decomp.h"
#include "ramp.h"
#include "savedview.h"
#include "transform.h"
#include "types.h"

// The view the scene is drawn from (g_eyepoint): its position and rotation, the light, the view
// rectangle on the pane (SelectPane sizes it to the pane; GetViewCenter reads its centre, moved
// by m_offsetX/m_offsetY), the clip planes and the projection UpdateProjection derives from them.
// The rotation is in 16.16 degrees: m_pitch about x, m_heading about y, m_roll about z, as
// BuildMatrix takes them (the overlay's eye position readout prints them in that order).
// SIZE 0xe0
typedef struct Eyepoint {
	MechS32 m_x;                             // 0x00
	MechS32 m_y;                             // 0x04
	MechS32 m_z;                             // 0x08
	MechS32 m_heading;                       // 0x0c
	MechS32 m_pitch;                         // 0x10 — clamped to +-90 degrees by the free camera
	MechS32 m_roll;                          // 0x14
	MechS32 m_fovX;                          // 0x18 — 16.16
	MechS32 m_lightX;                        // 0x1c — the light's position (an effect's flash moves it)
	MechS32 m_lightY;                        // 0x20
	MechS32 m_lightZ;                        // 0x24
	MechS16 m_directionalLight;              // 0x28 — nonzero: light from the origin's direction
	MechS16 m_ambientLight;                  // 0x2a — out of 0x80 (ComputeShade)
	MechS32 m_viewLeft;                      // 0x2c — the view rectangle, in pixels of the pane
	MechS32 m_viewRight;                     // 0x30
	MechS32 m_viewTop;                       // 0x34
	MechS32 m_viewBottom;                    // 0x38
	MechS32 m_nearPlane;                     // 0x3c
	MechS32 m_farPlane;                      // 0x40
	MechS32 m_pixelAspect;                   // 0x44 — pixel width / height, 16.16
	MechS32 m_unk0x48;                       // 0x48 — nothing uses it
	MechS32 m_offsetX;                       // 0x4c — moves the projection centre off the view's
	MechS32 m_offsetY;                       // 0x50
	Matrix m_viewMatrix;                     // 0x54 — rows 0-2 the view rotation, row 3 the eye position
	MechS32 m_halfWidth;                     // 0x84
	MechS32 m_halfHeight;                    // 0x88
	MechS32 m_centerX;                       // 0x8c
	MechS32 m_centerY;                       // 0x90
	MechS32 m_projectScaleX;                 // 0x94 — half width times the horizontal field of view
	MechS32 m_projectScaleY;                 // 0x98
	MechS32 m_projectScaleX16;               // 0x9c — m_projectScaleX in 16 bits, shifted by m_projectShiftX
	MechS32 m_projectScaleY16;               // 0xa0
	MechS16 m_projectShiftX;                 // 0xa4
	MechS16 m_projectShiftY;                 // 0xa6
	MechS32 m_frustumScaleX;                 // 0xa8 — the side planes' scale for the visibility tests
	MechS32 m_frustumScaleY;                 // 0xac — the top and bottom planes'
	MechS32 m_fovY;                          // 0xb0
	MechS32 m_cullDistance;                  // 0xb4 — the far plane, or 0x7fffffff for an unlimited one
	MechS32 m_detailScale;                   // 0xb8 — scales distances for the level of detail
	undefined4 m_unk0xbc[(0xe0 - 0xbc) / 4]; // 0xbc
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

	extern MechS32 g_normalFov;
	extern MechS32 g_zoomFov;
	extern MechS32 g_requestedViewMode;
	extern MechS32 g_initialViewMode;
	extern undefined4 g_inCockpitView;
	extern MechS32 g_lostViewMode;
	extern MechS32 g_spectating;
	extern struct Player* g_localPlayer;
	extern MechS32* g_eyeHeightOffset;
	extern MechS32* g_eyeTwist;
	extern MechS32 g_trackDistance;
	extern MechS32 g_trackMinDistance;
	extern MechS32 g_trackMaxDistance;
	extern MechS32 g_trackHeight;
	extern MechS32 g_trackTurn;
	extern MechS32 g_viewMode;
	extern MechS32 g_cockpitEyeSteady;
	extern MechS8 g_glanceReleased;
	extern MechS32 g_dropSpeed;
	extern MechS32 g_dropAcceleration;
	extern MechS32 g_dropStartClock;
	extern MechS32 g_ordinanceReturnMode;
	extern MechS32 g_trackedPlayer;
	extern MechS32 g_autopilotStart;
	extern MechS32 g_ordinanceSavedView[7];
	extern MechS32 g_trackMaxHeight;
	extern Ramp g_trackOffsetZ;
	extern WrappedRamp g_trackPitch;
	extern WrappedRamp g_trackHeading;
	extern MechS32 g_trackMinHeight;
	extern Ramp g_pilotTilt;
	extern Ramp g_trackOffsetY;
	extern SavedView g_savedViews[5];
	extern Ramp g_pilotPan;
	extern Ramp g_freeEyeSpeed;
	extern Ramp g_trackOffsetX;
	void FirstEyepoint(void);
	void UpdateCockpitView(void);
	void GetCockpitEyeView(
		MechS32* p_pitch,
		MechS32* p_heading,
		MechS32* p_roll,
		MechS32* p_x,
		MechS32* p_y,
		MechS32* p_z
	);
	MechS32 GetViewMode(void);
	MechS32 RestoreView(Eyepoint* p_eyepoint, MechS32* p_view);
	void CycleTrackedPlayer(MechS32 p_next, MechS32 p_home);
	void GetPlayerEyeView(
		MechS32* p_pitch,
		MechS32* p_heading,
		MechS32* p_roll,
		MechS32* p_x,
		MechS32* p_y,
		MechS32* p_z
	);
	void UpdateOrdinanceView(void);
	void SetViewMode(MechS32 p_zoom);
	MechS32 SaveView(Eyepoint* p_eyepoint, MechS32* p_view);
	void UpdateEyepoint(void);
	void ApplyCameraFov(MechS32 p_reset);
	void UpdateTrackView(MechS32 p_distance, MechS32 p_height, MechS32 p_tilt, MechS32 p_turn);
	void UpdateDropView(void);
	void UpdateFreeEyeView(MechS32 p_climb, MechS32 p_speed, MechS32 p_strafe, MechS32 p_turn, MechS32 p_pitch);
	void TurnBillboards(void);

#ifdef __cplusplus
}
#endif

#endif // EYEPOINT_H
