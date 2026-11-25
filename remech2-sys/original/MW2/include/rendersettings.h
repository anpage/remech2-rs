#ifndef RENDERSETTINGS_H
#define RENDERSETTINGS_H

#include "types.h"

struct Face;
struct ProjectedVertex;
struct Shape;
struct Vertex;

// Rendering settings (g_renderSettings) the map view saves and replaces as one block.
typedef struct RenderSettings {
	int32_t m_blankScene;      // the frame draw callback only clears the pane
	int32_t m_gouraud;         // DrawScenePolygon blends shaded polygons (VFX_dithered_Gouraud_polygon)
	int32_t m_textures;        // DrawScenePolygon draws textured polygons
	uint32_t m_flags;          // 1: bands are filled (DrawBandPolygon); FirstRender sets 8
	int32_t m_drawLines;       // two-point polygons are drawn as lines
	int32_t m_drawPixels;      // one-point polygons are drawn as pixels
	int32_t m_drawSky;         // the planet's sky (DrawSkyAndGround)
	int32_t m_drawGround;      // and its ground
	int32_t m_horizonBand;     // a shaded band blends the sky into the ground
	int32_t m_clearFrame;      // clears the pane each frame; FirstRender clears it when
							   // the sky or the ground covers the view
	int32_t m_wireframe;       // 0 fills polygons, 1 fills and outlines (ENHANCED_VISION), else outlines
							   // (TOGGLE_WIREFRAME)
	int32_t m_wireframeColors; // in wireframe, colors faces by the shape's kind (0),
							   // collision type (1) or flags (2) (COLLISION_WIREFRAME)
	int32_t m_distanceFade;    // set with a fade distance, cleared while an effect lights
							   // the scene
	uint32_t m_greyscale;
	int32_t m_fadeDistance;     // shades dim with the distance over it (ComputeShade)
	int32_t m_affineTextures;   // textures without perspective correction (low display
								// detail)
	uint32_t m_untexturedKinds; // the shape kinds (0x100 game pieces, 0x200 game things,
								// 0x800 terrain...) drawn without their texture maps
	void (*m_frameDrawCallback)(void);
	int32_t (*m_shapeFilter)(struct Shape* p_shape); // a shape filter: nonzero skips the shape
	struct ProjectedVertex* (*m_projectVertex)(struct ProjectedVertex* p_vertex); // projects a vertex
	// Draws a face (GetFaceColor)
	uint32_t (*m_drawFace)(struct Face* p_face, struct Vertex* p_vertices, uint32_t p_color, int32_t p_distance);
	void (*m_drawPolygon)(int32_t p_count, uint32_t* p_points, uint32_t p_flags);
} RenderSettings;

#endif // RENDERSETTINGS_H
