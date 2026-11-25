#ifndef INPUTMAP_H
#define INPUTMAP_H

#include "inputsink.h"
#include "playersteering.h"
#include "types.h"

// The functions and globals of inputmap.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern PlayerSteering g_localSteering;
	extern int32_t g_sinkPilotTilt;
	extern int32_t g_sinkPilotPan;
	extern int32_t g_sinkEyepointTilt;
	extern int32_t g_sinkEyepointPanDelta;
	extern int32_t g_sinkEyepointSlideDelta;
	extern int32_t g_sinkTrackDistanceDelta;
	extern int32_t g_sinkTrackHeightDelta;
	extern int32_t g_sinkZoomFactor;
	extern int8_t g_sinkPilotTiltPlus;
	extern int8_t g_sinkPilotTiltMinus;
	extern int8_t g_sinkPilotTiltReset;
	extern int8_t g_sinkPilotPanPlus;
	extern int8_t g_sinkPilotPanMinus;
	extern int8_t g_sinkPilotPanReset;
	extern int8_t g_sinkGlanceLeft;
	extern int8_t g_sinkGlanceRight;
	extern int8_t g_sinkGlanceUp;
	extern int8_t g_sinkGlanceDown;
	extern int8_t g_sinkEyepointTiltPlus;
	extern int8_t g_sinkEyepointTiltMinus;
	extern int8_t g_sinkEyepointTiltReset;
	extern int8_t g_sinkEyepointPanPlus;
	extern int8_t g_sinkEyepointPanMinus;
	extern int8_t g_sinkEyepointPanReset;
	extern int8_t g_sinkEyepointSlidePlus;
	extern int8_t g_sinkEyepointSlideMinus;
	extern int8_t g_sinkTrackDistancePlus;
	extern int8_t g_sinkTrackDistanceMinus;
	extern int8_t g_sinkTrackHeightPlus;
	extern int8_t g_sinkTrackHeightMinus;
	extern int8_t g_sinkZoomFactorPlus;
	extern int8_t g_sinkZoomFactorMinus;
	extern int8_t g_sinkZoomFactorReset;
	extern int32_t g_sinkMenuItem;
	extern int32_t g_sinkMenuValue;
	extern int8_t g_sinkMenuItemReset;
	extern int8_t g_sinkMenuValueReset;
	extern int8_t g_sinkMenuEnter;
	extern int8_t g_sinkMenuAbort;
	extern int32_t g_gameplayInputEnabled;
	extern InputSink g_inputSinks[87];

	void DisableGameplayInput(void);
	void EnableGameplayInput(void);

	// Implemented on the Rust side (src/sim/input.rs)
	void FirstInputs(void);
	void UpdateInputs(void);
	void CloseInputDevices(void);
	int16_t LookupGameKey(int16_t p_keyCode);
	// The key code the bindings give the open menu this frame, or 0
	int16_t TakeBoundMenuKey(void);

#ifdef __cplusplus
}
#endif

#endif // INPUTMAP_H
