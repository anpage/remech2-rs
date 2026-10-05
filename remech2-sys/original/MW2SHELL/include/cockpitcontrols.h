#ifndef COCKPITCONTROLS_H
#define COCKPITCONTROLS_H

#include "decomp.h"
#include "screenfield.h"
#include "types.h"

class LoopingMovie;

// One game control's binding in a configuration.
// SIZE 0x18
struct CpcBinding {
	MechS32 m_channelKind;  // 0x00 — 0: axis, 1: button, 2: axis with a button
	MechS32 m_deviceSlot;   // 0x04
	MechS32 m_flags;        // 0x08 — bits 0-2: modifier, bit 31: inverted
	MechS32 m_modeFlags;    // 0x0c — the flags of the axis's button
	MechS32 m_controlIndex; // 0x10 — the axis or button
	MechS32 m_mode;         // 0x14 — the button of an axis with a button
};

// A device slot of a .cpc file: the device a binding's slot number stood for when it was saved.
// SIZE 0x10
struct CpcDeviceSlot {
	MechS32 m_deviceId;    // 0x00
	MechChar m_name[0x0c]; // 0x04
};

enum CpcConfig {
	c_configCount = 4,
	c_bindingCount = 0x25,
	c_axisBindingCount = 7,
	c_buttonRows = 18,
	c_deviceSlotCount = 16,
	c_maxActiveDevices = 4
};

// The functions and globals of cockpitcontrols.cpp that other units use.
extern MechS32 g_cpcSelectedBinding;
extern MechS32 g_cpcSelectedPart;
extern MechS32 g_curInputDeviceIdx;
extern MechChar* g_cpcControlLabels[c_bindingCount];
extern MechChar* g_cpcSimControlNames[c_bindingCount];
extern MechS32 g_cpcConfigShown;
extern CpcBinding g_cpcBindings[c_configCount * c_bindingCount];
extern CpcBinding* g_cpcShownBindings;
extern MechS32 g_cpcFirstButton;
extern MechS32 g_curCpcConfigSlot;
extern MechS32 g_cpcConfigured;
extern MechS32 g_inputConfigChanged;
extern MechS32 g_cpcBindingsPage;
extern MechS32 g_activeInputDeviceCount;
extern MechS32 g_cpcLegsPanSlot;
extern MechChar* g_cpcConfigNames[c_configCount];
extern MechChar* g_cpcAxisDirections[4][2];
extern MechChar* g_cpcModifierNames[5];
extern MechChar* g_cpcSlotNames[5];
extern MechChar g_cpcConfigName[0x40];
extern MechChar g_cpcMessageText[0x400];
extern LoopingMovie* g_cpcLogoMovie;
extern MechChar g_cpcSlotText[0x40];
extern CpcBinding g_cpcDeviceFileBindings[c_configCount * c_bindingCount];
extern MechS32 g_inputDeviceActive[c_deviceSlotCount];
extern MechChar g_cpcFieldText[0x100];
extern CpcDeviceSlot g_cpcDeviceSlots[c_deviceSlotCount];
extern undefined g_cpcDisabledColors[0x100];
extern undefined g_cpcSelectedColors[0x100];
extern ScreenField g_cpcBindingsFields[169];
extern ScreenField g_cpcDevicesFields[24];
extern MechS32 g_cpcAnalogCount;
extern MechS32 g_cpcDiscreteCount;
extern MechChar g_cpcEditText[0x100];
void OpenCockpitControls();

#endif // COCKPITCONTROLS_H
