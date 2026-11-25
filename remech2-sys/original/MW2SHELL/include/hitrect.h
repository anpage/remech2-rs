#ifndef HITRECT_H
#define HITRECT_H

#include "decomp.h"
#include "types.h"

class VideoDriver;

class HitRect {
public:
	HitRect(undefined4 p_left, undefined4 p_top, undefined4 p_right, undefined4 p_bottom);
	void Draw(VideoDriver* p_videoDriver);
	uint8_t Contains(int32_t p_x, int32_t p_y);

private:
	int32_t m_left;   // 0x00
	int32_t m_top;    // 0x04
	int32_t m_right;  // 0x08
	int32_t m_bottom; // 0x0c
};

#endif // HITRECT_H
