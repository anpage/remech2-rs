#ifndef FACESHADE_H
#define FACESHADE_H

#include "decomp.h"
#include "types.h"

struct Face;
struct Vertex;

// The functions and globals of faceshade.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_brightenDamage;
	extern MechS32 g_ambientLight;

	MechU32 GetFaceColor(struct Face* p_face, struct Vertex* p_vertices, MechU32 p_color, MechS32 p_distance);
	void ToggleTextureMaps(MechU32 p_flags);
	MechS32 AreTextureMapsOn(MechU32 p_flags);
	void EnableTextureMaps(MechU32 p_flags, MechS32 p_enable);
	MechS32 ArePerspectiveTexturesOn(undefined4 p_unk0x00);
	void EnablePerspectiveTextures(undefined4 p_unk0x00, MechS32 p_enable);

#ifdef __cplusplus
}
#endif

#endif // FACESHADE_H
