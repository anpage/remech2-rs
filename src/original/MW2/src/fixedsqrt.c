#include "fixedsqrt.h"

#include "approxlen.h"
#include "decomp.h"
#include "fixeddiv.h"
#include "fixedmul.h"
#include "sqrtguess.h"
#include "types.h"

#include <math.h>

// Returns the square root of a 16.16 fixed-point value: a binary search between half and twice
// FixedSqrtGuess's estimate, for at most 100 steps.
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x100168f0
MechU32 FixedSqrt16(MechU32 p_value)
{
	MechU32 square;
	MechU32 high;
	MechS32 steps = 100;
	MechU32 previous;
	MechU32 root;
	MechS32 n;
	MechU32 low;

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
// FUNCTION: MW2 0x100169b4
void NormalizeVectorGuarded(MechS32* p_x, MechS32* p_y, MechS32* p_z)
{
	MechS32 length;

	length = ApproximateVectorLength(*p_x, *p_y, *p_z);
	if (length > 0) {
		*p_x = FixedDiv16(*p_x, length);
		*p_y = FixedDiv16(*p_y, length);
		*p_z = FixedDiv16(*p_z, length);
	}
}

// FUNCTION: MW2 0x10016a2e
MechS32 FloatVectorLength(MechS32 p_x, MechS32 p_y, MechS32 p_z)
{
	MechDouble x = p_x;
	MechDouble y = p_y;
	MechDouble z = p_z;

	/* Only this grouping of the sum matches. */
	return (MechS32) (sqrt(z * z + (x * x + y * y)) + 0.5);
}
