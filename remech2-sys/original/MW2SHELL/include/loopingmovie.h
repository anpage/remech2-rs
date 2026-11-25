#ifndef LOOPINGMOVIE_H
#define LOOPINGMOVIE_H

#include "smackw32.h"
#include "types.h"

// A looping Smacker movie drawn into a rectangle of the shell (the "amwlogo1" logo on the
// options, leaderboard and credits screens).
class LoopingMovie {
public:
	LoopingMovie(char* p_name, int32_t p_left, int32_t p_top);
	~LoopingMovie();

	void MoveTo(int32_t p_left, int32_t p_top);

	void Update();

private:
	Smack* m_smack;   // 0x00
	int32_t m_left;   // 0x04
	int32_t m_top;    // 0x08
	int32_t m_width;  // 0x0c
	int32_t m_height; // 0x10
	int32_t m_frame;  // 0x14
};

#endif // LOOPINGMOVIE_H
