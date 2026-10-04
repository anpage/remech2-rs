/* Hand-written assembly: MatrixMul29, MatrixDot29 and MultiplyRotations are C functions whose bodies
   are __asm blocks, and TransformPoint, RotatePoint, OrthogonalizeMatrixColumn and BuildMatrixEx have __asm
   blocks. Their portable C (PORTABLE_C) is tested against the assembly by tests/asmequiv: it
   replaces BuildMatrixEx's whole body, whose C wraps where standard C overflows. */
#include "transform.h"

#include "clock.h"
#include "compat.h"
#include "decomp.h"
#include "fixedtrig.h"
#include "loadres.h"
#include "portable.h"
#include "types.h"

#ifdef PORTABLE_C
// The 64-bit product of two 32-bit values, as imul leaves it in edx:eax. Sums of products wrap,
// like the add/adc chains.
static MechU64 Product(MechS32 p_a, MechS32 p_b)
{
	return (MechU64) ((MechS64) p_a * p_b);
}

// A 2.29 product, or sum of products, back to 32 bits: shifted right by 29 and rounded by the
// adc, which also adds p_offset.
static MechS32 Round29(MechU64 p_value, MechS32 p_offset)
{
	return PortableS32(PortableShrdRound(p_value, 29) + (MechU32) p_offset);
}

// The product of two 2.29 values.
static MechS32 Mul29(MechS32 p_a, MechS32 p_b)
{
	return Round29(Product(p_a, p_b), 0);
}

// neg: INT_MIN stays INT_MIN.
static MechS32 Negate(MechS32 p_value)
{
	return PortableS32(0 - (MechU32) p_value);
}

// Row p_row of p_matrix's rotation times (p_x, p_y, p_z), plus p_offset.
static MechS32 TransformRow(Matrix* p_matrix, MechS32 p_row, MechS32 p_x, MechS32 p_y, MechS32 p_z, MechS32 p_offset)
{
	MechS32* row = p_matrix->m_rows[p_row];

	return Round29(Product(row[0], p_x) + Product(row[1], p_y) + Product(row[2], p_z), p_offset);
}
#endif

// Transforms the point (*p_x, *p_y, *p_z) by p_matrix: its 2.29 rotation, then its translation.
// The products are an __asm block.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1000d650
void TransformPoint(Matrix* p_matrix, MechS32* p_x, MechS32* p_y, MechS32* p_z)
{
#ifdef PORTABLE_C
	MechS32 x = *p_x;
	MechS32 y = *p_y;
	MechS32 z = *p_z;

	*p_x = TransformRow(p_matrix, 0, x, y, z, p_matrix->m_rows[3][0]);
	*p_y = TransformRow(p_matrix, 1, x, y, z, p_matrix->m_rows[3][1]);
	*p_z = TransformRow(p_matrix, 2, x, y, z, p_matrix->m_rows[3][2]);
#else
	MechS32 rz;
	MechS32 x;
	MechS32 rx;
	MechS32 y;
	MechS32 z;
	MechS32 ry;

	x = *p_x;
	y = *p_y;
	z = *p_z;
	__asm {
		mov edi, p_matrix
		mov eax, dword ptr [edi]
		imul x
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [edi + 0x4]
		imul y
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [edi + 0x8]
		imul z
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 29
		adc eax, dword ptr [edi + 0x24]
		mov rx, eax
		mov eax, dword ptr [edi + 0xc]
		imul x
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [edi + 0x10]
		imul y
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [edi + 0x14]
		imul z
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 29
		adc eax, dword ptr [edi + 0x28]
		mov ry, eax
		mov eax, dword ptr [edi + 0x18]
		imul x
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [edi + 0x1c]
		imul y
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [edi + 0x20]
		imul z
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 29
		adc eax, dword ptr [edi + 0x2c]
		mov rz, eax
	}

	*p_x = rx;
	*p_y = ry;
	*p_z = rz;
#endif
}

