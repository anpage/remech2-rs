#ifndef POLYDRAW_H
#define POLYDRAW_H

#include "eyepoint.h"
#include "rendersettings.h"
#include "types.h"

// The functions and globals of polydraw.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern Eyepoint* g_eyepoint;
	extern Eyepoint g_mainEyepoint;
	extern RenderSettings g_renderSettings;
	extern int32_t g_horizonBandHeight;
	extern int32_t g_lineEndX;
	extern int32_t g_lineEndY;
	void DrawScenePolygon(int32_t p_count, uint32_t* p_points, uint32_t p_flags);
	void DrawSkyAndGround(Eyepoint* p_eyepoint);
	void DrawPentagon(
		int32_t p_x0,
		int32_t p_y0,
		int32_t p_x1,
		int32_t p_y1,
		int32_t p_x2,
		int32_t p_y2,
		int32_t p_x3,
		int32_t p_y3,
		int32_t p_x4,
		int32_t p_y4,
		uint32_t p_flags
	);
	void DrawQuad(
		int32_t p_x0,
		int32_t p_y0,
		int32_t p_x1,
		int32_t p_y1,
		int32_t p_x2,
		int32_t p_y2,
		int32_t p_x3,
		int32_t p_y3,
		uint32_t p_flags
	);
	void DrawTriangle(
		int32_t p_x0,
		int32_t p_y0,
		int32_t p_x1,
		int32_t p_y1,
		int32_t p_x2,
		int32_t p_y2,
		uint32_t p_flags
	);
	void DrawLineTo(int32_t p_x, int32_t p_y, uint32_t p_color);
	void DrawPolygonOrLine(int32_t p_count, uint32_t* p_points, uint32_t p_flags);

#ifdef __cplusplus
}
#endif

#endif // POLYDRAW_H
