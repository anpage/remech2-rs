#ifndef GIFSAVE_H
#define GIFSAVE_H

#include "types.h"

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
