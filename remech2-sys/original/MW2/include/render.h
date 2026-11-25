#ifndef RENDER_H
#define RENDER_H

#include "decomp.h"
#include "targeting.h"
#include "types.h"
#include "window.h"

typedef struct GameWindowGeometry {
	MechS32 m_width;  // 0x00
	MechS32 m_height; // 0x04
} GameWindowGeometry;

struct Shape;

// The functions and globals of render.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern void* g_bannerBuffer;
	extern MechS32 g_projectionDirty;
	extern MechS32 g_displayReady;
	extern MechS32 g_showBoundingSpheres;
	extern MechS32 g_drawModeIndex;
	extern MechS32 g_initDrawModeParam2;
	extern PANE g_screenPane;
	extern PANE g_currentPane;
	extern WINDOW g_mainPixelBuffer;
	extern MechS32 g_hasLightObject;
	extern MechS32 g_lightFollowsObject;
	extern MechS32 g_lightObject;
	extern GameWindowGeometry* g_gameWindowGeometry;
	extern MechS32 g_screenHeight;
	extern MechS32 g_screenHeightMinus1;
	extern MechS32 g_screenPixelCount;
	extern MechS32 g_screenWidth;
	extern MechS32 g_screenWidthMinus1;
	extern MechS32 g_screenHalfWidth;
	extern MechS32 g_screenHalfHeight;
	extern MechS32 g_stretchPending;
	extern MechS32 g_framePane;
	extern MechS32 g_drawnPolygonCount;
	extern struct SceneObject* g_skyObject;
	extern struct SceneObject* g_cockpitObject;

	MechS32 InitGameWindowGeometry(void);
	MechS32 InitDisplayGeometry(void);
	void FirstRender(void);
	void SecondRender(void);
	void DrawScene(void);
	void SetFramePane(MechS32 p_value);
	void ResetPane(void);
	void Blit(void);
	void ShutdownRender(void);
	void DrawBoundingSpheres(struct Shape* p_root);

#ifdef __cplusplus
}
#endif

#endif // RENDER_H
