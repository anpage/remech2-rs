#include "render.h"

#include "animation.h"
#include "cockpit.h"
#include "collision.h"
#include "decomp.h"
#include "depthsort.h"
#include "displaybackend.h"
#include "error.h"
#include "eyepoint.h"
#include "faceshade.h"
#include "files.h"
#include "fixedmul.h"
#include "geocache.h"
#include "hud.h"
#include "muldiv14.h"
#include "object.h"
#include "objectanim.h"
#include "palette.h"
#include "palettecolor.h"
#include "palidentity.h"
#include "polydraw.h"
#include "recordstacks.h"
#include "refreshmode.h"
#include "screenscale.h"
#include "setres.h"
#include "shape.h"
#include "shapelists.h"
#include "simmain.h"
#include "targeting.h"
#include "types.h"
#include "vfxa.h"
#include "vfxrend.h"
#include "view.h"

#include <string.h>

int32_t g_drawModeIndex = -1;

int32_t g_initDrawModeParam2 = 1;

int32_t g_showBoundingSpheres = 0;

void* g_bannerBuffer = NULL;

int32_t g_projectionDirty = 1;

int32_t g_displayReady = 0;

int32_t g_framePane = 0;

int32_t g_hasLightObject = 0;

// Cleared while an effect has the camera, set again when it gives it back.
int32_t g_lightFollowsObject = 1;

int32_t g_lightObject = -1;

// The object of the scene's shape of kind 0x90, made by SecondRender.
SceneObject* g_skyObject = NULL;

// The object of the scene's shape of kind 0xa0.
SceneObject* g_cockpitObject = NULL;

int32_t g_drawnPolygonCount = 0;

PANE g_screenPane;

GameWindowGeometry* g_gameWindowGeometry;

int32_t g_screenHeight;

// Set when the next Blit should stretch the current pane over the window.
int32_t g_stretchPending;

int32_t g_screenHeightMinus1;

int32_t g_screenPixelCount;

int32_t g_screenWidth;

PANE g_currentPane;

int32_t g_screenWidthMinus1;

int32_t g_screenHalfWidth;

int32_t g_screenHalfHeight;

WINDOW g_mainPixelBuffer;

// InitGameWindowGeometry is implemented on the Rust side (src/sim/window.rs). In widescreen the
// geometry is the 4:3 box the HUD is laid out in, not the whole frame.

int32_t InitDisplayGeometry(void)
{
	int32_t result;

	result = 0;
	if (InitGameWindowGeometry()) {
		ChooseArtResolution(g_gameWindowGeometry);
		SetPixelAspect(g_gameWindowGeometry);
		g_currentPane.m_window = &g_mainPixelBuffer;
		g_currentPane.m_x0 = 0;
		g_currentPane.m_y0 = 0;
		g_currentPane.m_x1 = g_gameWindowGeometry->m_width - 1;
		g_currentPane.m_y1 = g_gameWindowGeometry->m_height - 1;
		g_screenPane = g_currentPane;
		result = 1;
		InitPanes(&g_currentPane);
		ResetTextColors();
		g_displayReady = 1;
	}

	return result;
}

// Sets up the vertex buffers and the scene, and installs the normal render hooks.
// The original first made VFX's code block (GetCodeBlock) writable, as its drawing routines
// patched themselves, failing with error 0x4d. The portable C patches nothing.
void FirstRender(void)
{
	InitializeDrawBuffer(0x80, 0x5dc);
	g_maxPolygons = 0x578;
	InitShapeLists();
	g_renderSettings.m_frameDrawCallback = DrawScene;
	g_renderSettings.m_shapeFilter = CullSceneShape;
	g_renderSettings.m_projectVertex = ProjectVertex;
	g_renderSettings.m_drawFace = GetFaceColor;
	g_renderSettings.m_drawPolygon = DrawScenePolygon;
	if (g_renderSettings.m_drawSky || g_renderSettings.m_drawGround) {
		g_renderSettings.m_clearFrame = 0;
	}

	g_renderSettings.m_flags |= 8;
}

// Makes the objects of the scene's shapes of kinds 0x90 and 0xa0, and sets up its shapes of kind
// 0x70 and type 4.
void SecondRender(void)
{
	Shape* root;
	Shape* shape;
	Shape* next;

	root = g_sceneShapes;
	if (!root) {
		return;
	}

	for (shape = root->m_next; shape; shape = shape->m_next) {
		if ((shape->m_kind & 0xf0) == 0x90) {
			g_skyObject = GetShapeObject(shape);
			DetachObjTreeShapes(g_skyObject);
			break;
		}
	}

	for (shape = root->m_next; shape; shape = shape->m_next) {
		if ((shape->m_kind & 0xf0) == 0xa0) {
			g_cockpitObject = GetShapeObject(shape);
			break;
		}
	}

	for (shape = root->m_next; shape; shape = next) {
		next = shape->m_next;
		if ((shape->m_kind & 0xf0) == 0x70) {
			HideShape(shape);
			DisableShapeCollision(shape);
		}

		if (shape->m_collisionType == 4) {
			DisableShapeCollision(shape);
		}
	}
}

