#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "types.h"

/* A 2.29 fixed-point transform: three rotation rows and a translation row
   (SetIdentityMatrix sets the identity, 0x20000000 on the diagonal). */
typedef struct Matrix Matrix;

struct Matrix {
	int32_t m_rows[4][3]; // 0x00
};

// The functions and globals of transform.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t MatrixMul29(int32_t p_a, int32_t p_b);
	int32_t MatrixDot29(int32_t p_ax, int32_t p_ay, int32_t p_az, int32_t p_bx, int32_t p_by, int32_t p_bz);
	void TransformPoint(Matrix* p_matrix, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	void RotatePoint(Matrix* p_matrix, int32_t* p_x, int32_t* p_y, int32_t* p_z);
	void MultiplyRotations(Matrix* p_a, Matrix* p_b, Matrix* p_dst);
	void MultiplyMatrix(Matrix* p_a, Matrix* p_b, Matrix* p_dst);
	void TransposeRotation(Matrix* p_src, Matrix* p_dst);
	void InvertMatrix(Matrix* p_src, Matrix* p_dst);
	void SetIdentityMatrix(Matrix* p_matrix);
	void CopyMatrix(Matrix* p_src, Matrix* p_dst);
	void CopyRotation(Matrix* p_src, Matrix* p_dst);
	void BuildMatrixEx(
		Matrix* p_matrix,
		int32_t p_angleX,
		int32_t p_angleY,
		int32_t p_angleZ,
		int32_t p_x,
		int32_t p_y,
		int32_t p_z,
		uint32_t p_flags
	);
	void BuildMatrix(
		Matrix* p_matrix,
		int32_t p_angleX,
		int32_t p_angleY,
		int32_t p_angleZ,
		int32_t p_x,
		int32_t p_y,
		int32_t p_z
	);
	void GetMatrixAngles(Matrix* p_matrix, int32_t* p_angleX, int32_t* p_angleY, int32_t* p_angleZ);

#ifdef __cplusplus
}
#endif

#endif // TRANSFORM_H
