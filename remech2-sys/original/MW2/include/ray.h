#ifndef RAY_H
#define RAY_H

#include "types.h"

// A segment from (x0, y0, z0) to (x1, y1, z1) in 16.16 fixed point, with its delta and, once
// BuildRayFixed or BuildRayFloat has run, its unit direction and length.
typedef struct Ray {
	int32_t m_x0;     // 0x00
	int32_t m_y0;     // 0x04
	int32_t m_z0;     // 0x08
	int32_t m_x1;     // 0x0c
	int32_t m_y1;     // 0x10
	int32_t m_z1;     // 0x14
	int32_t m_dx;     // 0x18
	int32_t m_dy;     // 0x1c
	int32_t m_dz;     // 0x20
	int32_t m_dirX;   // 0x24
	int32_t m_dirY;   // 0x28
	int32_t m_dirZ;   // 0x2c
	int32_t m_length; // 0x30
	int32_t m_state;  // 0x34
} Ray;

// Ray::m_state: which of the builders computed the direction and length.
enum {
	c_rayNone = 0,
	c_rayFixed = 1,
	c_rayFloat = 2
};

// The functions and globals of ray.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void BuildRayFromSegment(
		Ray* p_ray,
		int32_t p_x0,
		int32_t p_y0,
		int32_t p_z0,
		int32_t p_x1,
		int32_t p_y1,
		int32_t p_z1
	);
	void BuildRayFromDirection(
		Ray* p_ray,
		int32_t p_x0,
		int32_t p_y0,
		int32_t p_z0,
		int32_t p_dirX,
		int32_t p_dirY,
		int32_t p_dirZ,
		int32_t p_length
	);
	int32_t GetRayLength(Ray* p_ray);
	void BuildRayFixed(Ray* p_ray);
	void BuildRayFloat(Ray* p_ray);
	void SetRayLength(Ray* p_ray, int32_t p_length);
	void AdvanceRayStart(Ray* p_ray, int32_t p_distance);
	void SetRayEnd(Ray* p_ray, int32_t p_x1, int32_t p_y1, int32_t p_z1);
	void ClipRayToGround(Ray* p_ray, int32_t p_y);
	void CopyRay(Ray* p_dst, Ray* p_src);
	int32_t ClipRaySlab(
		int32_t p_origin,
		int32_t p_delta,
		int32_t p_min,
		int32_t p_max,
		int32_t* p_tMin,
		int32_t* p_tMax
	);

#ifdef __cplusplus
}
#endif

#endif // RAY_H
