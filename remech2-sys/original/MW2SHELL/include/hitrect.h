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
	int32_t m_left;
	int32_t m_top;
	int32_t m_right;
	int32_t m_bottom;
};

#endif // HITRECT_H
