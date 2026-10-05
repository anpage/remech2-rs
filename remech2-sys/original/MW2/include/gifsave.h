#ifndef GIFSAVE_H
#define GIFSAVE_H

#include "types.h"

#include <stdio.h>

// GIFSAVE's result codes.
enum {
	c_gifOk = 0,
	c_gifErrCreate = 1,
	c_gifErrWrite = 2,
	c_gifOutOfMemory = 3
};

// The functions of gifsave.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern FILE* g_outFile;
	extern MechU8 g_gifBuffer[256];
	extern MechS32 g_gifIndex;
	extern MechS32 g_bitsLeft;
	extern MechU8* g_strChr;
	extern MechU32* g_strNxt;
	extern MechU32* g_strHsh;
	extern MechU32 g_numStrings;
	extern MechS32 g_bitsPrPrimColor;
	extern MechS32 g_numColors;
	extern MechU8* g_colorTable;
	extern MechU32 g_gifScreenHeight;
	extern MechU32 g_gifScreenWidth;
	extern MechU32 g_imageHeight;
	extern MechU32 g_imageWidth;
	extern MechU32 g_imageLeft;
	extern MechU32 g_imageTop;
	extern MechU32 g_relPixX;
	extern MechU32 g_relPixY;
	extern MechS32 (*g_getPixel)(MechS32 p_x, MechS32 p_y);

	MechS32 GifCreate(
		const MechChar* p_filename,
		MechU32 p_width,
		MechU32 p_height,
		MechU32 p_numColors,
		MechS32 p_colorRes
	);
	void GifSetColor(MechS32 p_colorNum, MechS32 p_red, MechS32 p_green, MechS32 p_blue);
	MechS32 GifCompressImage(
		MechS32 p_left,
		MechS32 p_top,
		MechS32 p_width,
		MechS32 p_height,
		MechS32 (*p_getPixel)(MechS32 p_x, MechS32 p_y)
	);
	MechS32 GifClose(void);

#ifdef __cplusplus
}
#endif

#endif // GIFSAVE_H
