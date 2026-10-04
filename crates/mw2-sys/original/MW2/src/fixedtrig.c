/* Hand-written assembly: FixedSin (sine), FixedAsin (arcsine) and FixedAtan2
   (arctangent) are C functions with __asm bodies over clock.c's tables. Their portable C
   (PORTABLE_C) is tested against the assembly by tests/asmequiv. */
#include "fixedtrig.h"

#include "clock.h"
#include "compat.h"
#include "decomp.h"
#include "portable.h"
#include "types.h"

#pragma warning(disable : 4035) /* FixedAtan2 leaves its result in eax */

// Returns the sine of p_angle (16.16 degrees) from the table, as 16.16: the angle is scaled
// to 1024 steps to the circle (0x5b05b05b is 2^37 / 360), and the table's quarter wave is
// interpolated, mirrored and negated by quadrant.
// FUNCTION: MW2 0x100696c0
MechS32 FixedSin(MechS32 p_angle)
{
	MechS32 result;

#ifdef PORTABLE_C
	/* The angle in 1024ths of the circle (8.16 per quadrant): bit 24 selects the falling half
	   of a half wave, bit 25 the negative half wave. */
	MechU32 steps = PortableShrdRound((MechU64) ((MechS64) p_angle * 0x5b05b05b), 29);
	MechU32 index = (steps >> 16) & 0xff;
	MechU32 fraction = steps;
	MechU32 low;
	MechU32 sine;

	if (steps & 0x1000000) {
		fraction = ~steps;
		index ^= 0xff;
	}

	low = (MechU32) g_sinTable[index];
	sine = low + PortableShrdRound((MechU64) ((MechU32) g_sinTable[index + 1] - low) * (fraction & 0xffff), 16);
	if (steps & 0x2000000) {
		sine = 0 - sine;
	}

	result = PortableS32(sine);
#else
	__asm {
		mov eax, p_angle
		mov edx, 0x5b05b05b
		imul edx
		shrd eax, edx, 29
		adc eax, 0
		mov p_angle, eax
		mov ebx, eax
		mov esi, eax
		mov edx, eax
		shr ebx, 14
		and ebx, 0x3fc
		test esi, 0x1000000
		jz rising
		not edx
		xor ebx, 0x3fc
rising:
		mov ecx, g_sinTable[ebx]
		mov eax, g_sinTable[ebx + 4]
		sub eax, ecx
		and edx, 0xffff
		mul edx
		shrd eax, edx, 16
		adc ecx, eax
		test esi, 0x2000000
		jz positive
		neg ecx
positive:
		mov result, ecx
	}
#endif

	return result;
}

// Returns the cosine of p_angle (16.16 degrees): the sine 90 degrees on.
// FUNCTION: MW2 0x1006973a
MechS32 FixedCos(MechS32 p_angle)
{
#ifdef PORTABLE_C
	/* The add wraps (BuildMatrixEx's portable C calls this with any angle). */
	return FixedSin(PortableS32((MechU32) p_angle + 0x5a0000));
#else
	return FixedSin(p_angle + 0x5a0000);
#endif
}

// Returns the arcsine of p_sine (2.29 fixed point, as the table holds it) in 16.16 degrees: a
// binary search of the table's quarter wave, interpolated between the two entries found.
// Stack-slot permutation: result and table.
// FUNCTION: MW2 0x1006975b
MechS32 FixedAsin(MechS32 p_sine)
{
#ifdef PORTABLE_C
	/* The magnitude wraps like the C's negation (INT_MIN stays negative), and the search
	   compares unsigned. */
	MechU32 sine = (MechU32) p_sine;
	MechS32 negative = 0;
	MechS32 result;
	MechU32 index;
	MechU32 step;
	MechU32 position;
	MechU32 offset;
	MechU32 span;

	if (p_sine == 0) {
		return 0;
	}

	if (p_sine < 0) {
		negative++;
		sine = 0 - sine;
	}

	if (PortableS32(sine) >= 0x20000000) {
		result = 0x5a0000;
	}
	else {
		index = 0;
		for (step = 0x80; step; step >>= 1) {
			if (sine >= (MechU32) g_sinTable[index + step]) {
				index += step;
			}
		}

		/* The entry in 8.16 steps, plus the fraction between it and the next one (the idiv faults
		   on two equal entries, which the game's table doesn't have). */
		position = index << 16;
		offset = sine - (MechU32) g_sinTable[index];
		if (offset) {
			span = (MechU32) g_sinTable[index + 1] - (MechU32) g_sinTable[index];
			if (span && offset >= span) {
				position += 0x10000;
			}
			else {
				position =
					(position & 0xffff0000) |
					((MechU32) PortableIdiv((MechS64) PortableS32(offset) * 0x10000, PortableS32(span)) & 0xffff);
			}
		}

		result = (MechS32) (((MechS64) 0x5a000000 * PortableS32(position)) >> 32);
	}

	return negative ? -result : result;
#else
	MechS32 negative;
	MechS32 result;
	MechS32* table;

	negative = 0;
	table = g_sinTable;

	if (p_sine == 0) {
		return 0;
	}

	if (p_sine < 0) {
		negative++;
		p_sine = -p_sine;
	}

	if (p_sine >= 0x20000000) {
		result = 0x5a0000;
	}
	else {
		__asm {
			mov ebx, table
			xor ecx, ecx
			mov eax, p_sine
			cmp eax, [ebx + 0x200]
			jb below_128
			add ebx, 0x200
			or ecx, 0x800000
below_128:
			cmp eax, [ebx + 0x100]
			jb below_64
			add ebx, 0x100
			or ecx, 0x400000
below_64:
			cmp eax, [ebx + 0x80]
			jb below_32
			add ebx, 0x80
			or ecx, 0x200000
below_32:
			cmp eax, [ebx + 0x40]
			jb below_16
			add ebx, 0x40
			or ecx, 0x100000
below_16:
			cmp eax, [ebx + 0x20]
			jb below_8
			add ebx, 0x20
			or ecx, 0x80000
below_8:
			cmp eax, [ebx + 0x10]
			jb below_4
			add ebx, 0x10
			or ecx, 0x40000
below_4:
			cmp eax, [ebx + 0x8]
			jb below_2
			add ebx, 0x8
			or ecx, 0x20000
below_2:
			cmp eax, [ebx + 0x4]
			jb below_1
			add ebx, 0x4
			or ecx, 0x10000
below_1:
			sub eax, [ebx]
			jz scale
			mov esi, [ebx + 0x4]
			sub esi, [ebx]
			jz fraction
			cmp eax, esi
			jb fraction
			add ecx, 0x10000
			jmp scale
fraction:
			cdq
			shld edx, eax, 16
			shl eax, 16
			idiv esi
			mov cx, ax
scale:
			mov eax, 0x5a000000
			imul ecx
			mov result, edx
		}
	}

	return negative ? -result : result;
#endif
}

