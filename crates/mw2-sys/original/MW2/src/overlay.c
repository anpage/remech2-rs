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
// GLOBAL: MW2 0x100a9478
MechChar g_blankText[] = "                                 ";

// GLOBAL: MW2 0x100a949c
MechS32 g_showPalette = 0;

// GLOBAL: MW2 0x100a94a0
MechS32 g_unk0x100a94a0 = 0;

// The row MonoPrint last wrote to; MonoClear resets it and its neighbours to the top row.
// GLOBAL: MW2 0x100a94a4
MechS32 g_monoLastRow = 3;

// GLOBAL: MW2 0x100a94a8
MechS32 g_unk0x100a94a8 = 3;

// GLOBAL: MW2 0x100a94ac
MechChar g_unk0x100a94ac[4] = "";

// GLOBAL: MW2 0x100a94b0
MechS32 g_frameRateShown = 0;

// GLOBAL: MW2 0x100a94b4
MechS32 g_showFrameRate = 0;

// GLOBAL: MW2 0x100a94b8
MechS32 g_showFrameRateMain = 0;

// GLOBAL: MW2 0x100a94bc
MechS32 g_unk0x100a94bc = 0;

// Where the _MAIN readouts are drawn, in 16.16 fractions of the screen until
// ScaleOverlayPositions scales them to pixels.
// GLOBAL: MW2 0x100a94c0
Point g_frameRateOrigin = {0x51f, 0x147b};

// GLOBAL: MW2 0x100a94c8
Point g_memInfoOrigin = {0x51f, 0x2148};

// GLOBAL: MW2 0x100a94d0
Point g_eyePositionOrigin = {0xccd, 0xb333};

// GLOBAL: MW2 0x100a94d8
MechS32 g_showSceneInfo = 0;

// GLOBAL: MW2 0x100a94dc
MechS32 g_showSceneInfoMain = 0;

// GLOBAL: MW2 0x100a94e0
MechS32 g_sceneInfoShown = 0;

// GLOBAL: MW2 0x100a94e4
MechS32 g_showEyePosition = 0;

// GLOBAL: MW2 0x100a94e8
MechS32 g_showEyePositionMain = 0;

// GLOBAL: MW2 0x100a94ec
MechS32 g_eyePositionShown = 0;

// GLOBAL: MW2 0x100a94f0
MechS32 g_showMemInfo = 1;

// GLOBAL: MW2 0x100a94f4
MechS32 g_showMemInfoMain = 0;

// GLOBAL: MW2 0x100a94f8
MechS32 g_memInfoShown = 0;

// GLOBAL: MW2 0x100a94fc
MechS32 g_showCacheInfo = 0;

// GLOBAL: MW2 0x100a9500
MechS32 g_cacheInfoShown = 0;

// GLOBAL: MW2 0x100a9504
MechS32 g_showSpinner = 1;

// The spinner StepSpinner steps through: the CP437 arrows up, left, down and right.
// GLOBAL: MW2 0x100a9508
MechChar g_spinnerArrows[4] = {0x1e, 0x11, 0x1f, 0x10};

// A repeated cache dump (DUMP_CACHE_REPEAT, whose key does nothing in this build) every
// second.
// GLOBAL: MW2 0x100a950c
MechS32 g_nextCacheDump = 0;

// GLOBAL: MW2 0x100a9510
MechS32 g_dumpCacheRepeat = 0;

// GLOBAL: MW2 0x100a9514
MechS32 g_unk0x100a9514 = 0;

// GLOBAL: MW2 0x100a9518
MechS32 g_unk0x100a9518 = 0;

// The frame rate, in whole frames per second and tenths, over the last ten frames.
// GLOBAL: MW2 0x100a951c
MechS32 g_frameRate = 0;

// GLOBAL: MW2 0x100a9520
MechS32 g_frameRateTenths = 0;

// GLOBAL: MW2 0x100a9524
MechS32 g_frameRateTime = 0;

// GLOBAL: MW2 0x100a9528
MechS32 g_frameRateFrames = 0;

// GLOBAL: MW2 0x100a952c
MechS32 g_spinnerDelay = 4;

// GLOBAL: MW2 0x100a9530
MechS32 g_spinnerArrow = 3;

// The cursor of MonoPrint.
// GLOBAL: MW2 0x100a9534
MechS32 g_monoRow = 3;

// GLOBAL: MW2 0x100a9538
MechS32 g_monoColumn = 0;

// The debug build drew each text it formatted while g_monoEnabled was set; the release
// build keeps only the test.
#define DRAW_DEBUG_TEXT()                                                                                              \
	do {                                                                                                               \
		if (g_monoEnabled) {                                                                                           \
		}                                                                                                              \
	} while (0)

