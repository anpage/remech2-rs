#ifndef INPUTMAP_H
#define INPUTMAP_H

#include "playersteering.h"
#include "types.h"

// The functions and globals of inputmap.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern PlayerSteering g_localSteering;
	extern MechS32 g_sinkPilotTilt;
	extern MechS32 g_sinkPilotPan;
	extern MechS32 g_sinkEyepointTilt;
	extern MechS32 g_sinkEyepointPanDelta;
	extern MechS32 g_sinkEyepointSlideDelta;
	extern MechS32 g_sinkTrackDistanceDelta;
	extern MechS32 g_sinkTrackHeightDelta;
	extern MechS32 g_sinkZoomFactor;
	extern MechS8 g_sinkPilotTiltPlus;
	extern MechS8 g_sinkPilotTiltMinus;
	extern MechS8 g_sinkPilotTiltReset;
	extern MechS8 g_sinkPilotPanPlus;
	extern MechS8 g_sinkPilotPanMinus;
	extern MechS8 g_sinkPilotPanReset;
	extern MechS8 g_sinkGlanceLeft;
	extern MechS8 g_sinkGlanceRight;
	extern MechS8 g_sinkGlanceUp;
	extern MechS8 g_sinkGlanceDown;
	extern MechS8 g_sinkEyepointTiltPlus;
	extern MechS8 g_sinkEyepointTiltMinus;
	extern MechS8 g_sinkEyepointTiltReset;
	extern MechS8 g_sinkEyepointPanPlus;
	extern MechS8 g_sinkEyepointPanMinus;
	extern MechS8 g_sinkEyepointPanReset;
	extern MechS8 g_sinkEyepointSlidePlus;
	extern MechS8 g_sinkEyepointSlideMinus;
	extern MechS8 g_sinkTrackDistancePlus;
	extern MechS8 g_sinkTrackDistanceMinus;
	extern MechS8 g_sinkTrackHeightPlus;
	extern MechS8 g_sinkTrackHeightMinus;
	extern MechS8 g_sinkZoomFactorPlus;
	extern MechS8 g_sinkZoomFactorMinus;
	extern MechS8 g_sinkZoomFactorReset;
	extern MechS32 g_sinkMenuItem;
	extern MechS32 g_sinkMenuValue;
	extern MechS8 g_sinkMenuItemReset;
	extern MechS8 g_sinkMenuValueReset;
	extern MechS8 g_sinkMenuEnter;
	extern MechS8 g_sinkMenuAbort;

	MechS32 RegisterInputDevice(MechChar* p_name);
	MechS32 FindInputDevice(MechChar* p_name);
	MechS32 FindInputAxis(MechS32 p_device, MechChar* p_name);
	MechS32 FindInputButton(MechS32 p_device, MechChar* p_name);
	MechS32 LoadInputMap(void);
	MechS32 LoadGamekeyMap(void);
	void FirstInputs(void);
	void UpdateInputs(void);
	void CloseInputDevices(void);
	void DisableGameplayInput(void);
	void EnableGameplayInput(void);
	MechS16 LookupGameKey(MechS16 p_keyCode);
	void ReportInputDeviceError(MechS32 p_code, MechChar* p_channel, MechChar* p_device);

#ifdef __cplusplus
}
#endif

#endif // INPUTMAP_H