// FUNCTION: MW2 0x100698b9
MechS32 FixedAcos(MechS32 p_cosine)
{
	return 0x5a0000 - FixedAsin(p_cosine);
}

// Returns the bearing of (p_x, p_z) in 16.16 degrees: the arctangent of the smaller over the
// larger component from the table, folded into its octant. The result is also left in dx:ax.
// FUNCTION: MW2 0x100698de
MechS32 FixedAtan2(MechS32 p_x, MechS32 p_z)
{
#ifdef PORTABLE_C
	/* The magnitudes wrap like neg (INT_MIN stays negative) and compare signed; the quotient,
	   interpolation and folding are unsigned. */
	MechS32 smaller = p_x < 0 ? PortableS32(0 - (MechU32) p_x) : p_x;
	MechS32 larger = p_z < 0 ? PortableS32(0 - (MechU32) p_z) : p_z;
	MechU32 octant = (p_x < 0 ? 1 : 0) | (p_z < 0 ? 2 : 0);
	MechU32 angle;
	MechU32 ratio;
	MechU32 index;
	MechU32 product;
	MechS32 swap;

	if (smaller == larger) {
		angle = 0x2d0000;
	}
	else {
		if (smaller > larger) {
			swap = smaller;
			smaller = larger;
			larger = swap;
			octant |= 4;
		}

		if (smaller == 0) {
			angle = 0;
		}
		else {
			/* The ratio in 8.24 fixed point */
			ratio = PortableDiv((MechU64) (MechU32) smaller << 24, (MechU32) larger);
			index = (ratio >> 16) & 0xff;
			product = (ratio & 0xffff) * ((MechU32) g_atanTable[index + 1] - (MechU32) g_atanTable[index]);
			angle = (product >> 16) + (MechU32) g_atanTable[index] + ((product >> 15) & 1);
		}
	}

	if (octant & 4) {
		angle = 0x5a0000 - angle;
	}

	if (octant & 2) {
		angle = 0xb40000 - angle;
	}

	if (octant & 1) {
		angle = 0 - angle;
	}

	return PortableS32(angle);
#else
	__asm {
		xor cl, cl
		mov edx, p_x
		or edx, edx
		jns x_positive
		neg edx
		or cl, 1
x_positive:
		mov ebx, p_z
		or ebx, ebx
		jns z_positive
		neg ebx
		or cl, 2
z_positive:
		cmp edx, ebx
		jl ordered
		jz diagonal
		xchg edx, ebx
		or cl, 4
		jmp short ordered
diagonal:
		mov eax, 0x2d0000
		jmp short octant
ordered:
		or edx, edx
		jnz divide
		mov eax, 0
		jmp short octant
divide:
		xor eax, eax
		shrd eax, edx, 8
		shr edx, 8
		div ebx
		mov ebx, eax
		shr ebx, 14
		and ebx, 0x3fc
		mov edi, g_atanTable[ebx]
		mov esi, g_atanTable[ebx + 4]
		sub esi, edi
		and eax, 0xffff
		mul esi
		shr eax, 16
		adc eax, edi
octant:
		test cl, 4
		jz below_diagonal
		neg eax
		add eax, 0x5a0000
below_diagonal:
		test cl, 2
		jz z_forward
		neg eax
		add eax, 0xb40000
z_forward:
		test cl, 1
		jz x_forward
		neg eax
x_forward:
		shld edx, eax, 16
	}
#endif
}