// The totals CountSceneShape adds up for ShowSceneInfo.
// GLOBAL: MW2 0x100bea00
MechS32 g_sceneVertexCount;

// GLOBAL: MW2 0x100bea04
MechS32 g_sceneMemory;

// GLOBAL: MW2 0x100bea08
MechS32 g_sceneShapeCount;

// GLOBAL: MW2 0x100bea0c
MechS32 g_sceneFaceCount;

// Set when the monochrome debug screen is in use; the AI logs its state to it too. Only the
// /M switch's InitializeMono could have set it, in a branch compiled out.
// GLOBAL: MW2 0x100e9630
MechS32 g_monoEnabled;

// A line of spaces, with its terminator.
// GLOBAL: MW2 0x100e9640
MechChar g_monoBlankLine[0x50];

// GLOBAL: MW2 0x100e9690
MechChar g_monoBlankLineEnd;

// Draws each debug overlay that is on, or blanks it once it goes off; called every frame.
// FUNCTION: MW2 0x10058750
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

	if (g_unk0x100a9518) {
		g_unk0x100a9514 = 1;
	}
	if (g_dumpCacheRepeat && g_nextCacheDump < g_currentClock) {
		g_nextCacheDump = g_currentClock + 0xb5;
		FUN_1001a521(g_mw2PrjHandle);
	}
}

// Draws the 256 palette colours as a grid of 16 swatches per row.
// Stack-slot permutation: x, y, color and i.
// FUNCTION: MW2 0x100588a7
void DrawPaletteGrid(void)
{
	MechS32 x;
	MechS32 y;
	MechS32 color;
	MechS32 i;

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
// FUNCTION: MW2 0x10058958
void InitializeMono(void)
{
	MechS32 i;

	if (FALSE) {
		g_monoEnabled = TRUE;
		g_missionTimerStopped = TRUE;
	}

	for (i = 0; i < 0x50; i++) {
		g_monoBlankLine[i] = ' ';
	}
	g_monoBlankLineEnd = '\0';
}

// Stack-slot permutation: text and rate.
// FUNCTION: MW2 0x100589ae
void ShowFrameRate(void)
{
	MechChar text[64];
	MechS32 rate;

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
			sprintf(text, "Framerate %2.2ld.%1.1ld", g_frameRate, g_frameRateTenths);
			DRAW_DEBUG_TEXT();
		}

		g_frameRateFrames = 0;
		g_frameRateTime = 0;
	}

	if (g_showFrameRateMain && g_frameRate > 0) {
		sprintf(text, "%ld.%ld", g_frameRate, g_frameRateTenths);
		DrawTextBox(0x4f, 1, text, g_frameRateOrigin.m_x, g_frameRateOrigin.m_y);
	}

	g_frameRateShown = 1;
}

// FUNCTION: MW2 0x10058ae5
void HideFrameRate(void)
{
	MechChar text[16];
	MechChar format[16];

	sprintf(format, "%%%d.%ds", 0xe, 0xe);
	sprintf(text, format, g_blankText);
	DRAW_DEBUG_TEXT();
	g_frameRateShown = 0;
}

