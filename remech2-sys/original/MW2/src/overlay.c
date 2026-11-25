#include "overlay.h"

#include "clock.h"
#include "decomp.h"
#include "eyepoint.h"
#include "gamekeys.h"
#include "loadres.h"
#include "mw2prj.h"
#include "point.h"
#include "polydraw.h"
#include "render.h"
#include "screenscale.h"
#include "shape.h"
#include "shapelists.h"
#include "targeting.h"
#include "timedoverlays.h"
#include "types.h"

#include <stdio.h>
#include <string.h>

// The debug overlays: frame rate, scene counts, the eyepoint's position, memory and cache
// sizes, the palette and an activity spinner. Each has a flag the debug keys toggle
// (GAMEKEY.MAP's TOGGLE_FRAMERATE, TOGGLE_SCENE_INFO, TOGGLE_EYE_POSITION, TOGGLE_MEM_INFO,
// TOGGLE_PALETTE); the _MAIN keys also set a second flag that draws the text on the main
// screen, and a third remembers that the text is up, to blank it once the flag goes off.
// The rest went to a monochrome debug screen (80x25, as in the launcher's debug.c), which
// the release build only formats for: the drawing is compiled out.

// The spaces the Hide functions blank a readout with.
char g_blankText[] = "                                 ";

int32_t g_showPalette = 0;

// The row MonoPrint last wrote to; MonoClear resets it and its neighbours to the top row.
int32_t g_monoLastRow = 3;

int32_t g_frameRateShown = 0;

int32_t g_showFrameRate = 0;

int32_t g_showFrameRateMain = 0;

// Where the _MAIN readouts are drawn, in 16.16 fractions of the screen until
// ScaleOverlayPositions scales them to pixels.
Point g_frameRateOrigin = {0x51f, 0x147b};

Point g_memInfoOrigin = {0x51f, 0x2148};

Point g_eyePositionOrigin = {0xccd, 0xb333};

int32_t g_showSceneInfo = 0;

int32_t g_showSceneInfoMain = 0;

int32_t g_sceneInfoShown = 0;

int32_t g_showEyePosition = 0;

int32_t g_showEyePositionMain = 0;

int32_t g_eyePositionShown = 0;

int32_t g_showMemInfo = 1;

int32_t g_showMemInfoMain = 0;

int32_t g_memInfoShown = 0;

int32_t g_showCacheInfo = 0;

int32_t g_cacheInfoShown = 0;

int32_t g_showSpinner = 1;

// The spinner StepSpinner steps through: the CP437 arrows up, left, down and right.
char g_spinnerArrows[4] = {0x1e, 0x11, 0x1f, 0x10};

// The frame rate, in whole frames per second and tenths, over the last ten frames.
int32_t g_frameRate = 0;

int32_t g_frameRateTenths = 0;

int32_t g_frameRateTime = 0;

int32_t g_frameRateFrames = 0;

int32_t g_spinnerDelay = 4;

int32_t g_spinnerArrow = 3;

// The cursor of MonoPrint.
int32_t g_monoRow = 3;

int32_t g_monoColumn = 0;

// The debug build drew each text it formatted while g_monoEnabled was set; the release
// build keeps only the test.
#define DRAW_DEBUG_TEXT()                                                                                              \
	do {                                                                                                               \
		if (g_monoEnabled) {                                                                                           \
		}                                                                                                              \
	} while (0)

// The totals CountSceneShape adds up for ShowSceneInfo.
int32_t g_sceneVertexCount;

int32_t g_sceneMemory;

int32_t g_sceneShapeCount;

int32_t g_sceneFaceCount;

// Set when the monochrome debug screen is in use; the AI logs its state to it too. Only the
// /M switch's InitializeMono could have set it, in a branch compiled out.
int32_t g_monoEnabled;

// A line of spaces, with its terminator.
char g_monoBlankLine[0x50];

char g_monoBlankLineEnd;

