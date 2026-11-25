#ifndef SCREENSCALE_H
#define SCREENSCALE_H

#include "gaugequadrant.h"
#include "point.h"
#include "rect.h"
#include "targeting.h"
#include "types.h"
#include "window.h"

// The functions and globals of screenscale.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern Point g_textMargins;
	extern int32_t g_pulseInset;
	extern int32_t g_pulseColor;
	extern int32_t g_pulseTime;
	extern int32_t g_pulseStep;

	PANE* ScaleRectToScreen(WINDOW* p_buffer, PANE* p_src, PANE* p_dst);
	PANE* ScaleRectToFrame(PANE* p_frame, PANE* p_src, PANE* p_dst);
	Point* ScalePointToScreen(WINDOW* p_buffer, Point* p_src, Point* p_dst);
	Point* ScalePointToFrame(PANE* p_frame, Point* p_src, Point* p_dst);
	PANE* ScaleRectToLowRes(PANE* p_rect, int32_t p_aspect);
	PANE* ScaleRectFromLowRes(PANE* p_src, PANE* p_dst);
	Point* ScalePointFromLowRes(Point* p_src, Point* p_dst);
	PANE* CenterRectOnScreen(WINDOW* p_buffer, PANE* p_src, PANE* p_dst);
	Rect* ScaleBoundsToScreen(WINDOW* p_buffer, Rect* p_src, Rect* p_dst);
	Rect* ScaleBoundsToFrame(PANE* p_frame, Rect* p_src, Rect* p_dst);
	Rect* ScaleBoundsFromLowRes(Rect* p_src, Rect* p_dst);
	PANE* FitRectToShape(PANE* p_src, PANE* p_dst, void* p_shape, int32_t p_frame);
	PANE* FitRectToGif(PANE* p_src, PANE* p_dst, void* p_shape);
	void OutlinePane(PANE* p_target, int32_t p_color);
	void DrawRuleUnderRow(PANE* p_target, Point p_pos, void* p_font, int32_t p_color);
	void UnderlineText(PANE* p_target, char* p_text, Point p_pos, void* p_font, int32_t p_color);
	void BoxText(PANE* p_target, char* p_text, Point p_pos, void* p_font, int32_t p_color);
	void DrawWrappedText(PANE* p_target, char* p_text, void* p_font);
	PANE* FitRectToText(char* p_text, void* p_font, PANE* p_rect);
	void DrawPulsingFrame(PANE* p_target);
	void TilePane(PANE* p_target, void* p_shape, int32_t p_frame);
	int32_t GetLineSlope(int32_t p_dx, int32_t p_dy, int32_t* p_slope);
	Point* GetRectNeedleToward(PANE* p_target, Point* p_point, Point* p_out);
	Point* GetRectNeedleAt(PANE* p_target, int32_t p_angle, Point* p_out);
	Point* GetRectEdgeAtSlope(Point* p_half, GaugeQuadrant p_quadrant, int32_t p_slope, Point* p_out);
	void DrawGaugeEllipse(PANE* p_target, int32_t p_color);
	void FillGaugeEllipse(PANE* p_target, Rect* p_rect, int32_t p_color);
	int32_t IsInsideGaugeEllipse(PANE* p_target, int32_t p_x, int32_t p_y);
	Point* GetEllipseNeedleToward(PANE* p_target, Point* p_point, Point* p_out);
	Point* GetEllipseNeedleAt(PANE* p_target, int32_t p_angle, Point* p_out);
	Point* GetEllipseEdgeAtAngle(
		PANE* p_target,
		Point* p_center,
		GaugeQuadrant p_quadrant,
		int32_t p_angle,
		Point* p_out
	);
	PANE* ScaleRectAboutCenter(PANE* p_src, PANE* p_dst, Point p_scale);

#ifdef __cplusplus
}
#endif

#endif // SCREENSCALE_H