// Rotates the point (*p_x, *p_y, *p_z) by p_matrix's 2.29 rotation. The products are an __asm
// block.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x1000d708
void RotatePoint(Matrix* p_matrix, MechS32* p_x, MechS32* p_y, MechS32* p_z)
{
#ifdef PORTABLE_C
	MechS32 x = *p_x;
	MechS32 y = *p_y;
	MechS32 z = *p_z;

	*p_x = TransformRow(p_matrix, 0, x, y, z, 0);
	*p_y = TransformRow(p_matrix, 1, x, y, z, 0);
	*p_z = TransformRow(p_matrix, 2, x, y, z, 0);
#else
	MechS32 rz;
	MechS32 x;
	MechS32 rx;
	MechS32 y;
	MechS32 z;
	MechS32 ry;

	x = *p_x;
	y = *p_y;
	z = *p_z;
	__asm {
		mov edi, p_matrix
		mov eax, dword ptr [edi]
		imul x
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [edi + 0x4]
		imul y
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [edi + 0x8]
		imul z
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 29
		adc eax, 0
		mov rx, eax
		mov eax, dword ptr [edi + 0xc]
		imul x
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [edi + 0x10]
		imul y
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [edi + 0x14]
		imul z
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 29
		adc eax, 0
		mov ry, eax
		mov eax, dword ptr [edi + 0x18]
		imul x
		mov ecx, edx
		mov ebx, eax
		mov eax, dword ptr [edi + 0x1c]
		imul y
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [edi + 0x20]
		imul z
		add eax, ebx
		adc edx, ecx
		shrd eax, edx, 29
		adc eax, 0
		mov rz, eax
	}

	*p_x = rx;
	*p_y = ry;
	*p_z = rz;
#endif
}

// Recomputes column p_column of p_matrix's 2.29 rotation as the cross product of the other two,
// which makes it orthogonal to them again. The products are an __asm block.
// Stack-slot permutation: the column values and x, y and z.
// FUNCTION: MW2 0x1000d7c0
void OrthogonalizeMatrixColumn(Matrix* p_matrix, MechS32 p_column)
{
#ifdef PORTABLE_C
	MechS32 a;
	MechS32 b;
	MechS32 cross[3];
	MechS32 i;

	/* Another column leaves the products' operands unset, and stores nothing. */
	if (p_column < 0 || p_column > 2) {
		return;
	}

	/* Each row's value from the next two rows of columns a and b. */
	a = (p_column + 1) % 3;
	b = (p_column + 2) % 3;
	for (i = 0; i < 3; i++) {
		MechS32* next = p_matrix->m_rows[(i + 1) % 3];
		MechS32* last = p_matrix->m_rows[(i + 2) % 3];

		cross[i] = Round29(Product(next[a], last[b]) - Product(next[b], last[a]), 0);
	}

	for (i = 0; i < 3; i++) {
		p_matrix->m_rows[i][p_column] = cross[i];
	}
#else
	MechS32 a2;
	MechS32 b2;
	MechS32 a0;
	MechS32 x;
	MechS32 b0;
	MechS32 y;
	MechS32 z;
	MechS32 a1;
	MechS32 b1;

	switch (p_column) {
	case 0:
		a0 = p_matrix->m_rows[0][1];
		a1 = p_matrix->m_rows[1][1];
		a2 = p_matrix->m_rows[2][1];
		b0 = p_matrix->m_rows[0][2];
		b1 = p_matrix->m_rows[1][2];
		b2 = p_matrix->m_rows[2][2];
		break;
	case 1:
		a0 = p_matrix->m_rows[0][2];
		a1 = p_matrix->m_rows[1][2];
		a2 = p_matrix->m_rows[2][2];
		b0 = p_matrix->m_rows[0][0];
		b1 = p_matrix->m_rows[1][0];
		b2 = p_matrix->m_rows[2][0];
		break;
	case 2:
		a0 = p_matrix->m_rows[0][0];
		a1 = p_matrix->m_rows[1][0];
		a2 = p_matrix->m_rows[2][0];
		b0 = p_matrix->m_rows[0][1];
		b1 = p_matrix->m_rows[1][1];
		b2 = p_matrix->m_rows[2][1];
		break;
	}

	__asm {
		mov eax, a1
		mov edx, b2
		imul edx
		mov edi, edx
		mov esi, eax
		mov eax, b1
		mov edx, a2
		imul edx
		sub esi, eax
		sbb edi, edx
		shrd esi, edi, 29
		adc esi, 0
		mov x, esi
		mov eax, a2
		mov edx, b0
		imul edx
		mov edi, edx
		mov esi, eax
		mov eax, b2
		mov edx, a0
		imul edx
		sub esi, eax
		sbb edi, edx
		shrd esi, edi, 29
		adc esi, 0
		mov y, esi
		mov eax, a0
		mov edx, b1
		imul edx
		mov edi, edx
		mov esi, eax
		mov eax, b0
		mov edx, a1
		imul edx
		sub esi, eax
		sbb edi, edx
		shrd esi, edi, 29
		adc esi, 0
		mov z, esi
	}

	switch (p_column)
	{
	case 0:
		p_matrix->m_rows[0][0] = x;
		p_matrix->m_rows[1][0] = y;
		p_matrix->m_rows[2][0] = z;
		break;
	case 1:
		p_matrix->m_rows[0][1] = x;
		p_matrix->m_rows[1][1] = y;
		p_matrix->m_rows[2][1] = z;
		break;
	case 2:
		p_matrix->m_rows[0][2] = x;
		p_matrix->m_rows[1][2] = y;
		p_matrix->m_rows[2][2] = z;
		break;
	}
#endif
}