// Draws each debug overlay that is on, or blanks it once it goes off; called every frame.
void DrawDebugOverlays(void)
{
	if (g_showPalette) {
		DrawPaletteGrid();
	}
	if (g_showSpinner) {
		StepSpinner();
	}

	if (g_showFrameRate) {
		ShowFrameRate();
	}
	else if (g_frameRateShown) {
		HideFrameRate();
	}

	if (g_showSceneInfo) {
		ShowSceneInfo();
	}
	else if (g_sceneInfoShown) {
		HideSceneInfo();
	}

	if (g_showEyePosition) {
		ShowEyePosition(g_eyepoint);
	}
	else if (g_eyePositionShown) {
		HideEyePosition();
	}

	if (g_showMemInfo) {
		ShowMemInfo();
	}
	else if (g_memInfoShown) {
		HideMemInfo();
	}

	if (g_showCacheInfo) {
		ShowCacheInfo();
	}
	else if (g_cacheInfoShown) {
		HideCacheInfo();
	}
}

// Draws the 256 palette colours as a grid of 16 swatches per row.
void DrawPaletteGrid(void)
{
	int32_t x;
	int32_t y;
	int32_t color;
	int32_t i;

	x = 10;
	y = 10;
	for (color = 0; color < 0x100; color++) {
		for (i = 0; i < 2; i++) {
			VFX_line_draw(&g_currentPane, x, i + y, x + 3, i + y, 0, color);
		}

		if ((color + 1) % 16 == 0) {
			x = 10;
			y += 2;
		}
		else {
			x += 4;
		}
	}
}

// Fills g_monoBlankLine (the /M switch's call; the debug build also turned the screen on and
// stopped the mission timer here).
void InitializeMono(void)
{
	int32_t i;

	if (FALSE) {
		g_monoEnabled = TRUE;
		g_missionTimerStopped = TRUE;
	}

	for (i = 0; i < 0x50; i++) {
		g_monoBlankLine[i] = ' ';
	}
	g_monoBlankLineEnd = '\0';
}

void ShowFrameRate(void)
{
	char text[64];
	int32_t rate;

	if (!g_monoEnabled && !g_showFrameRateMain) {
		return;
	}

	g_frameRateFrames++;
	g_frameRateTime += g_deltaTime;
	if (g_frameRateFrames >= 10) {
		if (g_frameRateTime > 0) {
			rate = (g_frameRateFrames * 0x712) / g_frameRateTime;
			g_frameRate = rate / 10;
			g_frameRateTenths = rate - g_frameRate * 10;
			sprintf(text, "Framerate %2.2d.%1.1d", g_frameRate, g_frameRateTenths);
			DRAW_DEBUG_TEXT();
		}

		g_frameRateFrames = 0;
		g_frameRateTime = 0;
	}

	if (g_showFrameRateMain && g_frameRate > 0) {
		sprintf(text, "%d.%d", g_frameRate, g_frameRateTenths);
		DrawTextBox(0x4f, 1, text, g_frameRateOrigin.m_x, g_frameRateOrigin.m_y);
	}

	g_frameRateShown = 1;
}

void HideFrameRate(void)
{
	char text[16];
	char format[16];

	sprintf(format, "%%%d.%ds", 0xe, 0xe);
	sprintf(text, format, g_blankText);
	DRAW_DEBUG_TEXT();
	g_frameRateShown = 0;
}