// Draws the 3D view, the normal frame draw callback: clears the frame first when drawing to
// another pane, updates the eyepoint, draws the scene, then the objects of the shapes of kinds
// 0x90 and 0xa0 with their own clip distances, the scene's objects, and the animations.
void DrawScene(void)
{
	int32_t saved;

	if (g_framePane) {
		memset(g_mainPixelBuffer.m_buffer, g_backgroundColor, g_refreshModePixelCount);
		SelectPane(g_framePane);
	}

	if (g_projectionDirty) {
		UpdateProjection(g_eyepoint);
		g_projectionDirty = 0;
	}

	if (g_renderSettings.m_blankScene) {
		VFX_pane_wipe(&g_currentPane, g_backgroundColor);
		return;
	}

	UpdateViewMatrix(g_eyepoint);
	SelectEyepoint(g_eyepoint);
	if (g_renderSettings.m_clearFrame || g_renderSettings.m_wireframe) {
		VFX_pane_wipe(&g_currentPane, g_backgroundColor);
	}
	else if (g_renderSettings.m_drawSky || g_renderSettings.m_drawGround) {
		// On the DirectDraw back end the original left the ground out here: SimMain had filled
		// the frame with the ground color.
		DrawSkyAndGround(g_eyepoint);
	}

	if (g_hasLightObject && g_lightFollowsObject && g_lightObject != -1) {
		GetStaticObjectPosition(g_lightObject, &g_eyepoint->m_lightX, &g_eyepoint->m_lightY, &g_eyepoint->m_lightZ);
	}

	g_drawnPolygonCount = 0;
	if (g_skyObject) {
		saved = g_eyepoint->m_farPlane;
		SetFarPlane(g_eyepoint, 0x7fffffff);
		g_renderSettings.m_shapeFilter = CullShapeToFrustum;
		DrawObjTreeShapes(g_skyObject);
		g_drawnPolygonCount += g_depthEntryCount;
		SetFarPlane(g_eyepoint, saved);
		g_renderSettings.m_shapeFilter = CullSceneShape;
	}

	DrawShapeList(g_sceneShapes);
	g_drawnPolygonCount += g_depthEntryCount;
	if (g_inCockpitView && g_cockpitObject) {
		saved = g_eyepoint->m_nearPlane;
		SetNearPlane(g_eyepoint, 8);
		g_renderSettings.m_shapeFilter = CullHiddenShape;
		DrawObjTreeShapes(g_cockpitObject);
		g_drawnPolygonCount += g_depthEntryCount;
		SetNearPlane(g_eyepoint, saved);
		g_renderSettings.m_shapeFilter = CullSceneShape;
	}

	if (g_showBoundingSpheres) {
		DrawBoundingSpheres(g_sceneShapes);
	}

	SelectPane(0);
}

void SetFramePane(int32_t p_value)
{
	if (p_value >= 0 && p_value < 11) {
		g_framePane = p_value;
	}
	else {
		g_framePane = 0;
	}
}

void ResetPane(void)
{
	SelectPane(0);
}

// Presents the frame, or stretches the current pane over the window when a stretch is
// pending, restoring the pane afterwards.
void Blit(void)
{
	if (g_stretchPending) {
		g_currentRefreshMode
			->m_stretchBlit(g_currentPane.m_x0 + 1, g_currentPane.m_y0 + 1, g_currentPane.m_x1, g_currentPane.m_y1);
		g_currentPane = g_screenPane;
		g_showHud = g_savedShowHud;
		g_stretchPending = 0;
	}
	else {
		g_currentRefreshMode->m_flip();
	}
}

void ShutdownRender(void)
{
	FreeSceneShapes();
	ShutdownDrawBuffer();
	if (g_bannerBuffer && g_currentPane.m_window) {
		VFX_pane_wipe(&g_currentPane, 0);
		g_currentRefreshMode->m_flip();
	}

	if (g_bannerBuffer) {
		MechHeapFree(g_primaryHeap, g_bannerBuffer);
	}

	g_displayReady = 0;
	ShutdownRefreshMode();
}

// Draws the bounding spheres (the "michelin" cheat) of the list p_root's colliding mech shapes
// (kind 0x100 without flag 0x800) and of kind 0x50, as circles of their radius in color 0xf.
void DrawBoundingSpheres(Shape* p_root)
{
	int32_t color;
	Shape* shape;
	int32_t x;
	int32_t y;
	int32_t z;
	int32_t radius;
	int32_t radiusY;

	if (!p_root || !p_root->m_next || p_root->m_prev == p_root->m_next) {
		return;
	}

	for (shape = p_root->m_next; shape; shape = shape->m_next) {
		if ((((shape->m_kind & 0x100) && !(shape->m_flags & 0x800)) || (shape->m_kind & 0xf0) == 0x50) &&
			!g_renderSettings.m_shapeFilter(shape)) {
			x = shape->m_centerX;
			y = shape->m_centerY;
			z = shape->m_centerZ;
			radius = shape->m_radius;
			color = 0xf;
			if (ProjectWorldPoint(&x, &y, &z)) {
				radius = ProjectRadius(g_eyepoint->m_projectScaleX, radius, z);
				radiusY = FixedMul16(radius, g_eyepoint->m_pixelAspect);
				VFX_ellipse_draw(&g_currentPane, x, y, radius, radiusY, color);
			}
		}
	}
}