// Multiplies two 2.29 fixed-point values. The body is an __asm block.
// FUNCTION: MW2 0x1000d9a8
MechS32 MatrixMul29(MechS32 p_a, MechS32 p_b)
{
#ifdef PORTABLE_C
	return Mul29(p_a, p_b);
#else
	MechS32 result;

	__asm {
		mov eax, p_a
		imul p_b
		shrd eax, edx, 29
		adc eax, 0
		mov result, eax
	}

	return result;
#endif
}

// The dot product of two vectors of 2.29 fixed-point values, with a 64-bit sum. The body is an
// __asm block.
// FUNCTION: MW2 0x1000d9ce
MechS32 MatrixDot29(MechS32 p_ax, MechS32 p_ay, MechS32 p_az, MechS32 p_bx, MechS32 p_by, MechS32 p_bz)
{
#ifdef PORTABLE_C
	return Round29(Product(p_ax, p_bx) + Product(p_ay, p_by) + Product(p_az, p_bz), 0);
#else
	MechS32 result;

	__asm {
		mov eax, p_ax
		imul p_bx
		mov esi, eax
		mov edi, edx
		mov eax, p_ay
		imul p_by
		add esi, eax
		adc edi, edx
		mov eax, p_az
		imul p_bz
		add esi, eax
		adc edi, edx
		shrd esi, edi, 29
		adc esi, 0
		mov result, esi
	}

	return result;
#endif
}

// Multiplies the rotations of p_a and p_b (2.29 fixed point) into p_dst. The
// products are an __asm block.
// Stack-slot permutation of the locals the __asm block names.
// FUNCTION: MW2 0x1000da0c
void MultiplyRotations(Matrix* p_a, Matrix* p_b, Matrix* p_dst)
{
#ifdef PORTABLE_C
	/* Every product is read before the first store: p_dst may be either operand. */
	MechS32 result[3][3];
	MechS32 i;
	MechS32 j;

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			result[i][j] = Round29(
				Product(p_a->m_rows[i][0], p_b->m_rows[0][j]) + Product(p_a->m_rows[i][1], p_b->m_rows[1][j]) +
					Product(p_a->m_rows[i][2], p_b->m_rows[2][j]),
				0
			);
		}
	}

	for (i = 0; i < 3; i++) {
		for (j = 0; j < 3; j++) {
			p_dst->m_rows[i][j] = result[i][j];
		}
	}
