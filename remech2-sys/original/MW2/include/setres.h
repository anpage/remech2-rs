#ifndef SETRES_H
#define SETRES_H

#include "point.h"
#include "render.h"
#include "types.h"

// The functions and globals of setres.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_artResolution;
	extern Point g_artResolutionSizes[3];
	extern MechS32 g_pixelAspect;

	void SetPixelAspect(GameWindowGeometry* p_geometry);
	// Implemented on the Rust side (src/sim/window.rs)
	void ChooseArtResolution(GameWindowGeometry* p_geometry);
	void SetRes(void);

#ifdef __cplusplus
}
#endif

#endif // SETRES_H
