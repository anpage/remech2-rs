#ifndef COCKPIT_H
#define COCKPIT_H

#include "decomp.h"
#include "mappoint.h"
#include "navpoint.h"
#include "point.h"
#include "recttransition.h"
#include "shape.h"
#include "targeting.h"
#include "types.h"

// A gauge-drawing function of a cockpit layout, or one of the map view's hooks.
typedef MechS32 (*CockpitGaugeFn)();

// The layout of one cockpit view (4: the satellite view).
typedef struct CockpitLayout {
	PANE* m_viewport;             // 0x00 — in 16.16 fractions of the screen
	PANE* m_savedViewport;        // 0x04 — the viewport, saved while a transition moves it
	MechS32 m_paneSlot;           // 0x08 — in g_panes
	MechS32 m_sounds[2];          // 0x0c — the sounds of entering and leaving the view, -1: none
	RectTransition* m_transition; // 0x14
	MechS32 m_range;              // 0x18 — the range the readout shows
	MechS32 m_formattedRange;     // 0x1c — the range it last formatted
	MechS32 m_startRange;         // 0x20 — the range to start at
	MechS32 m_minRange;           // 0x24 — the shortest range
	MechS32 m_maxRange;           // 0x28 — the longest range
	MechS32 m_zoom;               // 0x2c — the zoom: m_startRange over m_range
	MechS32 m_font;               // 0x30 — its font, from g_artResolution
	MechChar* m_extraLabel;       // 0x34 — a third label and text, which nothing draws
	MechChar* m_extraText;        // 0x38
	MechChar* m_rangeLabel;       // 0x3c — the range label
	MechChar* m_rangeText;        // 0x40 — the range text
	MechChar* m_headingLabel;     // 0x44 — the heading label
	MechChar* m_headingText;      // 0x48 — the heading text
	MechS32 m_formattedHeading;   // 0x4c — the heading it last formatted
	MechChar* m_shortUnit;        // 0x50 — the unit of short ranges
	MechChar* m_longUnit;         // 0x54 — the unit of long ranges
	Point m_extraTextOrigin;      // 0x58
	Point m_rangeTextOrigin;      // 0x60
	Point m_headingTextOrigin;    // 0x68
	MechS32 (*m_icons)[3];        // 0x70 — SHP ids by row and side: 0 the center, 1 players
	MechS32* m_colors;            // 0x74
	MechS32* m_anims;             // 0x78 — 2D animations; [2] plays over a damaged map view
	CockpitGaugeFn m_gauges[4];   // 0x7c — indices in g_cockpitGauges until loaded; in the
								  // map view, 1 tests a point, 2 clamps it to the view
								  // and 3 projects a bearing to its edge
} CockpitLayout;

// The functions and globals of cockpit.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern CockpitGaugeFn g_cockpitGauges[10];
	extern PANE g_cockpitGaugePanes[5];
	extern void* g_unk0x100a5bb8[4];
	extern MechS32 g_satelliteStaticState;
	extern void (*g_savedFrameDrawCallback)(void);
	extern MechS32 g_savedShowCrosshair;
	extern undefined4 g_savedShowHud;
	extern MechS32 g_savedShowTargetMarker;
	extern MechS32 g_hudSettingsSaved;
	extern MechS32 g_mapShadeTop;
	extern MechS32 g_mapShadeBase;
	extern MechS32 g_mapShadeRange;
	extern MechS32 g_previousCockpitView;
	extern MechS32 g_previousMapViewMode;
	extern MechS32 g_cockpitLayoutIndex;
	extern MechS32 g_mapViewMode;
	extern MechS32 g_requestedCockpitView;
	extern MechS32 g_mapFollowsFreeEye;
	extern MechS32 g_satelliteClean;

	void LoadCockpitLayout(MechS32 p_cockpit, CockpitLayout* p_layout);
	void InitCockpitViews(void);
	void RunMapView(void);
	MechS32 SwitchCockpitView(void);
	void DrawCockpitView(void);
	void DrawMapView(void);
	void DrawMapContents(CockpitLayout* p_layout);
	void DrawMapIcon(CockpitLayout* p_layout, MapPoint p_pos, MechS32 p_icon);
	void DrawMapUnits(CockpitLayout* p_layout);
	MechS32 IsNavReached(NavPoint* p_nav, MechS32 p_team);
	void DrawMapTarget(CockpitLayout* p_layout);
	void DrawMapNavPoints(CockpitLayout* p_layout);
	void DrawMapFieldOfView(CockpitLayout* p_layout, MechS32 p_heading);
	void DrawMapViewText(CockpitLayout* p_layout);
	void CycleCockpitView(void);
	MechS32 IsSatelliteView(void);
	void LeaveSatelliteView(void);
	void ToggleSatelliteView(void);
	void ZoomMapView(MechS32 p_zoom);
	MechS32 MapShapeFilter(Shape* p_shape);
	MechU32 SatelliteFaceColor(struct Face* p_face, undefined4 p_unk0x04, MechU32 p_flags);
	void SatelliteDrawPolygon(MechS32 p_count, MechU32* p_points, MechU32 p_flags);
	MechS32 GetMapHeightShade(CockpitLayout* p_layout, MechS32 p_height);
	MechS32 DrawMapViewTransition(
		MechS32 p_reverse,
		CockpitLayout* p_layout,
		RectTransition* p_transition,
		MechS32 p_final
	);
	void PowerUpMapView(void);
	void DrawSatelliteStatic(void);
	void DrawDamagedMapView(void);
	void PowerDownMapView(void);
	void ResetMapView(void);

#ifdef __cplusplus
}
#endif

#endif // COCKPIT_H