#else
	MechS32 m00;
	MechS32 m01;
	MechS32 m02;
	MechS32 m10;
	MechS32 m11;
	MechS32 m12;
	MechS32 m20;
	MechS32 m21;
	MechS32 m22;

	__asm {
		mov esi, p_a
		mov edi, p_b
		mov eax, dword ptr [esi]
		imul dword ptr [edi]
		mov ebx, eax
		mov ecx, edx
		mov eax, dword ptr [esi + 4]
		imul dword ptr [edi + 0xc]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 8]
		imul dword ptr [edi + 0x18]
		add ebx, eax
		adc ecx, edx
		shrd ebx, ecx, 0x1d
		adc ebx, 0
		mov m00, ebx
		mov eax, dword ptr [esi]
		imul dword ptr [edi + 4]
		mov ebx, eax
		mov ecx, edx
		mov eax, dword ptr [esi + 4]
		imul dword ptr [edi + 0x10]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 8]
		imul dword ptr [edi + 0x1c]
		add ebx, eax
		adc ecx, edx
		shrd ebx, ecx, 0x1d
		adc ebx, 0
		mov m01, ebx
		mov eax, dword ptr [esi]
		imul dword ptr [edi + 8]
		mov ebx, eax
		mov ecx, edx
		mov eax, dword ptr [esi + 4]
		imul dword ptr [edi + 0x14]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 8]
		imul dword ptr [edi + 0x20]
		add ebx, eax
		adc ecx, edx
		shrd ebx, ecx, 0x1d
		adc ebx, 0
		mov m02, ebx
		mov eax, dword ptr [esi + 0xc]
		imul dword ptr [edi]
		mov ebx, eax
		mov ecx, edx
		mov eax, dword ptr [esi + 0x10]
		imul dword ptr [edi + 0xc]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x14]
		imul dword ptr [edi + 0x18]
		add ebx, eax
		adc ecx, edx
		shrd ebx, ecx, 0x1d
		adc ebx, 0
		mov m10, ebx
		mov eax, dword ptr [esi + 0xc]
		imul dword ptr [edi + 4]
		mov ebx, eax
		mov ecx, edx
		mov eax, dword ptr [esi + 0x10]
		imul dword ptr [edi + 0x10]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x14]
		imul dword ptr [edi + 0x1c]
		add ebx, eax
		adc ecx, edx
		shrd ebx, ecx, 0x1d
		adc ebx, 0
		mov m11, ebx
		mov eax, dword ptr [esi + 0xc]
		imul dword ptr [edi + 8]
		mov ebx, eax
		mov ecx, edx
		mov eax, dword ptr [esi + 0x10]
		imul dword ptr [edi + 0x14]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x14]
		imul dword ptr [edi + 0x20]
		add ebx, eax
		adc ecx, edx
		shrd ebx, ecx, 0x1d
		adc ebx, 0
		mov m12, ebx
		mov eax, dword ptr [esi + 0x18]
		imul dword ptr [edi]
		mov ebx, eax
		mov ecx, edx
		mov eax, dword ptr [esi + 0x1c]
		imul dword ptr [edi + 0xc]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x20]
		imul dword ptr [edi + 0x18]
		add ebx, eax
		adc ecx, edx
		shrd ebx, ecx, 0x1d
		adc ebx, 0
		mov m20, ebx
		mov eax, dword ptr [esi + 0x18]
		imul dword ptr [edi + 4]
		mov ebx, eax
		mov ecx, edx
		mov eax, dword ptr [esi + 0x1c]
		imul dword ptr [edi + 0x10]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x20]
		imul dword ptr [edi + 0x1c]
		add ebx, eax
		adc ecx, edx
		shrd ebx, ecx, 0x1d
		adc ebx, 0
		mov m21, ebx
		mov eax, dword ptr [esi + 0x18]
		imul dword ptr [edi + 8]
		mov ebx, eax
		mov ecx, edx
		mov eax, dword ptr [esi + 0x1c]
		imul dword ptr [edi + 0x14]
		add ebx, eax
		adc ecx, edx
		mov eax, dword ptr [esi + 0x20]
		imul dword ptr [edi + 0x20]
		add ebx, eax
		adc ecx, edx
		shrd ebx, ecx, 0x1d
		adc ebx, 0
		mov m22, ebx
		mov edi, p_dst
		mov eax, m00
		mov dword ptr [edi], eax
		mov eax, m01
		mov dword ptr [edi + 4], eax
		mov eax, m10
		mov dword ptr [edi + 0xc], eax
		mov eax, m11
		mov dword ptr [edi + 0x10], eax
		mov eax, m20
		mov dword ptr [edi + 0x18], eax
		mov eax, m21
		mov dword ptr [edi + 0x1c], eax
		mov eax, m02
		mov dword ptr [edi + 8], eax
		mov eax, m12
		mov dword ptr [edi + 0x14], eax
		mov eax, m22
		mov dword ptr [edi + 0x20], eax
	}
#endif
}

// Composes p_b with p_a into p_dst: the rotations' product, and p_b's
// translation transformed by p_a.
// FUNCTION: MW2 0x1000dbba
void MultiplyMatrix(Matrix* p_a, Matrix* p_b, Matrix* p_dst)
{
	MechS32 z;
	MechS32 y;
	MechS32 x;
	Matrix result;

	MultiplyRotations(p_a, p_b, &result);
	x = p_b->m_rows[3][0];
	y = p_b->m_rows[3][1];
	z = p_b->m_rows[3][2];
	TransformPoint(p_a, &x, &y, &z);
	result.m_rows[3][0] = x;
	result.m_rows[3][1] = y;
	result.m_rows[3][2] = z;
	MemCopy(p_dst, &result, sizeof(Matrix));
}

