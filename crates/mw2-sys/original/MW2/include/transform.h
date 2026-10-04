#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "decomp.h"
#include "types.h"

/* A 2.29 fixed-point transform: three rotation rows and a translation row
   (SetIdentityMatrix sets the identity, 0x20000000 on the diagonal). */
typedef struct Matrix Matrix;

// SIZE 0x30
struct Matrix {
	MechS32 m_rows[4][3]; // 0x00
};

// The functions and globals of transform.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 MatrixMul29(MechS32 p_a, MechS32 p_b);
	MechS32 MatrixDot29(MechS32 p_ax, MechS32 p_ay, MechS32 p_az, MechS32 p_bx, MechS32 p_by, MechS32 p_bz);
	void TransformPoint(Matrix* p_matrix, MechS32* p_x, MechS32* p_y, MechS32* p_z);
	void RotatePoint(Matrix* p_matrix, MechS32* p_x, MechS32* p_y, MechS32* p_z);
	void MultiplyRotations(Matrix* p_a, Matrix* p_b, Matrix* p_dst);
	void MultiplyMatrix(Matrix* p_a, Matrix* p_b, Matrix* p_dst);
	void TransposeRotation(Matrix* p_src, Matrix* p_dst);
	void InvertMatrix(Matrix* p_src, Matrix* p_dst);
	void SetIdentityMatrix(Matrix* p_matrix);
	void CopyMatrix(Matrix* p_src, Matrix* p_dst);
	void CopyRotation(Matrix* p_src, Matrix* p_dst);
	void BuildMatrixEx(
		Matrix* p_matrix,
		MechS32 p_angleX,
		MechS32 p_angleY,
		MechS32 p_angleZ,
		MechS32 p_x,
		MechS32 p_y,
		MechS32 p_z,
		MechU32 p_flags
	);
	void BuildMatrix(
		Matrix* p_matrix,
		MechS32 p_angleX,
		MechS32 p_angleY,
		MechS32 p_angleZ,
		MechS32 p_x,
		MechS32 p_y,
		MechS32 p_z
	);
	void GetMatrixAngles(Matrix* p_matrix, undefined4* p_angleX, undefined4* p_angleY, undefined4* p_angleZ);

#ifdef __cplusplus
}
#endif

#endif // TRANSFORM_H
