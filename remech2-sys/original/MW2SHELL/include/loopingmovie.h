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
	Smack* m_smack;
	int32_t m_left;
	int32_t m_top;
	int32_t m_width;
	int32_t m_height;
	int32_t m_frame;
};

#endif // LOOPINGMOVIE_H