// Transposes the rotation of p_src into p_dst.
// FUNCTION: MW2 0x1000dc33
void TransposeRotation(Matrix* p_src, Matrix* p_dst)
{
	MechS32 temp;

	p_dst->m_rows[0][0] = p_src->m_rows[0][0];
	p_dst->m_rows[1][1] = p_src->m_rows[1][1];
	p_dst->m_rows[2][2] = p_src->m_rows[2][2];
	temp = p_src->m_rows[0][1];
	p_dst->m_rows[0][1] = p_src->m_rows[1][0];
	p_dst->m_rows[1][0] = temp;
	temp = p_src->m_rows[2][0];
	p_dst->m_rows[2][0] = p_src->m_rows[0][2];
	p_dst->m_rows[0][2] = temp;
	temp = p_src->m_rows[2][1];
	p_dst->m_rows[2][1] = p_src->m_rows[1][2];
	p_dst->m_rows[1][2] = temp;
}

// Inverts a rigid transform: the transposed rotation, and the negated translation rotated by it.
// FUNCTION: MW2 0x1000dcbd
void InvertMatrix(Matrix* p_src, Matrix* p_dst)
{
	MechS32 z;
	MechS32 y;
	MechS32 x;

	x = -p_src->m_rows[3][0];
	y = -p_src->m_rows[3][1];
	z = -p_src->m_rows[3][2];
	TransposeRotation(p_src, p_dst);
	p_dst->m_rows[3][0] = 0;
	p_dst->m_rows[3][1] = 0;
	p_dst->m_rows[3][2] = 0;
	TransformPoint(p_dst, &x, &y, &z);
	p_dst->m_rows[3][0] = x;
	p_dst->m_rows[3][1] = y;
	p_dst->m_rows[3][2] = z;
}

// FUNCTION: MW2 0x1000dd4d
void SetIdentityMatrix(Matrix* p_matrix)
{
	p_matrix->m_rows[0][0] = p_matrix->m_rows[1][1] = p_matrix->m_rows[2][2] = 0x20000000;
	p_matrix->m_rows[1][0] = p_matrix->m_rows[2][0] = p_matrix->m_rows[3][0] = 0;
	p_matrix->m_rows[0][1] = p_matrix->m_rows[2][1] = p_matrix->m_rows[3][1] = 0;
	p_matrix->m_rows[0][2] = p_matrix->m_rows[1][2] = p_matrix->m_rows[3][2] = 0;
}

// FUNCTION: MW2 0x1000dddf
void CopyMatrix(Matrix* p_src, Matrix* p_dst)
{
	MemCopy(p_dst, p_src, sizeof(Matrix));
}

// FUNCTION: MW2 0x1000ddfc
void CopyRotation(Matrix* p_src, Matrix* p_dst)
{
	MemCopy(p_dst, p_src, sizeof(Matrix));
	p_dst->m_rows[3][0] = p_dst->m_rows[3][1] = p_dst->m_rows[3][2] = 0;
}

