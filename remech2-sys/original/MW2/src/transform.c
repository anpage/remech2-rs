/* In the original, MatrixMul29, MatrixDot29 and MultiplyRotations are C functions whose bodies are
   __asm blocks, and TransformPoint, RotatePoint, OrthogonalizeMatrixColumn and BuildMatrixEx have
   __asm blocks. This is portable C in their place; BuildMatrixEx's wraps where standard C
   overflows. */
#include "transform.h"

#include "clock.h"
#include "fixedtrig.h"
#include "loadres.h"
#include "portable.h"
#include "types.h"

// The 64-bit product of two 32-bit values, as imul leaves it in edx:eax. Sums of products wrap,
// like the add/adc chains.
static MechU64 Product(int32_t p_a, int32_t p_b)
{
	return (MechU64) ((MechS64) p_a * p_b);
}

// A 2.29 product, or sum of products, back to 32 bits: shifted right by 29 and rounded by the
// adc, which also adds p_offset.
static int32_t Round29(MechU64 p_value, int32_t p_offset)
{
	return PortableS32(PortableShrdRound(p_value, 29) + (uint32_t) p_offset);
}

// The product of two 2.29 values.
static int32_t Mul29(int32_t p_a, int32_t p_b)
{
	return Round29(Product(p_a, p_b), 0);
}

// neg: INT_MIN stays INT_MIN.
static int32_t Negate(int32_t p_value)
{
	return PortableS32(0 - (uint32_t) p_value);
}

// Row p_row of p_matrix's rotation times (p_x, p_y, p_z), plus p_offset.
static int32_t TransformRow(Matrix* p_matrix, int32_t p_row, int32_t p_x, int32_t p_y, int32_t p_z, int32_t p_offset)
{
	int32_t* row = p_matrix->m_rows[p_row];

	return Round29(Product(row[0], p_x) + Product(row[1], p_y) + Product(row[2], p_z), p_offset);
}

// Transforms the point (*p_x, *p_y, *p_z) by p_matrix: its 2.29 rotation, then its translation.
// The products are an __asm block.
void TransformPoint(Matrix* p_matrix, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	int32_t x = *p_x;
	int32_t y = *p_y;
	int32_t z = *p_z;

	*p_x = TransformRow(p_matrix, 0, x, y, z, p_matrix->m_rows[3][0]);
	*p_y = TransformRow(p_matrix, 1, x, y, z, p_matrix->m_rows[3][1]);
	*p_z = TransformRow(p_matrix, 2, x, y, z, p_matrix->m_rows[3][2]);
}

// Rotates the point (*p_x, *p_y, *p_z) by p_matrix's 2.29 rotation. The products are an __asm
// block.
void RotatePoint(Matrix* p_matrix, int32_t* p_x, int32_t* p_y, int32_t* p_z)
{
	int32_t x = *p_x;
	int32_t y = *p_y;
	int32_t z = *p_z;

	*p_x = TransformRow(p_matrix, 0, x, y, z, 0);
	*p_y = TransformRow(p_matrix, 1, x, y, z, 0);
	*p_z = TransformRow(p_matrix, 2, x, y, z, 0);
}

// Recomputes column p_column of p_matrix's 2.29 rotation as the cross product of the other two,
// which makes it orthogonal to them again. The products are an __asm block.
void OrthogonalizeMatrixColumn(Matrix* p_matrix, int32_t p_column)
{
	int32_t a;
	int32_t b;
	int32_t cross[3];
	int32_t i;

	/* Another column leaves the products' operands unset, and stores nothing. */
	if (p_column < 0 || p_column > 2) {
		return;
	}

	/* Each row's value from the next two rows of columns a and b. */
	a = (p_column + 1) % 3;
	b = (p_column + 2) % 3;
	for (i = 0; i < 3; i++) {
		int32_t* next = p_matrix->m_rows[(i + 1) % 3];
		int32_t* last = p_matrix->m_rows[(i + 2) % 3];

		cross[i] = Round29(Product(next[a], last[b]) - Product(next[b], last[a]), 0);
	}

	for (i = 0; i < 3; i++) {
		p_matrix->m_rows[i][p_column] = cross[i];
	}
}

// Multiplies two 2.29 fixed-point values. The body is an __asm block.
int32_t MatrixMul29(int32_t p_a, int32_t p_b)
{
	return Mul29(p_a, p_b);
}

// The dot product of two vectors of 2.29 fixed-point values, with a 64-bit sum. The body is an
// __asm block.
int32_t MatrixDot29(int32_t p_ax, int32_t p_ay, int32_t p_az, int32_t p_bx, int32_t p_by, int32_t p_bz)
{
	return Round29(Product(p_ax, p_bx) + Product(p_ay, p_by) + Product(p_az, p_bz), 0);
}

// Multiplies the rotations of p_a and p_b (2.29 fixed point) into p_dst. The
// products are an __asm block.
void MultiplyRotations(Matrix* p_a, Matrix* p_b, Matrix* p_dst)
{
	/* Every product is read before the first store: p_dst may be either operand. */
	int32_t result[3][3];
	int32_t i;
	int32_t j;

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
}

