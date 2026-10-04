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
	extern MechS32 g_horizonBandHeight;
	void DrawScenePolygon(MechS32 p_count, MechU32* p_points, MechU32 p_flags);
	void DrawSkyAndGround(Eyepoint* p_eyepoint);
	void DrawPentagon(
		MechS32 p_x0,
		MechS32 p_y0,
		MechS32 p_x1,
		MechS32 p_y1,
		MechS32 p_x2,
		MechS32 p_y2,
		MechS32 p_x3,
		MechS32 p_y3,
		MechS32 p_x4,
		MechS32 p_y4,
		MechU32 p_flags
	);
	void DrawQuad(
		MechS32 p_x0,
		MechS32 p_y0,
		MechS32 p_x1,
		MechS32 p_y1,
		MechS32 p_x2,
		MechS32 p_y2,
		MechS32 p_x3,
		MechS32 p_y3,
		MechU32 p_flags
	);
	void DrawTriangle(
		MechS32 p_x0,
		MechS32 p_y0,
		MechS32 p_x1,
		MechS32 p_y1,
		MechS32 p_x2,
		MechS32 p_y2,
		MechU32 p_flags
	);
	void DrawLineTo(MechS32 p_x, MechS32 p_y, MechU32 p_color);
	void DrawPolygonOrLine(MechS32 p_count, MechU32* p_points, MechU32 p_flags);

#ifdef __cplusplus
}
#endif

#endif // POLYDRAW_H
