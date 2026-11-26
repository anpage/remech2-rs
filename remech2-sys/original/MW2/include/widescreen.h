#ifndef WIDESCREEN_H
#define WIDESCREEN_H

// Widescreen, implemented on the Rust side (src/sim/window.rs): with video.widescreen the frame is
// 16:9, the HUD and cockpit stay in a 4:3 box centred in it, and the 3D views cover the whole
// frame behind them. In 4:3 these do nothing.

#include "cockpit.h"
#include "types.h"
#include "window.h"

#ifdef __cplusplus
extern "C"
{
#endif

	// Widens the satellite view's viewport and pane (cockpit view 4) to the whole frame
	void MechWidenCockpitLayout(int32_t p_cockpit, CockpitLayout* p_layout);
	// The world span the map view shows across pane p_slot: the satellite view's widens with it
	int32_t MechWidenMapSpan(int32_t p_slot, int32_t p_span);
	// Narrows p_pane to the HUD box
	void MechHudPane(PANE* p_pane);
	// Centers a message box pane horizontally in the frame
	void MechCenterMessagePane(PANE* p_pane);
	// Widens p_pane (the main 3D view) to the whole frame
	void MechWidenViewPane(PANE* p_pane);
	// Maps p_rect from 16.16 fractions to pixels of the 3D view. The frame in widescreen, otherwise as
	// ScaleRectToScreen
	void MechScaleRectToView(PANE* p_rect);

#ifdef __cplusplus
}
#endif

#endif // WIDESCREEN_H