void ShowSceneInfo(void)
{
	char text[80];

	if (!g_monoEnabled && !g_showSceneInfoMain) {
		return;
	}

	g_sceneShapeCount = g_sceneVertexCount = g_sceneFaceCount = g_sceneMemory = 0;
	ForEachShape(g_sceneShapes, CountSceneShape);
	ForEachShape(g_hiddenShapes, CountSceneShape);

	if (g_monoEnabled) {
		if (!g_sceneInfoShown) {
			if (g_monoEnabled) {
				DRAW_DEBUG_TEXT();
			}

			if (g_showSceneInfoMain) {
				sprintf(
					text,
					"Objects: %d  Polygons: %d  Vertices: %d Curpolys: %d",
					g_sceneShapeCount,
					g_sceneFaceCount,
					g_sceneVertexCount,
					g_drawnPolygonCount
				);
				ShowInGameMessage(text, 1, 0xb5, 0x50);
			}

			g_sceneInfoShown = 1;
		}

		if (!g_monoEnabled) {
			return;
		}

		sprintf(text, "%4.4d", g_sceneShapeCount);
		DRAW_DEBUG_TEXT();
		sprintf(text, "%4.4d", g_sceneVertexCount);
		DRAW_DEBUG_TEXT();
		sprintf(text, "%4.4d", g_sceneFaceCount);
		DRAW_DEBUG_TEXT();
		sprintf(text, "%4.4d", g_sceneMemory);
		DRAW_DEBUG_TEXT();
	}
}

void HideSceneInfo(void)
{
	char text[32];
	char format[32];

	sprintf(format, "%%%d.%ds", 0x1e, 0x1e);
	sprintf(text, format, g_blankText);
	DRAW_DEBUG_TEXT();
	DRAW_DEBUG_TEXT();
	g_sceneInfoShown = 0;
}

// Adds a shape's counts to the totals ShowSceneInfo shows.
void CountSceneShape(Shape* p_shape)
{
	int32_t vertexCount;
	int32_t faceCount;

	vertexCount = 0;
	faceCount = 0;
	GetModelCounts(p_shape, &vertexCount, &faceCount);

	g_sceneShapeCount++;
	g_sceneVertexCount += vertexCount;
	g_sceneFaceCount += faceCount;
	g_sceneMemory += GetShapeMemorySize(p_shape);
}

void ShowEyePosition(Eyepoint* p_eyepoint)
{
	char text[132];

	if (!g_showEyePositionMain && !g_monoEnabled) {
		return;
	}

	sprintf(text, "txyz: %4.4d %4.4d %4.4d", p_eyepoint->m_x, p_eyepoint->m_y, p_eyepoint->m_z);
	DRAW_DEBUG_TEXT();
	sprintf(
		text,
		"rxyz:  %4.4d %4.4d %4.4d",
		(p_eyepoint->m_pitch >> 16) % 360,
		(p_eyepoint->m_heading >> 16) % 360,
		(p_eyepoint->m_roll >> 16) % 360
	);
	DRAW_DEBUG_TEXT();

	if (g_showEyePositionMain) {
		sprintf(
			text,
			"txyz: %5d %5d %5d\nrxyz: %5d %5d %5d ",
			p_eyepoint->m_x,
			p_eyepoint->m_y,
			p_eyepoint->m_z,
			(p_eyepoint->m_pitch >> 16) % 360,
			(p_eyepoint->m_heading >> 16) % 360,
			(p_eyepoint->m_roll >> 16) % 360
		);
		DrawTextBox(0x4f, 1, text, g_eyePositionOrigin.m_x, g_eyePositionOrigin.m_y);
	}

	g_eyePositionShown = 1;
}

void HideEyePosition(void)
{
	char text[28];
	char format[28];

	sprintf(format, "%%%d.%ds", 0x18, 0x18);
	sprintf(text, format, g_blankText);
	DRAW_DEBUG_TEXT();
	DRAW_DEBUG_TEXT();
	g_eyePositionShown = 0;
}

void ShowCacheInfo(void)
{
	char text[28];

	sprintf(text, "Items in cache: %i     ", g_cacheEntryCount);
	DRAW_DEBUG_TEXT();
	g_cacheInfoShown = 1;
}

void HideCacheInfo(void)
{
	char text[28];

	// The original also passed 0x19 twice, which the format doesn't use
	sprintf(text, "                           ");
	DRAW_DEBUG_TEXT();
	g_cacheInfoShown = 0;
}

