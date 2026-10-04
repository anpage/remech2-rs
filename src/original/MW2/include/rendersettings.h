#ifndef RENDERSETTINGS_H
#define RENDERSETTINGS_H

#include "decomp.h"
#include "types.h"

struct ProjectedVertex;

// SIZE 0x68
// Rendering settings (g_renderSettings) the map view saves and replaces as one block.
typedef struct RenderSettings {
	MechS32 m_blankScene;  // 0x00 — the frame draw callback only clears the pane
	MechS32 m_gouraud;     // 0x04 — DrawScenePolygon blends shaded polygons (VFX_dithered_Gouraud_polygon)
	undefined4 m_unk0x08;  // 0x08
	MechS32 m_textures;    // 0x0c — DrawScenePolygon draws textured polygons
	undefined4 m_flags;    // 0x10 — 1: bands are filled (DrawBandPolygon); FirstRender sets 8
	MechS32 m_drawLines;   // 0x14 — two-point polygons are drawn as lines
	MechS32 m_drawPixels;  // 0x18 — one-point polygons are drawn as pixels
	MechS32 m_drawSky;     // 0x1c — the planet's sky (DrawSkyAndGround)
	MechS32 m_drawGround;  // 0x20 — and its ground
	MechS32 m_horizonBand; // 0x24 — a shaded band blends the sky into the ground
	undefined4 m_unk0x28[(0x30 - 0x28) / 4]; // 0x28
	MechS32 m_clearFrame;                    // 0x30 — clears the pane each frame; FirstRender clears it when
											 // the sky or the ground covers the view
	MechS32 m_wireframe;               // 0x34 — 0 fills polygons, 1 fills and outlines (ENHANCED_VISION), else outlines
									   // (TOGGLE_WIREFRAME)
	MechS32 m_wireframeColors;         // 0x38 — in wireframe, colors faces by the shape's kind (0),
									   // collision type (1) or flags (2) (COLLISION_WIREFRAME)
	MechS32 m_distanceFade;            // 0x3c — set with a fade distance, cleared while an effect lights
									   // the scene
	undefined4 m_greyscale;            // 0x40
	MechS32 m_fadeDistance;            // 0x44 — shades dim with the distance over it (ComputeShade)
	undefined4 m_unk0x48;              // 0x48
	MechS32 m_affineTextures;          // 0x4c — textures without perspective correction (low display
									   // detail)
	MechU32 m_untexturedKinds;         // 0x50 — the shape kinds (0x100 game pieces, 0x200 game things,
									   // 0x800 terrain...) drawn without their texture maps
	void (*m_frameDrawCallback)(void); // 0x54
	MechS32 (*m_shapeFilter)();        // 0x58 — a shape filter: nonzero skips the shape
	struct ProjectedVertex* (*m_projectVertex)(struct ProjectedVertex* p_vertex); // 0x5c — projects a vertex
	MechS32 (*m_drawFace)();                                                      // 0x60 — draws a face (GetFaceColor)
	void (*m_drawPolygon)(MechS32 p_count, MechU32* p_points, MechU32 p_flags);   // 0x64
} RenderSettings;

#endif // RENDERSETTINGS_H
