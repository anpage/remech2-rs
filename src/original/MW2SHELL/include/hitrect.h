#ifndef HITRECT_H
#define HITRECT_H

#include "decomp.h"
#include "types.h"

class VideoDriver;

// SIZE 0x10
class HitRect {
public:
	HitRect(undefined4 p_left, undefined4 p_top, undefined4 p_right, undefined4 p_bottom);
	void FUN_10049183();
	void Draw(VideoDriver* p_videoDriver);
	MechU8 Contains(MechS32 p_x, MechS32 p_y);

private:
	MechS32 m_left;   // 0x00
	MechS32 m_top;    // 0x04
	MechS32 m_right;  // 0x08
	MechS32 m_bottom; // 0x0c
};

#endif // HITRECT_H
