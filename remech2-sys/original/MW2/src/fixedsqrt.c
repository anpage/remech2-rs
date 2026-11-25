#include "fixedsqrt.h"

#include "approxlen.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "sqrtguess.h"
#include "types.h"

#include <math.h>

// Returns the square root of a 16.16 fixed-point value: a binary search between half and twice
// FixedSqrtGuess's estimate, for at most 100 steps.
uint32_t FixedSqrt16(uint32_t p_value)
{
	uint32_t square;
	uint32_t high;
	int32_t steps = 100;
	uint32_t previous;
	uint32_t root;
	int32_t n;
	uint32_t low;

	root = FixedSqrtGuess(p_value);
	previous = p_value;
	low = root >> 1;
	high = root * 2;
	for (n = steps; n--;) {
		square = FixedMul16(root, root);
		if (p_value == square) {
			break;
		}

		if (p_value < square) {
			high = root;
		}
		else {
			low = root;
		}

		previous = root;
		root = (high + low + 1) >> 1;
		if (previous == root) {
			break;
		}
	}

	return root;
}

// Scales the vector to length 1.0 (16.16); a zero vector stays as it is.
void NormalizeVectorGuarded(int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	int32_t length;

	length = ApproximateVectorLength(*p_x, *p_y, *p_z);
	if (length > 0) {
		*p_x = FixedDiv16(*p_x, length);
		*p_y = FixedDiv16(*p_y, length);
		*p_z = FixedDiv16(*p_z, length);
	}
}

int32_t FloatVectorLength(int32_t p_x, int32_t p_y, int32_t p_z)
{
	double x = p_x;
	double y = p_y;
	double z = p_z;

	/* Only this grouping of the sum matches. */
	return (int32_t) (sqrt(z * z + (x * x + y * y)) + 0.5);
}