// Builds p_matrix from three rotation angles (16.16 degrees) and a translation. The bits 0-1 of
// p_flags pick the rotation order, and bit 2 builds the inverse rotation. A rotation about one axis
// only is built directly. The products are __asm blocks.
// Stack-slot permutation: t, the sines and the cosines.
// FUNCTION: MW2 0x1000de3b
void BuildMatrixEx(
	Matrix* p_matrix,
	MechS32 p_angleX,
	MechS32 p_angleY,
	MechS32 p_angleZ,
	MechS32 p_x,
	MechS32 p_y,
	MechS32 p_z,
	MechU32 p_flags
)
{
#ifdef PORTABLE_C
	MechS32 sa;
	MechS32 sb;
	MechS32 sc;
	MechS32 ca;
	MechS32 cb;
	MechS32 cc;

	do {
		if (p_angleX == 0) {
			if (p_angleY == 0) {
				SetIdentityMatrix(p_matrix);
				if (p_angleZ) {
					p_matrix->m_rows[1][1] = FixedCos(p_angleZ);
					p_matrix->m_rows[0][0] = p_matrix->m_rows[1][1];
					p_matrix->m_rows[1][0] = FixedSin(p_angleZ);
					p_matrix->m_rows[0][1] = Negate(p_matrix->m_rows[1][0]);
				}
				break;
			}
			else if (p_angleZ == 0) {
				SetIdentityMatrix(p_matrix);
				p_matrix->m_rows[2][2] = FixedCos(p_angleY);
				p_matrix->m_rows[0][0] = p_matrix->m_rows[2][2];
				p_matrix->m_rows[0][2] = FixedSin(p_angleY);
				p_matrix->m_rows[2][0] = Negate(p_matrix->m_rows[0][2]);
				break;
			}
		}
		else if (p_angleY == 0 && p_angleZ == 0) {
			SetIdentityMatrix(p_matrix);
			p_matrix->m_rows[2][2] = FixedCos(p_angleX);
			p_matrix->m_rows[1][1] = p_matrix->m_rows[2][2];
			p_matrix->m_rows[2][1] = FixedSin(p_angleX);
			p_matrix->m_rows[1][2] = Negate(p_matrix->m_rows[2][1]);
			break;
		}

		ca = FixedCos(p_angleX);
		cb = FixedCos(p_angleY);
		cc = FixedCos(p_angleZ);
		sa = FixedSin(p_angleX);
		sb = FixedSin(p_angleY);
		sc = FixedSin(p_angleZ);
		if (p_flags & 4) {
			sa = Negate(sa);
			sb = Negate(sb);
			sc = Negate(sc);
		}

		/* The sums and differences of the products wrap. */
		switch (p_flags & 3) {
		case 1:
			p_matrix->m_rows[0][0] = Mul29(cc, cb);
			p_matrix->m_rows[1][0] = PortableS32((MechU32) Mul29(ca, sc) + (MechU32) Mul29(Mul29(cc, sa), sb));
			p_matrix->m_rows[2][0] = PortableS32((MechU32) Mul29(sc, sa) - (MechU32) Mul29(Mul29(cc, ca), sb));
			p_matrix->m_rows[0][2] = sb;
			p_matrix->m_rows[1][2] = Negate(Mul29(cb, sa));
			p_matrix->m_rows[2][2] = Mul29(ca, cb);
			OrthogonalizeMatrixColumn(p_matrix, 1);
			break;
		case 0:
			p_matrix->m_rows[0][0] = PortableS32((MechU32) Mul29(cc, cb) + (MechU32) Mul29(Mul29(sc, sa), sb));
			p_matrix->m_rows[1][0] = Mul29(ca, sc);
			p_matrix->m_rows[2][0] = PortableS32((MechU32) Mul29(Mul29(cb, sc), sa) - (MechU32) Mul29(cc, sb));
			p_matrix->m_rows[0][2] = Mul29(ca, sb);
			p_matrix->m_rows[1][2] = Negate(sa);
			p_matrix->m_rows[2][2] = Mul29(ca, cb);
			OrthogonalizeMatrixColumn(p_matrix, 1);
			break;
		case 2:
			p_matrix->m_rows[0][0] = Mul29(cc, cb);
			p_matrix->m_rows[1][0] = PortableS32((MechU32) Mul29(sa, sb) + (MechU32) Mul29(Mul29(ca, cb), sc));
			p_matrix->m_rows[2][0] = PortableS32((MechU32) Mul29(Mul29(cb, sc), sa) - (MechU32) Mul29(ca, sb));
			p_matrix->m_rows[0][1] = Negate(sc);
			p_matrix->m_rows[1][1] = Mul29(cc, ca);
			p_matrix->m_rows[2][1] = Mul29(cc, sa);
			OrthogonalizeMatrixColumn(p_matrix, 2);
			break;
		}

		if (p_flags & 4) {
			TransposeRotation(p_matrix, p_matrix);
		}
	} while (0);
#else
	MechS32 t;
	MechS32 sa;
	MechS32 sb;
	MechS32 sc;
	MechS32 ca;
	MechS32 cb;
	MechS32 cc;

	do {
		if (p_angleX == 0) {
			if (p_angleY == 0) {
				SetIdentityMatrix(p_matrix);
				if (p_angleZ) {
					p_matrix->m_rows[1][1] = FixedCos(p_angleZ);
					p_matrix->m_rows[0][0] = p_matrix->m_rows[1][1];
					p_matrix->m_rows[1][0] = FixedSin(p_angleZ);
					p_matrix->m_rows[0][1] = -p_matrix->m_rows[1][0];
				}
				break;
			}
			else if (p_angleZ == 0) {
				SetIdentityMatrix(p_matrix);
				p_matrix->m_rows[2][2] = FixedCos(p_angleY);
				p_matrix->m_rows[0][0] = p_matrix->m_rows[2][2];
				p_matrix->m_rows[0][2] = FixedSin(p_angleY);
				p_matrix->m_rows[2][0] = -p_matrix->m_rows[0][2];
				break;
			}
		}
		else if (p_angleY == 0 && p_angleZ == 0) {
			SetIdentityMatrix(p_matrix);
			p_matrix->m_rows[2][2] = FixedCos(p_angleX);
			p_matrix->m_rows[1][1] = p_matrix->m_rows[2][2];
			p_matrix->m_rows[2][1] = FixedSin(p_angleX);
			p_matrix->m_rows[1][2] = -p_matrix->m_rows[2][1];
			break;
		}

		ca = FixedCos(p_angleX);
		cb = FixedCos(p_angleY);
		cc = FixedCos(p_angleZ);
		sa = FixedSin(p_angleX);
		sb = FixedSin(p_angleY);
		sc = FixedSin(p_angleZ);
		if (p_flags & 4) {
			__asm {
				neg sa
				neg sb
				neg sc
			}
		}

		switch (p_flags & 3) {
		case 1:
			__asm {
				mov eax, cc
				imul cb
				shrd eax, edx, 0x1d
				adc eax, 0
				mov t, eax
			}
			p_matrix->m_rows[0][0] = t;
			__asm {
				mov eax, ca
				imul sc
				shrd eax, edx, 0x1d
				adc eax, 0
				mov ecx, eax
				mov eax, cc
				imul sa
				shrd eax, edx, 0x1d
				adc eax, 0
				imul sb
				shrd eax, edx, 0x1d
				adc ecx, eax
				mov t, ecx
			}
			p_matrix->m_rows[1][0] = t;
			__asm {
				mov eax, sc
				imul sa
				shrd eax, edx, 0x1d
				adc eax, 0
				mov ecx, eax
				mov eax, cc
				imul ca
				shrd eax, edx, 0x1d
				adc eax, 0
				imul sb
				shrd eax, edx, 0x1d
				adc eax, 0
				sub ecx, eax
				mov t, ecx
			}
			p_matrix->m_rows[2][0] = t;
			p_matrix->m_rows[0][2] = sb;
			__asm {
				mov eax, cb
				imul sa
				shrd eax, edx, 0x1d
				adc eax, 0
				neg eax
				mov t, eax
			}
			p_matrix->m_rows[1][2] = t;
			__asm {
				mov eax, ca
				imul cb
				shrd eax, edx, 0x1d
				adc eax, 0
				mov t, eax
			}
			p_matrix->m_rows[2][2] = t;
			OrthogonalizeMatrixColumn(p_matrix, 1);
			break;
		case 0:
			__asm {
				mov eax, cc
				imul cb
				shrd eax, edx, 0x1d
				adc eax, 0
				mov ecx, eax
				mov eax, sc
				imul sa
				shrd eax, edx, 0x1d
				adc eax, 0
				imul sb
				shrd eax, edx, 0x1d
				adc ecx, eax
				mov t, ecx
			}
			p_matrix->m_rows[0][0] = t;
			__asm {
				mov eax, ca
				imul sc
				shrd eax, edx, 0x1d
				adc eax, 0
				mov t, eax
			}
			p_matrix->m_rows[1][0] = t;
			__asm {
				mov eax, cb
				imul sc
				shrd eax, edx, 0x1d
				adc eax, 0
				imul sa
				shrd eax, edx, 0x1d
				adc eax, 0
				mov ecx, eax
				mov eax, cc
				imul sb
				shrd eax, edx, 0x1d
				adc eax, 0
				sub ecx, eax
				mov t, ecx
			}
			p_matrix->m_rows[2][0] = t;
			__asm {
				mov eax, ca
				imul sb
				shrd eax, edx, 0x1d
				adc eax, 0
				mov t, eax
			}
			p_matrix->m_rows[0][2] = t;
			p_matrix->m_rows[1][2] = -sa;
			__asm {
				mov eax, ca
				imul cb
				shrd eax, edx, 0x1d
				adc eax, 0
				mov t, eax
			}
			p_matrix->m_rows[2][2] = t;
			OrthogonalizeMatrixColumn(p_matrix, 1);
			break;
		case 2:
			__asm {
				mov eax, cc
				imul cb
				shrd eax, edx, 0x1d
				adc eax, 0
				mov t, eax
			}
			p_matrix->m_rows[0][0] = t;
			__asm {
				mov eax, sa
				imul sb
				shrd eax, edx, 0x1d
				adc eax, 0
				mov ecx, eax
				mov eax, ca
				imul cb
				shrd eax, edx, 0x1d
				adc eax, 0
				imul sc
				shrd eax, edx, 0x1d
				adc ecx, eax
				mov t, ecx
			}
			p_matrix->m_rows[1][0] = t;
			__asm {
				mov eax, cb
				imul sc
				shrd eax, edx, 0x1d
				adc eax, 0
				imul sa
				shrd eax, edx, 0x1d
				adc eax, 0
				mov ecx, eax
				mov eax, ca
				imul sb
				shrd eax, edx, 0x1d
				adc eax, 0
				sub ecx, eax
				mov t, ecx
			}
			p_matrix->m_rows[2][0] = t;
			p_matrix->m_rows[0][1] = -sc;
			__asm {
				mov eax, cc
				imul ca
				shrd eax, edx, 0x1d
				adc eax, 0
				mov t, eax
			}
			p_matrix->m_rows[1][1] = t;
			__asm {
				mov eax, cc
				imul sa
				shrd eax, edx, 0x1d
				adc eax, 0
				mov t, eax
			}
			p_matrix->m_rows[2][1] = t;
			OrthogonalizeMatrixColumn(p_matrix, 2);
			break;
		}

		if (p_flags & 4) {
			TransposeRotation(p_matrix, p_matrix);
		}
	} while (0);
#endif

	p_matrix->m_rows[3][0] = p_x;
	p_matrix->m_rows[3][1] = p_y;
	p_matrix->m_rows[3][2] = p_z;
}

