#ifndef RAY_H
#define RAY_H

#include "decomp.h"
#include "types.h"

// A segment from (x0, y0, z0) to (x1, y1, z1) in 16.16 fixed point, with its delta and, once
// BuildRayFixed or BuildRayFloat has run, its unit direction and length.
// SIZE 0x38
typedef struct Ray {
	MechS32 m_x0;     // 0x00
	MechS32 m_y0;     // 0x04
	MechS32 m_z0;     // 0x08
	MechS32 m_x1;     // 0x0c
	MechS32 m_y1;     // 0x10
	MechS32 m_z1;     // 0x14
	MechS32 m_dx;     // 0x18
	MechS32 m_dy;     // 0x1c
	MechS32 m_dz;     // 0x20
	MechS32 m_dirX;   // 0x24
	MechS32 m_dirY;   // 0x28
	MechS32 m_dirZ;   // 0x2c
	MechS32 m_length; // 0x30
	MechS32 m_state;  // 0x34
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
		MechS32 p_x0,
		MechS32 p_y0,
		MechS32 p_z0,
		MechS32 p_x1,
		MechS32 p_y1,
		MechS32 p_z1
	);
	void BuildRayFromDirection(
		Ray* p_ray,
		MechS32 p_x0,
		MechS32 p_y0,
		MechS32 p_z0,
		MechS32 p_dirX,
		MechS32 p_dirY,
		MechS32 p_dirZ,
		MechS32 p_length
	);
	MechS32 GetRayLength(Ray* p_ray);
	void BuildRayFixed(Ray* p_ray);
	void BuildRayFloat(Ray* p_ray);
	void SetRayLength(Ray* p_ray, MechS32 p_length);
	void AdvanceRayStart(Ray* p_ray, MechS32 p_distance);
	void SetRayEnd(Ray* p_ray, MechS32 p_x1, MechS32 p_y1, MechS32 p_z1);
	void ClipRayToGround(Ray* p_ray, MechS32 p_y);
	void CopyRay(Ray* p_dst, Ray* p_src);
	MechS32 ClipRaySlab(
		MechS32 p_origin,
		MechS32 p_delta,
		MechS32 p_min,
		MechS32 p_max,
		MechS32* p_tMin,
		MechS32* p_tMax
	);

#ifdef __cplusplus
}
#endif

#endif // RAY_H
