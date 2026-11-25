#ifndef COCKPIT_H
#define COCKPIT_H

#include "mappoint.h"
#include "navpoint.h"
#include "point.h"
#include "recttransition.h"
#include "shape.h"
#include "targeting.h"
#include "types.h"

struct Face;
struct Vertex;

// A gauge-drawing function of a cockpit layout, or one of the map view's hooks, stored as any of the
// four kinds below; a call casts it to its slot's. The original called them without a prototype,
// and passed IsInsideGaugeEllipse's x and y as one MechPoint.
typedef void (*CockpitGaugeFn)(void);
// m_gauges[0]: draws the view's outline
typedef void (*CockpitOutlineFn)(PANE* p_target, int32_t p_color);
// m_gauges[1]: whether a point is inside the view
typedef int32_t (*CockpitInsideFn)(PANE* p_target, int32_t p_x, int32_t p_y);
// m_gauges[2]: where the edge's needle towards a point ends
typedef Point* (*CockpitNeedleTowardFn)(PANE* p_target, Point* p_point, Point* p_out);
// m_gauges[3]: where the edge's needle at an angle ends
typedef Point* (*CockpitNeedleAtFn)(PANE* p_target, int32_t p_angle, Point* p_out);

// The layout of one cockpit view (4: the satellite view).
typedef struct CockpitLayout {
	PANE* m_viewport;             // 0x00 — in 16.16 fractions of the screen
	PANE* m_savedViewport;        // 0x04 — the viewport, saved while a transition moves it
	int32_t m_paneSlot;           // 0x08 — in g_panes
	int32_t m_sounds[2];          // 0x0c — the sounds of entering and leaving the view, -1: none
	RectTransition* m_transition; // 0x14
	int32_t m_range;              // 0x18 — the range the readout shows
	int32_t m_formattedRange;     // 0x1c — the range it last formatted
	int32_t m_startRange;         // 0x20 — the range to start at
	int32_t m_minRange;           // 0x24 — the shortest range
	int32_t m_maxRange;           // 0x28 — the longest range
	int32_t m_zoom;               // 0x2c — the zoom: m_startRange over m_range
	int32_t m_font;               // 0x30 — its font, from g_artResolution
	char* m_extraLabel;           // 0x34 — a third label and text, which nothing draws
	char* m_extraText;            // 0x38
	char* m_rangeLabel;           // 0x3c — the range label
	char* m_rangeText;            // 0x40 — the range text
	char* m_headingLabel;         // 0x44 — the heading label
	char* m_headingText;          // 0x48 — the heading text
	int32_t m_formattedHeading;   // 0x4c — the heading it last formatted
	char* m_shortUnit;            // 0x50 — the unit of short ranges
	char* m_longUnit;             // 0x54 — the unit of long ranges
	Point m_extraTextOrigin;      // 0x58
	Point m_rangeTextOrigin;      // 0x60
	Point m_headingTextOrigin;    // 0x68
	int32_t (*m_icons)[3];        // 0x70 — SHP ids by row and side: 0 the center, 1 players
	int32_t* m_colors;            // 0x74
	int32_t* m_anims;             // 0x78 — 2D animations; [2] plays over a damaged map view
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
	extern int32_t g_satelliteStaticState;
	extern void (*g_savedFrameDrawCallback)(void);
	extern int32_t g_savedShowCrosshair;
	extern uint32_t g_savedShowHud;
	extern int32_t g_savedShowTargetMarker;
	extern int32_t g_hudSettingsSaved;
	extern int32_t g_mapShadeTop;
	extern int32_t g_mapShadeBase;
	extern int32_t g_mapShadeRange;
	extern int32_t g_previousCockpitView;
	extern int32_t g_previousMapViewMode;
	extern int32_t g_cockpitLayoutIndex;
	extern int32_t g_mapViewMode;
	extern int32_t g_requestedCockpitView;
	extern int32_t g_mapFollowsFreeEye;
	extern int32_t g_satelliteClean;
	extern int32_t g_mapDamageAnimShown;
	extern int32_t g_staticCleanUntil;
	extern int32_t g_staticNoiseUntil;

	void LoadCockpitLayout(int32_t p_cockpit, CockpitLayout* p_layout);
	void InitCockpitViews(void);
	void RunMapView(void);
	int32_t SwitchCockpitView(void);
	void DrawCockpitView(void);
	void DrawMapView(void);
	void DrawMapContents(CockpitLayout* p_layout);
	void DrawMapIcon(CockpitLayout* p_layout, MapPoint p_pos, int32_t p_icon);
	void DrawMapUnits(CockpitLayout* p_layout);
	int32_t IsNavReached(NavPoint* p_nav, int32_t p_team);
	void DrawMapTarget(CockpitLayout* p_layout);
	void DrawMapNavPoints(CockpitLayout* p_layout);
	void DrawMapFieldOfView(CockpitLayout* p_layout, int32_t p_heading);
	void DrawMapViewText(CockpitLayout* p_layout);
	void CycleCockpitView(void);
	int32_t IsSatelliteView(void);
	void LeaveSatelliteView(void);
	void ToggleSatelliteView(void);
	void ZoomMapView(int32_t p_zoom);
	int32_t MapShapeFilter(Shape* p_shape);
	uint32_t SatelliteFaceColor(struct Face* p_face, struct Vertex* p_vertices, uint32_t p_flags, int32_t p_distance);
	void SatelliteDrawPolygon(int32_t p_count, uint32_t* p_points, uint32_t p_flags);
	int32_t GetMapHeightShade(CockpitLayout* p_layout, int32_t p_height);
	int32_t DrawMapViewTransition(
		int32_t p_reverse,
		CockpitLayout* p_layout,
		RectTransition* p_transition,
		int32_t p_final
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