// FUNCTION: MW2 0x1000e2b9
void BuildMatrix(
	Matrix* p_matrix,
	MechS32 p_angleX,
	MechS32 p_angleY,
	MechS32 p_angleZ,
	MechS32 p_x,
	MechS32 p_y,
	MechS32 p_z
)
{
	BuildMatrixEx(p_matrix, p_angleX, p_angleY, p_angleZ, p_x, p_y, p_z, 0);
}

// Reads the three rotation angles (16.16 degrees) back out of p_matrix. Near straight up or down
// (the pitch sine within 0.001 of 1), the angle comes from the other rows; with nothing to go on,
// the yaw is taken as 90 degrees and the roll from the first row.
// Stack-slot permutation: pitch, roll, length and yaw.
// FUNCTION: MW2 0x1000e2ea
void GetMatrixAngles(Matrix* p_matrix, undefined4* p_angleX, undefined4* p_angleY, undefined4* p_angleZ)
{
	MechS32 pitch;
	MechS32 roll;
	MechS32 length;
	MechS32 yaw;

	if (p_matrix->m_rows[1][2] > 0x1ff7ced9 || p_matrix->m_rows[1][2] < -0x1ff7ced9) {
		length = Hypot2D(p_matrix->m_rows[0][2], p_matrix->m_rows[2][2]);
		if (length > 2000000) {
			pitch = FixedAcos(length);
			if (p_matrix->m_rows[1][2] < 0) {
				pitch = -pitch;
			}
		}
		else {
			if (p_matrix->m_rows[1][2] >= 0) {
				yaw = -0x5a0000;
			}
			else {
				yaw = 0x5a0000;
			}

			pitch = 0;
			roll = FixedAtan2(p_matrix->m_rows[0][1], p_matrix->m_rows[0][0]);
			goto done;
		}
	}
	else {
		pitch = -FixedAsin(p_matrix->m_rows[1][2]);
	}

	yaw = FixedAtan2(p_matrix->m_rows[0][2], p_matrix->m_rows[2][2]);
	roll = FixedAtan2(p_matrix->m_rows[1][0], p_matrix->m_rows[1][1]);

done:
	*p_angleY = yaw;
	*p_angleX = pitch;
	*p_angleZ = roll;
}
