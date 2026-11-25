#ifndef RENDER_H
#define RENDER_H

#include "decomp.h"
#include "targeting.h"
#include "types.h"
#include "window.h"

typedef struct GameWindowGeometry {
	int32_t m_width;  // 0x00
	int32_t m_height; // 0x04
} GameWindowGeometry;

struct Shape;

// The functions and globals of render.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern void* g_bannerBuffer;
	extern int32_t g_projectionDirty;
	extern int32_t g_displayReady;
	extern int32_t g_showBoundingSpheres;
	extern int32_t g_drawModeIndex;
	extern int32_t g_initDrawModeParam2;
	extern PANE g_screenPane;
	extern PANE g_currentPane;
	extern WINDOW g_mainPixelBuffer;
	extern int32_t g_hasLightObject;
	extern int32_t g_lightFollowsObject;
	extern int32_t g_lightObject;
	extern GameWindowGeometry* g_gameWindowGeometry;
	extern int32_t g_screenHeight;
	extern int32_t g_screenHeightMinus1;
	extern int32_t g_screenPixelCount;
	extern int32_t g_screenWidth;
	extern int32_t g_screenWidthMinus1;
	extern int32_t g_screenHalfWidth;
	extern int32_t g_screenHalfHeight;
	extern int32_t g_stretchPending;
	extern int32_t g_framePane;
	extern int32_t g_drawnPolygonCount;
	extern struct SceneObject* g_skyObject;
	extern struct SceneObject* g_cockpitObject;

	int32_t InitGameWindowGeometry(void);
	int32_t InitDisplayGeometry(void);
	void FirstRender(void);
	void SecondRender(void);
	void DrawScene(void);
	void SetFramePane(int32_t p_value);
	void ResetPane(void);
	void Blit(void);
	void ShutdownRender(void);
	void DrawBoundingSpheres(struct Shape* p_root);

#ifdef __cplusplus
}
#endif

#endif // RENDER_H