void ShowMemInfo(void)
{
	char text[40];
	int32_t value;

	value = 0;
	if (!g_memInfoShown) {
		DRAW_DEBUG_TEXT();
		g_memInfoShown = 1;
	}

	sprintf(text, "%8.8d", value);
	DRAW_DEBUG_TEXT();
	if (g_showMemInfoMain) {
		DrawTextBox(0x4f, 1, text, g_memInfoOrigin.m_x, g_memInfoOrigin.m_y);
	}
}

void HideMemInfo(void)
{
	char text[16];
	char format[16];

	sprintf(format, "%%%d.%ds", 0xc, 0xc);
	sprintf(text, format, g_blankText);
	DRAW_DEBUG_TEXT();
	g_memInfoShown = 0;
}

// Steps the activity spinner.
void StepSpinner(void)
{
	char text[2];

	if (--g_spinnerDelay == 0) {
		g_spinnerDelay = 4;
		text[0] = g_spinnerArrows[g_spinnerArrow];
		text[1] = '\0';
		DRAW_DEBUG_TEXT();

		if (g_spinnerArrow-- == 0) {
			g_spinnerArrow = 3;
		}
	}
}

// Formats a line for the debug console: a leading space, padded with spaces to 79 characters.
void MonoPrintLine(char* p_text)
{
	int32_t i;
	char text[80];
	char* dst;

	for (i = 0; i < 80; i++) {
		text[i] = '\0';
	}

	text[0] = ' ';
	dst = text + 1;
	strncpy(dst, p_text, 78);
	for (i = 0; i < 79; i++) {
		if (text[i] == '\0') {
			text[i] = ' ';
		}
	}

	DRAW_DEBUG_TEXT();
}

// Clears the debug console's rows 3 to 23.
void MonoClear(void)
{
	int32_t i;

	for (i = 3; i <= 0x17; i++) {
		DRAW_DEBUG_TEXT();
	}

	g_monoLastRow = 3;
}

// Writes text to the debug console, 80 columns and rows 3 to 23, clearing it when it fills.
void MonoPrint(char* p_text)
{
	int32_t newline;
	char ch[2];
	int32_t i;
	int32_t length;

	newline = FALSE;
	length = strlen(p_text);
	ch[1] = '\0';
	for (i = 0; i < length; i++) {
		if (p_text[i] == '\n') {
			newline = TRUE;
		}
		else {
			ch[0] = p_text[i];
			DRAW_DEBUG_TEXT();
		}

		g_monoColumn++;
		if (g_monoColumn > 0x4f || newline) {
			g_monoColumn = 0;
			g_monoRow++;
			if (g_monoRow > 0x17) {
				MonoClear();
				g_monoRow = 3;
			}

			newline = FALSE;
		}
	}

	g_monoLastRow = g_monoRow;
}

// Scales the _MAIN readouts' positions to the screen, after a change of resolution.
void ScaleOverlayPositions(void)
{
	ScalePointToScreen(&g_mainPixelBuffer, &g_frameRateOrigin, &g_frameRateOrigin);
	ScalePointToScreen(&g_mainPixelBuffer, &g_eyePositionOrigin, &g_eyePositionOrigin);
	ScalePointToScreen(&g_mainPixelBuffer, &g_memInfoOrigin, &g_memInfoOrigin);
}

void SimEntranceDbug(char* p_mission, int32_t p_memory)
{
	char text[80];

	sprintf(
		text,
		"%s  Res:%dx%d  Mem:%d",
		p_mission,
		g_gameWindowGeometry->m_width,
		g_gameWindowGeometry->m_height,
		p_memory
	);
	ShowInGameMessage(text, 0, 0x4f3, 0x50);

	if (!g_monoEnabled) {
		return;
	}

	sprintf(text, "WarThink / MechWarrior II %s %d", p_mission, p_memory);
	MonoPrintLine(text);
}
