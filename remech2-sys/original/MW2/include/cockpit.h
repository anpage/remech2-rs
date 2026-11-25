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
	PANE* m_viewport;      // in 16.16 fractions of the screen
	PANE* m_savedViewport; // the viewport, saved while a transition moves it
	int32_t m_paneSlot;    // in g_panes
	int32_t m_sounds[2];   // the sounds of entering and leaving the view, -1: none
	RectTransition* m_transition;
	int32_t m_range;          // the range the readout shows
	int32_t m_formattedRange; // the range it last formatted
	int32_t m_startRange;     // the range to start at
	int32_t m_minRange;       // the shortest range
	int32_t m_maxRange;       // the longest range
	int32_t m_zoom;           // the zoom: m_startRange over m_range
	int32_t m_font;           // its font, from g_artResolution
	char* m_extraLabel;       // a third label and text, which nothing draws
	char* m_extraText;
	char* m_rangeLabel;         // the range label
	char* m_rangeText;          // the range text
	char* m_headingLabel;       // the heading label
	char* m_headingText;        // the heading text
	int32_t m_formattedHeading; // the heading it last formatted
	char* m_shortUnit;          // the unit of short ranges
	char* m_longUnit;           // the unit of long ranges
	Point m_extraTextOrigin;
	Point m_rangeTextOrigin;
	Point m_headingTextOrigin;
	int32_t (*m_icons)[3]; // SHP ids by row and side: 0 the center, 1 players
	int32_t* m_colors;
	int32_t* m_anims;           // 2D animations; [2] plays over a damaged map view
	CockpitGaugeFn m_gauges[4]; // indices in g_cockpitGauges until loaded; in the
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