// FUNCTION: MW2 0x10058b34
void ShowSceneInfo(void)
{
	MechChar text[80];

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

// FUNCTION: MW2 0x10058cda
void HideSceneInfo(void)
{
	MechChar text[32];
	MechChar format[32];

	sprintf(format, "%%%d.%ds", 0x1e, 0x1e);
	sprintf(text, format, g_blankText);
	DRAW_DEBUG_TEXT();
	DRAW_DEBUG_TEXT();
	g_sceneInfoShown = 0;
}

// Adds a shape's counts to the totals ShowSceneInfo shows.
// Stack-slot permutation: vertexCount and faceCount.
// FUNCTION: MW2 0x10058d36
void CountSceneShape(Shape* p_shape)
{
	MechS32 vertexCount;
	MechS32 faceCount;

	vertexCount = 0;
	faceCount = 0;
	GetModelCounts(p_shape, &vertexCount, &faceCount);

	g_sceneShapeCount++;
	g_sceneVertexCount += vertexCount;
	g_sceneFaceCount += faceCount;
	g_sceneMemory += GetShapeMemorySize(p_shape);
}

// FUNCTION: MW2 0x10058d90
void ShowEyePosition(Eyepoint* p_eyepoint)
{
	MechChar text[132];

	if (!g_showEyePositionMain && !g_monoEnabled) {
		return;
	}

	sprintf(text, "txyz: %04.4ld %04.4ld %04.4ld", p_eyepoint->m_x, p_eyepoint->m_y, p_eyepoint->m_z);
	DRAW_DEBUG_TEXT();
	sprintf(
		text,
		"rxyz:  %04.4ld %04.4ld %04.4ld",
		(p_eyepoint->m_pitch >> 16) % 360,
		(p_eyepoint->m_heading >> 16) % 360,
		(p_eyepoint->m_roll >> 16) % 360
	);
	DRAW_DEBUG_TEXT();

	if (g_showEyePositionMain) {
		sprintf(
			text,
			"txyz: %5ld %5ld %5ld\nrxyz: %5d %5d %5d ",
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

// FUNCTION: MW2 0x10058ee0
void HideEyePosition(void)
{
	MechChar text[28];
	MechChar format[28];

	sprintf(format, "%%%d.%ds", 0x18, 0x18);
	sprintf(text, format, g_blankText);
	DRAW_DEBUG_TEXT();
	DRAW_DEBUG_TEXT();
	g_eyePositionShown = 0;
}

// FUNCTION: MW2 0x10058f3c
void ShowCacheInfo(void)
{
	MechChar text[28];

	sprintf(text, "Items in cache: %li     ", g_cacheEntryCount);
	DRAW_DEBUG_TEXT();
	g_cacheInfoShown = 1;
}

// FUNCTION: MW2 0x10058f78
void HideCacheInfo(void)
{
	MechChar text[28];

	sprintf(text, "                           ", 0x19, 0x19);
	DRAW_DEBUG_TEXT();
	g_cacheInfoShown = 0;
}

// Stack-slot permutation: text and value.
// FUNCTION: MW2 0x10058fb2
void ShowMemInfo(void)
{
	MechChar text[40];
	MechS32 value;

	value = 0;
	if (!g_memInfoShown) {
		DRAW_DEBUG_TEXT();
		g_memInfoShown = 1;
	}

	sprintf(text, "%8.8ld", value);
	DRAW_DEBUG_TEXT();
	if (g_showMemInfoMain) {
		DrawTextBox(0x4f, 1, text, g_memInfoOrigin.m_x, g_memInfoOrigin.m_y);
	}
}

// FUNCTION: MW2 0x10059036
void HideMemInfo(void)
{
	MechChar text[16];
	MechChar format[16];

	sprintf(format, "%%%d.%ds", 0xc, 0xc);
	sprintf(text, format, g_blankText);
	DRAW_DEBUG_TEXT();
	g_memInfoShown = 0;
}

// Steps the activity spinner.
// FUNCTION: MW2 0x10059085
void StepSpinner(void)
{
	MechChar text[2];

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
// Stack-slot permutation: i, text and dst.
// FUNCTION: MW2 0x100590ea
void MonoPrintLine(MechChar* p_text)
{
	MechS32 i;
	MechChar text[80];
	MechChar* dst;

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
// FUNCTION: MW2 0x1005917d
void MonoClear(void)
{
	MechS32 i;

	for (i = 3; i <= 0x17; i++) {
		DRAW_DEBUG_TEXT();
	}

	g_unk0x100a94ac[0] = '\0';
	g_unk0x100a94a8 = 3;
	g_monoLastRow = 3;
}

// Writes text to the debug console, 80 columns and rows 3 to 23, clearing it when it fills.
// Stack-slot permutation: newline, ch and length.
// FUNCTION: MW2 0x100591d1
void MonoPrint(MechChar* p_text)
{
	MechS32 newline;
	MechChar ch[2];
	MechS32 i;
	MechS32 length;

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
// FUNCTION: MW2 0x100592b0
void ScaleOverlayPositions(void)
{
	ScalePointToScreen(&g_mainPixelBuffer, &g_frameRateOrigin, &g_frameRateOrigin);
	ScalePointToScreen(&g_mainPixelBuffer, &g_eyePositionOrigin, &g_eyePositionOrigin);
	ScalePointToScreen(&g_mainPixelBuffer, &g_memInfoOrigin, &g_memInfoOrigin);
}

// FUNCTION: MW2 0x10059300
void SimEntranceDbug(MechChar* p_mission, MechS32 p_memory)
{
	MechChar text[80];

	sprintf(
		text,
		"%s  Res:%dx%d  Mem:%ld",
		p_mission,
		g_gameWindowGeometry->m_width,
		g_gameWindowGeometry->m_height,
		p_memory
	);
	ShowInGameMessage(text, 0, 0x4f3, 0x50);

	if (!g_monoEnabled) {
		return;
	}

	sprintf(text, "WarThink / MechWarrior II %s %ld", p_mission, p_memory);
	MonoPrintLine(text);
}
