#ifndef PORTABLE_H
#define PORTABLE_H

#include "types.h"

// Types and helpers for the portable C that replaces hand-written assembly (PORTABLE_C in
// compat.h). The replacements are standard C with defined behaviour; the register-level
// operations whose C spelling would be implementation-defined are written here once, in defined
// terms.

// 64-bit integers (imul products, edx:eax dividends). VC++ 4.1 has no long long: its type is
// __int64, and constants are written as casts ((MechS64) 1 << 40). They live here rather than in
// types.h: every typedef there is a symbol in every unit, and 4.1's stack-slot assignment and
// operand order react to the symbol count.
#if defined(_MSC_VER) && _MSC_VER < 1200
typedef __int64 MechS64;
typedef unsigned __int64 MechU64;
#else
#include <stdint.h>
typedef int64_t MechS64;
typedef uint64_t MechU64;
#endif

#if defined(_MSC_VER) || !defined(__STDC_VERSION__)
#define PORTABLE_INLINE static __inline
#else
#define PORTABLE_INLINE static inline
#endif

// Preconditions. Inputs where the original faults (an idiv by zero or overflowing), or does what
// the game never asks of it, are outside a routine's domain: the portable C asserts them, so that
// a call that breaks one is caught in the tests and debug builds (PORTABLE_ASSERTS, _DEBUG).
// Elsewhere the assertions compile to nothing, and the C may do anything there.
#if defined(PORTABLE_C) && (defined(PORTABLE_ASSERTS) || defined(_DEBUG))
#include <stdio.h>
#include <stdlib.h>

PORTABLE_INLINE void PortableAssertFailed(const char* p_condition, const char* p_file, int p_line)
{
	fprintf(stderr, "%s(%d): precondition failed: %s\n", p_file, p_line, p_condition);
	abort();
}

#define PORTABLE_ASSERT(p_condition) ((p_condition) ? (void) 0 : PortableAssertFailed(#p_condition, __FILE__, __LINE__))
#else
#define PORTABLE_ASSERT(p_condition) ((void) 0)
#endif

// Reads a 32-bit register's bits as a signed value (two's complement). A plain cast of a value
// above 0x7fffffff is implementation-defined before C23; compilers fold this to nothing.
PORTABLE_INLINE MechS32 PortableS32(MechU32 p_value)
{
	if (p_value <= 0x7fffffff) {
		return (MechS32) p_value;
	}

	return -(MechS32) ~p_value - 1;
}

// Reads a 64-bit register pair's bits (edx:eax, a sum that wraps) as a signed value.
PORTABLE_INLINE MechS64 PortableS64(MechU64 p_value)
{
	if (p_value <= (((MechU64) 0x7fffffff << 32) | 0xffffffff)) {
		return (MechS64) p_value;
	}

	return -(MechS64) ~p_value - 1;
}

// Reads a 16-bit register's bits as a signed value.
PORTABLE_INLINE MechS16 PortableS16(MechU16 p_value)
{
	if (p_value <= 0x7fff) {
		return (MechS16) p_value;
	}

	return (MechS16) (-(MechS32) (MechU16) ~p_value - 1);
}

// `sar` of a 32-bit register (0 <= n <= 31): an arithmetic right shift, rounding towards minus
// infinity. C leaves the right shift of a negative value implementation-defined.
PORTABLE_INLINE MechS32 PortableSar32(MechS32 p_value, MechS32 p_shift)
{
	if (p_value >= 0) {
		return p_value >> p_shift;
	}

	return -1 - ((-1 - p_value) >> p_shift);
}

// The same for a 64-bit value in edx:eax (`shrd eax, edx, n; sar edx, n`; 0 <= n <= 63).
PORTABLE_INLINE MechS64 PortableSar64(MechS64 p_value, MechS32 p_shift)
{
	if (p_value >= 0) {
		return p_value >> p_shift;
	}

	return -1 - ((-1 - p_value) >> p_shift);
}

// `bsr`: the index of the highest set bit. p_value must not be 0 (bsr leaves its destination
// undefined then).
PORTABLE_INLINE MechS32 PortableBsr(MechU32 p_value)
{
	MechS32 bit = 31;

	while (!(p_value & 0x80000000)) {
		p_value <<= 1;
		bit--;
	}

	return bit;
}

// `idiv` of edx:eax: the quotient, truncated towards zero. Out of domain (the idiv faults): a
// divisor of 0, or a quotient outside MechS32.
PORTABLE_INLINE MechS32 PortableIdiv(MechS64 p_dividend, MechS32 p_divisor)
{
	MechS64 quotient;

	PORTABLE_ASSERT(p_divisor != 0);
	quotient = p_dividend / p_divisor;
	PORTABLE_ASSERT(quotient >= -0x7fffffff - 1 && quotient <= 0x7fffffff);
	return (MechS32) quotient;
}

// `div` of edx:eax: the same, unsigned. Out of domain: a divisor of 0, or a quotient of more than
// 32 bits.
PORTABLE_INLINE MechU32 PortableDiv(MechU64 p_dividend, MechU32 p_divisor)
{
	MechU64 quotient;

	PORTABLE_ASSERT(p_divisor != 0);
	quotient = p_dividend / p_divisor;
	PORTABLE_ASSERT(quotient <= 0xffffffff);
	return (MechU32) quotient;
}

// `shrd lo, hi, n; adc lo, 0` on the 64-bit value hi:lo (1 <= n <= 31): the low 32 bits of the
// value shifted right by n, rounded up by the last bit shifted out (wrapping like the adc).
PORTABLE_INLINE MechU32 PortableShrdRound(MechU64 p_value, MechS32 p_shift)
{
	return (MechU32) (p_value >> p_shift) + ((MechU32) (p_value >> (p_shift - 1)) & 1);
}

#endif // PORTABLE_H
