#ifndef HITRECT_H
#define HITRECT_H

#include "types.h"

class VideoDriver;

class HitRect {
public:
	HitRect(uint32_t p_left, uint32_t p_top, uint32_t p_right, uint32_t p_bottom);
	void Draw(VideoDriver* p_videoDriver);
	uint8_t Contains(int32_t p_x, int32_t p_y);

private:
	int32_t m_left;   // 0x00
	int32_t m_top;    // 0x04
	int32_t m_right;  // 0x08
	int32_t m_bottom; // 0x0c
};

#endif // HITRECT_H
