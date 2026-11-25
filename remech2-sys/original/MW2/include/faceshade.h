#ifndef FACESHADE_H
#define FACESHADE_H

#include "types.h"

struct Face;
struct Vertex;

// The functions and globals of faceshade.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_brightenDamage;
	extern int32_t g_ambientLight;

	uint32_t GetFaceColor(struct Face* p_face, struct Vertex* p_vertices, uint32_t p_color, int32_t p_distance);
	void ToggleTextureMaps(uint32_t p_flags);
	int32_t AreTextureMapsOn(uint32_t p_flags);
	void EnableTextureMaps(uint32_t p_flags, int32_t p_enable);
	int32_t ArePerspectiveTexturesOn(uint32_t p_unk0x00);
	void EnablePerspectiveTextures(uint32_t p_unk0x00, int32_t p_enable);

#ifdef __cplusplus
}
#endif

#endif // FACESHADE_H