// Composes p_b with p_a into p_dst: the rotations' product, and p_b's
// translation transformed by p_a.
void MultiplyMatrix(Matrix* p_a, Matrix* p_b, Matrix* p_dst)
{
	int32_t z;
	int32_t y;
	int32_t x;
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
void TransposeRotation(Matrix* p_src, Matrix* p_dst)
{
	int32_t temp;

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
void InvertMatrix(Matrix* p_src, Matrix* p_dst)
{
	int32_t z;
	int32_t y;
	int32_t x;

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

void SetIdentityMatrix(Matrix* p_matrix)
{
	p_matrix->m_rows[0][0] = p_matrix->m_rows[1][1] = p_matrix->m_rows[2][2] = 0x20000000;
	p_matrix->m_rows[1][0] = p_matrix->m_rows[2][0] = p_matrix->m_rows[3][0] = 0;
	p_matrix->m_rows[0][1] = p_matrix->m_rows[2][1] = p_matrix->m_rows[3][1] = 0;
	p_matrix->m_rows[0][2] = p_matrix->m_rows[1][2] = p_matrix->m_rows[3][2] = 0;
}

void CopyMatrix(Matrix* p_src, Matrix* p_dst)
{
	MemCopy(p_dst, p_src, sizeof(Matrix));
}

void CopyRotation(Matrix* p_src, Matrix* p_dst)
{
	MemCopy(p_dst, p_src, sizeof(Matrix));
	p_dst->m_rows[3][0] = p_dst->m_rows[3][1] = p_dst->m_rows[3][2] = 0;
}

// Builds p_matrix from three rotation angles (16.16 degrees) and a translation. The bits 0-1 of
// p_flags pick the rotation order, and bit 2 builds the inverse rotation. A rotation about one axis
// only is built directly. The products are __asm blocks.
void BuildMatrixEx(
	Matrix* p_matrix,
	int32_t p_angleX,
	int32_t p_angleY,
	int32_t p_angleZ,
	int32_t p_x,
	int32_t p_y,
	int32_t p_z,
	uint32_t p_flags
)
{
	int32_t sa;
	int32_t sb;
	int32_t sc;
	int32_t ca;
	int32_t cb;
	int32_t cc;

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
			p_matrix->m_rows[1][0] = PortableS32((uint32_t) Mul29(ca, sc) + (uint32_t) Mul29(Mul29(cc, sa), sb));
			p_matrix->m_rows[2][0] = PortableS32((uint32_t) Mul29(sc, sa) - (uint32_t) Mul29(Mul29(cc, ca), sb));
			p_matrix->m_rows[0][2] = sb;
			p_matrix->m_rows[1][2] = Negate(Mul29(cb, sa));
			p_matrix->m_rows[2][2] = Mul29(ca, cb);
			OrthogonalizeMatrixColumn(p_matrix, 1);
			break;
		case 0:
			p_matrix->m_rows[0][0] = PortableS32((uint32_t) Mul29(cc, cb) + (uint32_t) Mul29(Mul29(sc, sa), sb));
			p_matrix->m_rows[1][0] = Mul29(ca, sc);
			p_matrix->m_rows[2][0] = PortableS32((uint32_t) Mul29(Mul29(cb, sc), sa) - (uint32_t) Mul29(cc, sb));
			p_matrix->m_rows[0][2] = Mul29(ca, sb);
			p_matrix->m_rows[1][2] = Negate(sa);
			p_matrix->m_rows[2][2] = Mul29(ca, cb);
			OrthogonalizeMatrixColumn(p_matrix, 1);
			break;
		case 2:
			p_matrix->m_rows[0][0] = Mul29(cc, cb);
			p_matrix->m_rows[1][0] = PortableS32((uint32_t) Mul29(sa, sb) + (uint32_t) Mul29(Mul29(ca, cb), sc));
			p_matrix->m_rows[2][0] = PortableS32((uint32_t) Mul29(Mul29(cb, sc), sa) - (uint32_t) Mul29(ca, sb));
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

	p_matrix->m_rows[3][0] = p_x;
	p_matrix->m_rows[3][1] = p_y;
	p_matrix->m_rows[3][2] = p_z;
}

void BuildMatrix(
	Matrix* p_matrix,
	int32_t p_angleX,
	int32_t p_angleY,
	int32_t p_angleZ,
	int32_t p_x,
	int32_t p_y,
	int32_t p_z
)
{
	BuildMatrixEx(p_matrix, p_angleX, p_angleY, p_angleZ, p_x, p_y, p_z, 0);
}

// Reads the three rotation angles (16.16 degrees) back out of p_matrix. Near straight up or down
// (the pitch sine within 0.001 of 1), the angle comes from the other rows; with nothing to go on,
// the yaw is taken as 90 degrees and the roll from the first row.
void GetMatrixAngles(Matrix* p_matrix, uint32_t* p_angleX, uint32_t* p_angleY, uint32_t* p_angleZ)
{
	int32_t pitch;
	int32_t roll;
	int32_t length;
	int32_t yaw;

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
