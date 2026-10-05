#include "cockpitcontrols.h"

#include "debugprint.h"
#include "decomp.h"
#include "files.h"
#include "font.h"
#include "input.h"
#include "inputdevice.h"
#include "keyboardinput.h"
#include "log.h"
#include "loopingmovie.h"
#include "mechbay.h"
#include "mousestate.h"
#include "options.h"
#include "refreshmode.h"
#include "screenfield.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "textglyph.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

// The cockpit controls screen: four configurations of bindings from the game's controls to
// device axes and buttons, and the fields that show and edit them. The configurations are
// saved as .cpc files (giddi\configNN.cpc, and one per device) and written out as the sim's
// input.map.

DECOMP_SIZE_ASSERT(CpcBinding, 0x18)
DECOMP_SIZE_ASSERT(CpcDeviceSlot, 0x10)

enum CpcBindingFlags {
	c_flagModifierMask = 0x07
};

// CpcShowDeviceMessage's messages.
enum CpcMessage {
	c_messageMissingJoystick = 100,
	c_messageRemapJoystick = 101,
	c_messageNoJoysticks = 102
};

// Not an enumerator: the unsigned constant makes `^=` on the signed flags a load, xor and store.
#define CPC_FLAG_INVERTED 0x80000000

void CpcScreenTick(MechS32 p_active);

TextGlyph* CpcDrawText(ScreenField* p_tab);
MechS32 CpcEditTextField(
	Font* p_font,
	MechS32 p_left,
	MechS32 p_top,
	MechChar* p_text,
	undefined* p_colors,
	MechS32 p_maxLength,
	MechS32 p_maxWidth
);
void CpcEditField(ScreenField* p_tab);
TextGlyph* CpcDrawBinding(ScreenField* p_tab);
TextGlyph* CpcDrawAxisButton(ScreenField* p_tab);
TextGlyph* CpcDrawControlName(ScreenField* p_tab);
TextGlyph* CpcDrawConfigName(ScreenField* p_tab);
void CpcNextConfig(ScreenField* p_tab);
TextGlyph* CpcDrawAxisDirection(ScreenField* p_tab);
void CpcToggleInverted(ScreenField* p_tab);
TextGlyph* CpcDrawModifier(ScreenField* p_tab);
void CpcCycleModifier(ScreenField* p_tab);
void CpcAbort(ScreenField* p_tab);
void CpcClickBinding(ScreenField* p_tab);
void CpcClickAxisButton(ScreenField* p_tab);
TextGlyph* CpcDrawDeviceAxis(ScreenField* p_tab);
TextGlyph* CpcDrawDeviceButton(ScreenField* p_tab);
void CpcBindAxis(ScreenField* p_tab);
TextGlyph* CpcDrawScrollArrow(ScreenField* p_tab);
void CpcScrollButtons(ScreenField* p_tab);
void CpcBindButton(ScreenField* p_tab);
TextGlyph* CpcDrawDeviceName(ScreenField* p_tab);
TextGlyph* CpcDrawBindingDevice(ScreenField* p_tab);
TextGlyph* CpcDrawDeviceEntry(ScreenField* p_tab);
void InputToggleDeviceActive(ScreenField* p_tab);
void CpcSelectDevice(ScreenField* p_tab);
TextGlyph* CpcDrawDivider(ScreenField* p_tab);
TextGlyph* CpcDrawSlotName(ScreenField* p_tab);
void CpcAcceptDevices(ScreenField* p_tab);
TextGlyph* CpcDrawNoJoysticks(ScreenField* p_tab);
void CpcCheckJoysticks(ScreenField* p_tab);
void CpcRefreshDevices(ScreenField* p_tab);
void CpcShowBindingsPage(ScreenField* p_tab);
void CpcOpenJoystickControlPanel(ScreenField* p_tab);
MechS32 CpcCheckControlCount();
void CpcRemapDeviceSlots(CpcBinding* p_bindings);
void CpcResetBinding(MechS32 p_index, CpcBinding* p_binding);
void CpcLoadConfigSlot(ScreenField* p_tab);
void CpcSaveConfigSlot(ScreenField* p_tab);
void CpcLoadActiveDevices(ScreenField* p_tab);
void CpcAcceptAndCommit(ScreenField* p_tab);
void CpcShowDeviceMessage(MechS32 p_message, MechChar* p_unk0x04, MechChar* p_name);

// The binding selected for editing, -1 for none.
// GLOBAL: MW2SHELL 0x1006af18
MechS32 g_cpcSelectedBinding = -1;

// Which part of the selected binding is edited: 0 the axis or button, 1 the axis's button.
// GLOBAL: MW2SHELL 0x1006af1c
MechS32 g_cpcSelectedPart = 0;

// GLOBAL: MW2SHELL 0x1006af20
MechS32 g_curInputDeviceIdx = -1;

// GLOBAL: MW2SHELL 0x1006af28
MechChar* g_cpcControlLabels[c_bindingCount] = {
	"Throttle ",      "Chassis ",        "Turret Turn",    "Turret Tilt",     "Eye Point ",      "Eye Point ",
	"Eye Point ",     "Recenter Torso",  "Glance Left",    "Glance Right",    "Glance Up",       "Glance Down",
	"Jump Jets On",   "Jump Jets Front", "Jump Jets Back", "Jump Jets Left",  "Jump Jets Right", "Fire Weapon",
	"Cycle Weapon",   "Cycle Group",     "Fire Group 1",   "Fire Group 2",    "Fire Group 3",    "Group Toggle",
	"Target Next",    "Target Prev",     "Target Reticle", "Target Friendly", "Nearest Enemy",   "Inspect Target",
	"Next Nav Point",
};

// The sim's name of each control, for input.map.
// GLOBAL: MW2SHELL 0x1006afc0
MechChar* g_cpcSimControlNames[c_bindingCount] = {
	"throttle",
	"legs_pan_delta",
	"torso_pan",
	"torso_tilt",
	"pilot_pan",
	"pilot_tilt",
	"zoom_factor",
	"torso_tilt_reset",
	"glance_left",
	"glance_right",
	"glance_up",
	"glance_down",
	"jumpjet_enabled",
	"jumpjet_fire_forward",
	"jumpjet_fire_backward",
	"jumpjet_fire_left",
	"jumpjet_fire_right",
	"weapon_fire",
	"weapon_cycle",
	"weapon_cycle_group",
	"weapon_fire_group_1",
	"weapon_fire_group_2",
	"weapon_fire_group_3",
	"toggle_group_fire",
	"advance_target",
	"previous_target",
	"target_reticle",
	"target_friendly",
	"nearest_enemy",
	"inspect_target",
	"advance_nav",
};

// The configuration shown.
// GLOBAL: MW2SHELL 0x1006b054
MechS32 g_cpcConfigShown = 0;

// The bindings of the four configurations, one after the other.
// GLOBAL: MW2SHELL 0x10090ad0
CpcBinding g_cpcBindings[c_configCount * c_bindingCount];

// GLOBAL: MW2SHELL 0x1006b058
CpcBinding* g_cpcShownBindings = g_cpcBindings;

// The first button shown in the device's button list.
// GLOBAL: MW2SHELL 0x1006b05c
MechS32 g_cpcFirstButton = 0;

// GLOBAL: MW2SHELL 0x1006b060
MechS32 g_curCpcConfigSlot = 1;

// GLOBAL: MW2SHELL 0x1006b064
MechS32 g_cpcConfigured = 0;

// GLOBAL: MW2SHELL 0x1006b068
MechS32 g_inputConfigChanged = 0;

// GLOBAL: MW2SHELL 0x1006b06c
MechS32 g_cpcBindingsPage = 0;

// GLOBAL: MW2SHELL 0x1006b070
MechS32 g_activeInputDeviceCount = 0;

// The device slot of the bindings CpcCheckControlCount adds for the legs' pan.
// GLOBAL: MW2SHELL 0x1006b074
MechS32 g_cpcLegsPanSlot = -1;

// GLOBAL: MW2SHELL 0x1006b078
MechChar* g_cpcConfigNames[c_configCount] =
	{"Primary Controls", "Secondary Controls", "Tertiary Controls", "Quaternary Controls"};

// Axis directions, normal and inverted, per kind of axis.
// GLOBAL: MW2SHELL 0x1006b088
MechChar* g_cpcAxisDirections[4][2] = {{"-/+", "+/-"}, {"L/R", "R/L"}, {"D/U", "U/D"}, {"I/O", "O/I"}};

// Modifier names, by the modifier bits of a binding.
// GLOBAL: MW2SHELL 0x1006b0a8
MechChar* g_cpcModifierNames[5] = {"~--", "~Ctrl", "~Alt", "~--", "~Shft"};

// GLOBAL: MW2SHELL 0x1006b0c0
MechChar* g_cpcSlotNames[5] = {"Default", "Custom 1", "Custom 2", "Custom 3", "Custom 4"};

// GLOBAL: MW2SHELL 0x10090690
MechChar g_cpcConfigName[0x40];

// The text of CpcShowDeviceMessage's message box.
// GLOBAL: MW2SHELL 0x100906d0
MechChar g_cpcMessageText[0x400];

// GLOBAL: MW2SHELL 0x100918b0
LoopingMovie* g_cpcLogoMovie;

// CpcSaveConfigSlot's file name, then its "Saved" dialog.
// GLOBAL: MW2SHELL 0x100918b8
MechChar g_cpcSlotText[0x40];

// The bindings of one device's .cpc file.
// GLOBAL: MW2SHELL 0x100918f8
CpcBinding g_cpcDeviceFileBindings[c_configCount * c_bindingCount];

// GLOBAL: MW2SHELL 0x100927d8
MechS32 g_inputDeviceActive[c_deviceSlotCount];

// GLOBAL: MW2SHELL 0x10092818
MechChar g_cpcFieldText[0x100];

// GLOBAL: MW2SHELL 0x10092918
CpcDeviceSlot g_cpcDeviceSlots[c_deviceSlotCount];

// Color map for disabled fields.
// GLOBAL: MW2SHELL 0x10092a18
undefined g_cpcDisabledColors[0x100];

// Color map for the selected field.
// GLOBAL: MW2SHELL 0x10092b18
undefined g_cpcSelectedColors[0x100];

// A field's m_top: a packed row and offset below the previous field (see ScreenField).
#define CPC_ROW(row, offset) ((MechS32) (0x80000000 | ((row) << 4) | (offset)))
#define CPC_TAB(left, top, width, draw, click, data, next)                                                             \
	{left, top, width, -1, 0, NULL, NULL, draw, click, (void*) (data), (ScreenField*) (next)}
#define CPC_TEXT(left, top, width, draw, click, data)                                                                  \
	{left, top, width, -1, 0, g_cpcDisabledColors, NULL, draw, click, (void*) (data), NULL}
#define CPC_END {-1, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL}

// One control bound to an axis: its name, direction, modifier, axis and button. kind picks the
// direction names.
#define CPC_AXIS_BINDING(top, index, kind)                                                                             \
	CPC_TEXT(175, top, -1, CpcDrawControlName, NULL, index),                                                           \
		CPC_TAB(225, CPC_ROW(0, 0), -1, CpcDrawAxisDirection, CpcToggleInverted, index, kind),                         \
		CPC_TAB(260, CPC_ROW(0, 0), 20, CpcDrawModifier, CpcCycleModifier, index, 0),                                  \
		CPC_TAB(285, CPC_ROW(0, 0), 100, CpcDrawBinding, CpcClickBinding, index, 0),                                   \
		CPC_TAB(345, CPC_ROW(0, 0), 50, CpcDrawAxisButton, CpcClickAxisButton, index, 0)

// One control bound to a button: its name, modifier and button.
#define CPC_BUTTON_BINDING(index)                                                                                      \
	CPC_TEXT(175, CPC_ROW(1, 1), -1, CpcDrawControlName, NULL, index),                                                 \
		CPC_TAB(260, CPC_ROW(0, 0), 20, CpcDrawModifier, CpcCycleModifier, index, 0),                                  \
		CPC_TAB(285, CPC_ROW(0, 0), 100, CpcDrawBinding, CpcClickBinding, index, 0)

// The fields of one of the device's axes or buttons, or one input device.
#define CPC_LIST(top, draw, click, index) CPC_TAB(480, top, 100, draw, click, index, 0)
#define CPC_DEVICE(left, top, draw, click, index) CPC_TAB(left, top, 100, draw, click, index, 0)

// The bindings screen.
// GLOBAL: MW2SHELL 0x1006b0d8
ScreenField g_cpcBindingsFields[169] = {
	CPC_TEXT(172, 0x78, -1, CpcDrawDivider, NULL, 1),
	CPC_TEXT(477, 0x78, -1, CpcDrawDivider, NULL, 1),
	CPC_TEXT(175, 0x6e, -1, CpcDrawText, NULL, "GAME CONTROLS"),
	CPC_TAB(325, CPC_ROW(0, 0), -1, CpcDrawConfigName, CpcNextConfig, 0, 0),
	CPC_AXIS_BINDING(CPC_ROW(2, 2), 0, 0),
	CPC_AXIS_BINDING(CPC_ROW(1, 1), 1, 1),
	CPC_AXIS_BINDING(CPC_ROW(1, 1), 2, 1),
	CPC_AXIS_BINDING(CPC_ROW(1, 1), 3, 2),
	CPC_AXIS_BINDING(CPC_ROW(1, 1), 4, 1),
	CPC_AXIS_BINDING(CPC_ROW(1, 1), 5, 2),
	CPC_AXIS_BINDING(CPC_ROW(1, 1), 6, 3),
	CPC_BUTTON_BINDING(7),
	CPC_BUTTON_BINDING(8),
	CPC_BUTTON_BINDING(9),
	CPC_BUTTON_BINDING(10),
	CPC_BUTTON_BINDING(11),
	CPC_BUTTON_BINDING(12),
	CPC_BUTTON_BINDING(13),
	CPC_BUTTON_BINDING(14),
	CPC_BUTTON_BINDING(15),
	CPC_BUTTON_BINDING(16),
	CPC_BUTTON_BINDING(17),
	CPC_BUTTON_BINDING(18),
	CPC_BUTTON_BINDING(19),
	CPC_BUTTON_BINDING(20),
	CPC_BUTTON_BINDING(21),
	CPC_BUTTON_BINDING(22),
	CPC_BUTTON_BINDING(23),
	CPC_BUTTON_BINDING(24),
	CPC_BUTTON_BINDING(25),
	CPC_BUTTON_BINDING(26),
	CPC_BUTTON_BINDING(27),
	CPC_BUTTON_BINDING(28),
	CPC_BUTTON_BINDING(29),
	CPC_BUTTON_BINDING(30),
	CPC_TEXT(480, 0x6e, -1, CpcDrawDeviceName, NULL, -1),
	CPC_TEXT(480, CPC_ROW(2, 2), -1, CpcDrawText, NULL, "Directional"),
	CPC_LIST(CPC_ROW(2, 2), CpcDrawDeviceAxis, CpcBindAxis, 0),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceAxis, CpcBindAxis, 1),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceAxis, CpcBindAxis, 2),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceAxis, CpcBindAxis, 3),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceAxis, CpcBindAxis, 4),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceAxis, CpcBindAxis, 5),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceAxis, CpcBindAxis, 6),
	CPC_TEXT(480, CPC_ROW(2, 2), -1, CpcDrawText, NULL, "Buttons"),
	CPC_TAB(600, CPC_ROW(2, 2), 10, CpcDrawScrollArrow, CpcScrollButtons, -1, 0),
	CPC_LIST(CPC_ROW(0, 0), CpcDrawDeviceButton, CpcBindButton, 0),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 1),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 2),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 3),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 4),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 5),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 6),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 7),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 8),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 9),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 10),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 11),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 12),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 13),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 14),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 15),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 16),
	CPC_LIST(CPC_ROW(1, 1), CpcDrawDeviceButton, CpcBindButton, 17),
	CPC_TAB(600, CPC_ROW(0, 0), 10, CpcDrawScrollArrow, CpcScrollButtons, 1, 0),
	CPC_TEXT(0, 0x6e, -1, CpcDrawText, NULL, "INPUT DEVICES"),
	CPC_DEVICE(0, CPC_ROW(2, 2), CpcDrawBindingDevice, CpcSelectDevice, 0),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 1),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 2),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 3),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 4),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 5),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 6),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 7),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 8),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 9),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 10),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawBindingDevice, CpcSelectDevice, 11),
	CPC_TEXT(0, CPC_ROW(2, 2), -1, CpcDrawText, NULL, "Current Config:"),
	CPC_TEXT(90, CPC_ROW(0, 0), -1, CpcDrawSlotName, NULL, &g_curCpcConfigSlot),
	CPC_TAB(20, CPC_ROW(1, 1), 150, CpcDrawText, CpcEditField, g_cpcConfigName, 0),
	CPC_TAB(0, CPC_ROW(2, 2), -1, CpcDrawText, CpcLoadActiveDevices, "RESET DEFAULTS", 0),
	CPC_TAB(0, CPC_ROW(2, 2), -1, CpcDrawText, CpcLoadConfigSlot, "Load Custom 1", 1),
	CPC_TAB(0, CPC_ROW(1, 1), -1, CpcDrawText, CpcLoadConfigSlot, "Load Custom 2", 2),
	CPC_TAB(0, CPC_ROW(1, 1), -1, CpcDrawText, CpcLoadConfigSlot, "Load Custom 3", 3),
	CPC_TAB(0, CPC_ROW(1, 1), -1, CpcDrawText, CpcLoadConfigSlot, "Load Custom 4", 4),
	CPC_TAB(0, CPC_ROW(2, 2), -1, CpcDrawText, CpcSaveConfigSlot, "Save Custom 1", 1),
	CPC_TAB(0, CPC_ROW(1, 1), -1, CpcDrawText, CpcSaveConfigSlot, "Save Custom 2", 2),
	CPC_TAB(0, CPC_ROW(1, 1), -1, CpcDrawText, CpcSaveConfigSlot, "Save Custom 3", 3),
	CPC_TAB(0, CPC_ROW(1, 1), -1, CpcDrawText, CpcSaveConfigSlot, "Save Custom 4", 4),
	CPC_TAB(0, CPC_ROW(2, 2), -1, CpcDrawText, CpcAcceptAndCommit, "ACCEPT CONFIG AND EXIT", 0),
	CPC_TAB(0, CPC_ROW(2, 2), -1, CpcDrawText, CpcAbort, "ABORT", 0),
	CPC_END,
};

// The input devices screen.
// GLOBAL: MW2SHELL 0x1006cde8
ScreenField g_cpcDevicesFields[24] = {
	CPC_TEXT(172, 0x78, -1, CpcDrawDivider, NULL, 1),
	CPC_TEXT(477, 0x78, -1, CpcDrawDivider, NULL, 1),
	CPC_TEXT(0, 0x6e, -1, CpcDrawText, NULL, "Current Config:"),
	CPC_TEXT(80, CPC_ROW(0, 0), -1, CpcDrawText, NULL, g_cpcConfigName),
	CPC_TEXT(0, CPC_ROW(2, 2), -1, CpcDrawText, NULL, "SELECT INPUT DEVICES"),
	CPC_DEVICE(0, CPC_ROW(2, 2), CpcDrawDeviceEntry, InputToggleDeviceActive, 0),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 1),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 2),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 3),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 4),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 5),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 6),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 7),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 8),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 9),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 10),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawDeviceEntry, InputToggleDeviceActive, 11),
	CPC_DEVICE(0, CPC_ROW(1, 1), CpcDrawNoJoysticks, CpcCheckJoysticks, "No joysticks configured"),
	CPC_DEVICE(0, CPC_ROW(2, 2), CpcDrawText, CpcOpenJoystickControlPanel, "JOYSTICK CONTROL PANEL"),
	CPC_DEVICE(0, CPC_ROW(2, 2), CpcDrawText, CpcRefreshDevices, "REFRESH"),
	CPC_DEVICE(0, 0x190, CpcDrawText, CpcAcceptDevices, "ACCEPT"),
	CPC_DEVICE(0, 0x1a4, CpcDrawText, CpcShowBindingsPage, "CUSTOM CONFIGURATION"),
	CPC_DEVICE(0, 0x1b8, CpcDrawText, CpcAbort, "ABORT"),
	CPC_END,
};

#undef CPC_ROW
#undef CPC_TAB
#undef CPC_TEXT
#undef CPC_END
#undef CPC_AXIS_BINDING
#undef CPC_BUTTON_BINDING
#undef CPC_LIST
#undef CPC_DEVICE

// The axes and buttons written to the last input.map.
// GLOBAL: MW2SHELL 0x1006d208
MechS32 g_cpcAnalogCount = 0;

// GLOBAL: MW2SHELL 0x1006d20c
MechS32 g_cpcDiscreteCount = 0;

// FUNCTION: MW2SHELL 0x1003e9d0
TextGlyph* CpcDrawText(ScreenField* p_tab)
{
	MechChar* text;

	text = (MechChar*) p_tab->m_data;
	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, text, p_tab->m_colors);
}

// The text being edited by CpcEditTextField, with its cursor.
// GLOBAL: MW2SHELL 0x100926d8
MechChar g_cpcEditText[0x100];

// Edits p_text with a cursor ('_') drawn after it, like EditTextField but keeping the screen's
// video running. Return or a click store the text and return 1; Escape stores it and returns
// 0. When the window closes it gives up, returning 0 (the original returned nothing).
// Not 100%: the stack slots of the locals are permuted.
// FUNCTION: MW2SHELL 0x1003ea0f
MechS32 CpcEditTextField(
	Font* p_font,
	MechS32 p_left,
	MechS32 p_top,
	MechChar* p_text,
	undefined* p_colors,
	MechS32 p_maxLength,
	MechS32 p_maxWidth
)
{
	MechS32 key;
	MechS32 width;
	MechS32 length;
	TextGlyph* glyph;

	glyph = NULL;
	length = strlen(p_text);
	strcpy(g_cpcEditText, p_text);
	strcat(g_cpcEditText, "_");
	width = p_font->GetTextWidth(g_cpcEditText);
	glyph = p_font->AddOverlayText(p_left, p_top, g_cpcEditText, p_colors);

	for (;;) {
		if (g_cpcLogoMovie) {
			g_cpcLogoMovie->Update();
		}

		if (!PumpMessage()) {
			break;
		}

		g_mouseState->ReadMouseState();
		g_videoDriver->DrawShell();
		if (g_mouseState->GetLeftPressed() == 1) {
			g_cpcEditText[length] = '\0';
			if (glyph) {
				delete glyph;
			}

			strcpy(p_text, g_cpcEditText);
			return 1;
		}

		if (g_keyboardInput->PollKey()) {
			switch (g_keyboardInput->m_key) {
			case 8:
				if (length == 0) {
					break;
				}

				length--;
				g_cpcEditText[length] = '_';
				g_cpcEditText[length + 1] = '\0';
				if (glyph) {
					delete glyph;
				}

				glyph = p_font->AddOverlayText(p_left, p_top, g_cpcEditText, p_colors);
				break;
			case 0x0d:
				g_cpcEditText[length] = '\0';
				if (glyph) {
					delete glyph;
				}

				strcpy(p_text, g_cpcEditText);
				return 1;
			case 0x1b:
				g_cpcEditText[length] = '\0';
				if (glyph) {
					delete glyph;
				}

				strcpy(p_text, g_cpcEditText);
				return 0;
			default:
				key = g_keyboardInput->m_key;
				if (key < 0x20 || key > 0x7f || key == 0x7e) {
					break;
				}

				if (length == p_maxLength) {
					break;
				}

				if (!p_font->GetCharacterWidth(key)) {
					break;
				}

				g_cpcEditText[length] = key;
				length++;
				g_cpcEditText[length] = '_';
				g_cpcEditText[length + 1] = '\0';

				if (p_font->GetTextWidth(g_cpcEditText) < p_maxWidth) {
					if (glyph) {
						delete glyph;
					}

					glyph = p_font->AddOverlayText(p_left, p_top, g_cpcEditText, p_colors);
				}
				else {
					length--;
					g_cpcEditText[length] = '_';
					g_cpcEditText[length + 1] = '\0';
				}
				break;
			}
		}
	}

	return 0;
}

// Edit a field's text in place, then show it.
// FUNCTION: MW2SHELL 0x1003ee3e
void CpcEditField(ScreenField* p_tab)
{
	if (p_tab->m_glyph != NULL) {
		delete p_tab->m_glyph;
	}

	CpcEditTextField(
		g_defaultFont,
		p_tab->m_left,
		p_tab->m_top,
		(MechChar*) p_tab->m_data,
		p_tab->m_colors,
		0x3e,
		p_tab->m_width
	);
	p_tab->m_glyph = g_textFont->AddText(p_tab->m_left, p_tab->m_top, (MechChar*) p_tab->m_data, p_tab->m_colors);
}

// The same as CpcDrawText. Unused: a duplicate keeps its placeholder rather than a second name.
// FUNCTION: MW2SHELL 0x1003eef3
TextGlyph* FUN_1003eef3(ScreenField* p_tab)
{
	MechChar* text;

	text = (MechChar*) p_tab->m_data;
	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, text, p_tab->m_colors);
}

// The device and axis or button bound to a control.
// Stack-slot permutation: device, label, colors and control.
// FUNCTION: MW2SHELL 0x1003ef32
TextGlyph* CpcDrawBinding(ScreenField* p_tab)
{
	InputDevice* device;
	TextGlyph* glyph;
	MechChar* label;
	undefined* colors;
	MechS32 control;

	control = g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex;
	colors = p_tab->m_colors;
	device = InputGetDevice(g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot);
	if (MECH_PTR_TO_S32(p_tab->m_data) == g_cpcSelectedBinding && g_cpcSelectedPart == 0) {
		colors = g_cpcSelectedColors;
	}

	if (device == NULL) {
		strcpy(g_cpcFieldText, "----");
	}
	else {
		if (control < 0) {
			label = "----";
		}
		else if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind == 0) {
			label = device->m_info.m_axisShortNames[control];
		}
		else {
			label = device->m_info.m_buttonShortNames[control];
		}

		if (label == NULL || *label == '\0' || *label == '*') {
			label = "----";
		}

		sprintf(
			g_cpcFieldText,
			"%s %s",
			strcmp(device->m_info.m_shortName, "keyboard") ? device->m_info.m_shortName : "key",
			label
		);
	}

	glyph = g_textFont->AddText(p_tab->m_left, p_tab->m_top, g_cpcFieldText, colors);
	if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind != 2) {
		p_tab->m_width = 100;
	}
	else {
		p_tab->m_width = glyph->m_width;
	}

	return glyph;
}

// The button of a control bound to an axis with a button, placed after the previous field.
// Stack-slot permutation: device, glyph, colors and button.
// FUNCTION: MW2SHELL 0x1003f136
TextGlyph* CpcDrawAxisButton(ScreenField* p_tab)
{
	InputDevice* device;
	TextGlyph* glyph;
	MechChar* label;
	undefined* colors;
	MechS32 button;

	button = g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_mode;
	colors = p_tab->m_colors;
	device = InputGetDevice(g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot);
	if (device == NULL || g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind != 2) {
		return NULL;
	}

	if (p_tab[-1].m_glyph != NULL) {
		p_tab->m_left = p_tab[-1].m_glyph->m_right + 1;
	}
	if (MECH_PTR_TO_S32(p_tab->m_data) == g_cpcSelectedBinding && g_cpcSelectedPart == 1) {
		colors = g_cpcSelectedColors;
	}

	if (button < 0 || button > device->m_info.m_buttonCount) {
		label = "----";
	}
	else {
		label = device->m_info.m_buttonShortNames[button];
	}

	if (label == NULL || *label == '\0') {
		label = "----";
	}

	sprintf(g_cpcFieldText, "/%s", label);
	glyph = g_textFont->AddText(p_tab->m_left, p_tab->m_top, g_cpcFieldText, colors);
	return glyph;
}

// FUNCTION: MW2SHELL 0x1003f283
TextGlyph* CpcDrawControlName(ScreenField* p_tab)
{
	return g_textFont
		->AddText(p_tab->m_left, p_tab->m_top, g_cpcControlLabels[MECH_PTR_TO_S32(p_tab->m_data)], p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x1003f2c0
TextGlyph* CpcDrawNumber(ScreenField* p_tab)
{
	sprintf(g_cpcFieldText, "%d", *(MechS32*) p_tab->m_data);
	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, g_cpcFieldText, p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x1003f30f
TextGlyph* CpcDrawConfigName(ScreenField* p_tab)
{
	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, g_cpcConfigNames[g_cpcConfigShown], p_tab->m_colors);
}

// Show the next configuration.
// FUNCTION: MW2SHELL 0x1003f34b
void CpcNextConfig(ScreenField* p_tab)
{
	if (p_tab) {
	}

	g_cpcConfigShown++;
	if (g_cpcConfigShown >= c_configCount) {
		g_cpcConfigShown = 0;
	}

	g_cpcShownBindings = &g_cpcBindings[g_cpcConfigShown * c_bindingCount];
}

// An axis's direction, placed after the previous field.
// Stack-slot permutation: inverted and glyph.
// FUNCTION: MW2SHELL 0x1003f39e
TextGlyph* CpcDrawAxisDirection(ScreenField* p_tab)
{
	MechS32 inverted;
	TextGlyph* glyph;

	inverted = g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_flags & CPC_FLAG_INVERTED ? 1 : 0;
	if (p_tab[-1].m_glyph != NULL) {
		p_tab->m_left = p_tab[-1].m_glyph->m_right + 1;
	}

	glyph =
		g_textFont
			->AddText(p_tab->m_left, p_tab->m_top, g_cpcAxisDirections[MECH_PTR_TO_S32(p_tab->m_arg)][inverted], NULL);
	return glyph;
}

// FUNCTION: MW2SHELL 0x1003f42e
void CpcToggleInverted(ScreenField* p_tab)
{
	g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_flags ^= CPC_FLAG_INVERTED;
}

// FUNCTION: MW2SHELL 0x1003f469
TextGlyph* CpcDrawModifier(ScreenField* p_tab)
{
	return g_textFont->AddText(
		p_tab->m_left + p_tab->m_width / 2,
		p_tab->m_top,
		g_cpcModifierNames[g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_flags & c_flagModifierMask],
		NULL
	);
}

// Cycle a binding's modifier: none, 1, 4, none.
// FUNCTION: MW2SHELL 0x1003f4be
void CpcCycleModifier(ScreenField* p_tab)
{
	switch (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_flags & c_flagModifierMask) {
	case 0:
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_flags |= 1;
		break;
	case 1:
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_flags &= ~1;
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_flags |= 4;
		break;
	case 4:
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_flags &= ~c_flagModifierMask;
		break;
	}
}

// Leave the screen without committing the configuration.
// FUNCTION: MW2SHELL 0x1003f576
void CpcAbort(ScreenField*)
{
	g_cpcConfigured = 1;
}

// Scroll the button list to the selected binding's button.
// FUNCTION: MW2SHELL 0x1003f590
void CpcScrollToSelection()
{
	MechS32 count;

	count = InputGetDevice(g_curInputDeviceIdx)->m_info.m_buttonCount;
	if (g_cpcShownBindings[g_cpcSelectedBinding].m_deviceSlot == g_curInputDeviceIdx &&
		g_cpcShownBindings[g_cpcSelectedBinding].m_channelKind != 0) {
		if (g_cpcSelectedPart == 1) {
			g_cpcFirstButton = g_cpcShownBindings[g_cpcSelectedBinding].m_mode;
		}
		else {
			g_cpcFirstButton = g_cpcShownBindings[g_cpcSelectedBinding].m_controlIndex;
		}

		if (g_cpcFirstButton < 0) {
			g_cpcFirstButton = 0;
		}
		if (g_cpcFirstButton + c_buttonRows >= count) {
			g_cpcFirstButton = count - c_buttonRows;
		}
		if (g_cpcFirstButton < 0) {
			g_cpcFirstButton = 0;
		}
	}
}

// Advance a binding to the device's next named axis or button (p_mode: the axis's button);
// past the last one, the binding loses its device.
// Stack-slot permutation: names, index and count.
// FUNCTION: MW2SHELL 0x1003f677
void CpcAdvanceBinding(CpcBinding* p_binding, MechS32 p_mode)
{
	InputDevice* device;
	MechChar** names;
	MechS32 index;
	MechS32 count;

	device = InputGetDevice(p_binding->m_deviceSlot);
	if (p_binding->m_channelKind == 0) {
		names = device->m_info.m_axisShortNames;
		count = device->m_info.m_axisCount;
	}
	else {
		names = device->m_info.m_buttonShortNames;
		count = device->m_info.m_buttonCount;
	}

	if (!p_mode) {
		index = p_binding->m_controlIndex;
	}
	else {
		index = p_binding->m_mode;
	}

	if (index < 0) {
		index = -1;
	}

	while (++index < count && (names[index] == NULL || *names[index] == '\0' || *names[index] == '*')) {
	}

	if (index >= count) {
		p_binding->m_deviceSlot = -1;
		index = -1;
	}

	if (!p_mode) {
		p_binding->m_controlIndex = index;
	}
	else {
		p_binding->m_mode = index;
	}
}

// Click on a binding's axis or button: select it, or advance it to the current device's next one.
// A right click clears the binding.
// FUNCTION: MW2SHELL 0x1003f78e
void CpcClickBinding(ScreenField* p_tab)
{
	InputDevice* device;

	device = InputGetDevice(g_curInputDeviceIdx);
	if (g_mouseState->GetRightPressed() == 1) {
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot = -1;
		if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind == 2) {
			g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind = 0;
		}

		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_flags = 0;
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_modeFlags = 0;
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex = -1;
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_mode = -1;
		g_cpcSelectedBinding = MECH_PTR_TO_S32(p_tab->m_data);
		g_cpcSelectedPart = 0;
		return;
	}

	if (MECH_PTR_TO_S32(p_tab->m_data) == g_cpcSelectedBinding && g_cpcSelectedPart == 0) {
		switch (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind) {
		case 0:
			if (device->m_info.m_axisCount == 0) {
				break;
			}

			if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot != g_curInputDeviceIdx) {
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot = g_curInputDeviceIdx;
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex = 0;
			}
			else {
				CpcAdvanceBinding(&g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)], 0);
			}
			break;
		case 1:
			if (device->m_info.m_buttonCount == 0) {
				break;
			}

			if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot != g_curInputDeviceIdx) {
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot = g_curInputDeviceIdx;
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex = -1;
			}

			CpcAdvanceBinding(&g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)], 0);
			break;
		case 2:
			if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot != g_curInputDeviceIdx) {
				if (device->m_info.m_buttonCount != 0) {
					g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot = g_curInputDeviceIdx;
					g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex = -1;
					g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_mode = -1;
					CpcAdvanceBinding(&g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)], 0);
				}
				else if (device->m_info.m_axisCount != 0) {
					g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind = 0;
					g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot = g_curInputDeviceIdx;
					g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex = -1;
					g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_mode = -1;
					CpcAdvanceBinding(&g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)], 0);
				}
			}
			else {
				CpcAdvanceBinding(&g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)], 0);
				if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex < 0) {
					if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_mode < 0) {
						g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind = 0;
					}
					else {
						g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot = g_curInputDeviceIdx;
					}
				}
			}
			break;
		}
	}
	else {
		g_cpcSelectedBinding = MECH_PTR_TO_S32(p_tab->m_data);
		g_cpcSelectedPart = 0;
	}

	CpcScrollToSelection();
}

// Click on the button of a binding to an axis with a button: select it, or advance it to the
// current device's next button. A right click clears the binding.
// FUNCTION: MW2SHELL 0x1003fbb3
void CpcClickAxisButton(ScreenField* p_tab)
{
	InputDevice* device;

	device = InputGetDevice(g_curInputDeviceIdx);
	if (g_mouseState->GetRightPressed() == 1) {
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot = -1;
		if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind == 2) {
			g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind = 0;
		}

		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_flags = 0;
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_modeFlags = 0;
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex = -1;
		g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_mode = -1;
		g_cpcSelectedBinding = MECH_PTR_TO_S32(p_tab->m_data);
		g_cpcSelectedPart = 0;
		return;
	}

	if (MECH_PTR_TO_S32(p_tab->m_data) == g_cpcSelectedBinding && g_cpcSelectedPart == 1) {
		if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot != g_curInputDeviceIdx) {
			if (device->m_info.m_buttonCount != 0) {
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot = g_curInputDeviceIdx;
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex = -1;
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_mode = 0;
			}
			else if (device->m_info.m_axisCount != 0) {
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot = g_curInputDeviceIdx;
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind = 0;
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex = 0;
				g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_mode = -1;
				g_cpcSelectedPart = 0;
			}
		}
		else {
			CpcAdvanceBinding(&g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)], 1);
			if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_mode < 0) {
				if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_controlIndex < 0) {
					g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind = 0;
					g_cpcSelectedPart = 0;
				}
				else {
					g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_deviceSlot = g_curInputDeviceIdx;
				}
			}
		}
	}
	else if (g_cpcShownBindings[MECH_PTR_TO_S32(p_tab->m_data)].m_channelKind == 2) {
		g_cpcSelectedBinding = MECH_PTR_TO_S32(p_tab->m_data);
		g_cpcSelectedPart = 1;
	}

	CpcScrollToSelection();
}

// An axis of the current device.
// Operand order: the original loads the index p_tab->m_data before the name tables; it follows
// the unit's symbol table.
// FUNCTION: MW2SHELL 0x1003fe7b
TextGlyph* CpcDrawDeviceAxis(ScreenField* p_tab)
{
	InputDevice* device;
	undefined* colors;

	device = InputGetDevice(g_curInputDeviceIdx);
	colors = p_tab->m_colors;
	if (MECH_PTR_TO_S32(p_tab->m_data) >= device->m_info.m_axisCount) {
		return NULL;
	}
	if (device->m_info.m_axisShortNames[MECH_PTR_TO_S32(p_tab->m_data)] == NULL) {
		return NULL;
	}
	if (device->m_info.m_axisNames[MECH_PTR_TO_S32(p_tab->m_data)] == NULL) {
		return NULL;
	}
	if (*device->m_info.m_axisShortNames[MECH_PTR_TO_S32(p_tab->m_data)] == '\0') {
		return NULL;
	}
	if (*device->m_info.m_axisNames[MECH_PTR_TO_S32(p_tab->m_data)] == '\0') {
		return NULL;
	}

	if (g_cpcSelectedBinding < 0 || g_cpcShownBindings[g_cpcSelectedBinding].m_channelKind == 1) {
		colors = g_cpcDisabledColors;
	}

	return g_textFont
		->AddText(p_tab->m_left, p_tab->m_top, device->m_info.m_axisNames[MECH_PTR_TO_S32(p_tab->m_data)], colors);
}

// A button of the current device, in the scrolled list.
// FUNCTION: MW2SHELL 0x1003ff95
TextGlyph* CpcDrawDeviceButton(ScreenField* p_tab)
{
	InputDevice* device;
	MechS32 index;
	MechChar* label;
	undefined* colors;

	device = InputGetDevice(g_curInputDeviceIdx);
	colors = p_tab->m_colors;
	index = MECH_PTR_TO_S32(p_tab->m_data) + g_cpcFirstButton;
	if (index >= device->m_info.m_buttonCount) {
		return NULL;
	}

	label = "----";
	if (device->m_info.m_buttonShortNames[index] == NULL || device->m_info.m_buttonNames[index] == NULL ||
		*device->m_info.m_buttonShortNames[index] == '\0' || *device->m_info.m_buttonNames[index] == '\0') {
		label = "----";
		colors = g_cpcDisabledColors;
	}
	else {
		label = device->m_info.m_buttonNames[index];
		if (*device->m_info.m_buttonShortNames[index] == '*') {
			colors = g_cpcDisabledColors;
		}
	}

	if (g_cpcSelectedBinding < 0) {
		colors = g_cpcDisabledColors;
	}

	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, label, colors);
}

// Bind the selected control to an axis of the current device.
// FUNCTION: MW2SHELL 0x100400b7
void CpcBindAxis(ScreenField* p_tab)
{
	InputDevice* device;

	device = InputGetDevice(g_curInputDeviceIdx);
	if (g_cpcSelectedBinding < 0 || MECH_PTR_TO_S32(p_tab->m_data) >= device->m_info.m_axisCount) {
		return;
	}
	if (device->m_info.m_axisShortNames[MECH_PTR_TO_S32(p_tab->m_data)] == NULL) {
		return;
	}
	if (device->m_info.m_axisNames[MECH_PTR_TO_S32(p_tab->m_data)] == NULL) {
		return;
	}
	if (g_cpcShownBindings[g_cpcSelectedBinding].m_channelKind == 1) {
		return;
	}

	g_cpcShownBindings[g_cpcSelectedBinding].m_channelKind = 0;
	g_cpcShownBindings[g_cpcSelectedBinding].m_deviceSlot = g_curInputDeviceIdx;
	g_cpcShownBindings[g_cpcSelectedBinding].m_controlIndex = MECH_PTR_TO_S32(p_tab->m_data);
	g_cpcShownBindings[g_cpcSelectedBinding].m_mode = -1;
	g_cpcSelectedPart = 0;
}

// The button list's scroll arrows: up for a negative step, down otherwise.
// Stack-slot permutation: colors and count.
// FUNCTION: MW2SHELL 0x100401b8
TextGlyph* CpcDrawScrollArrow(ScreenField* p_tab)
{
	undefined* colors;
	MechS32 count;

	count = InputGetDevice(g_curInputDeviceIdx)->m_info.m_buttonCount;
	colors = p_tab->m_colors;
	if (count <= c_buttonRows) {
		return NULL;
	}

	if (MECH_PTR_TO_S32(p_tab->m_data) < 0) {
		if (g_cpcFirstButton == 0) {
			colors = g_cpcDisabledColors;
		}
	}
	else if (g_cpcFirstButton >= count - c_buttonRows) {
		colors = g_cpcDisabledColors;
	}

	return g_textFont->AddText(
		p_tab->m_left,
		p_tab->m_top,
		(MechChar*) (MECH_PTR_TO_S32(p_tab->m_data) >= 0 ? "\x02" : "\x01"),
		colors
	);
}

// Scroll the button list while a mouse button is held; the right button scrolls twice as fast.
// Stack-slot permutation: count and step.
// FUNCTION: MW2SHELL 0x10040272
void CpcScrollButtons(ScreenField* p_tab)
{
	MechS32 count;
	MechS32 step;

	count = InputGetDevice(g_curInputDeviceIdx)->m_info.m_buttonCount;
	step = MECH_PTR_TO_S32(p_tab->m_data);
	if (g_mouseState->m_rightDown == 1) {
		step <<= 1;
	}

	do {
		if ((g_cpcFirstButton += step) < 0) {
			g_cpcFirstButton = 0;
		}
		if (g_cpcFirstButton + c_buttonRows >= count) {
			g_cpcFirstButton = count - c_buttonRows;
		}
		if (g_cpcFirstButton < 0) {
			g_cpcFirstButton = 0;
		}

		RedrawFields(g_cpcBindingsFields);
		if (g_cpcLogoMovie != NULL) {
			g_cpcLogoMovie->Update();
		}

		g_mouseState->ReadMouseState();
	} while (g_mouseState->m_leftDown == 1 || g_mouseState->m_rightDown == 1);
}

// Bind the selected control, or the selected axis's button, to a button of the current device.
// FUNCTION: MW2SHELL 0x1004034e
void CpcBindButton(ScreenField* p_tab)
{
	InputDevice* device;
	MechS32 index;

	device = InputGetDevice(g_curInputDeviceIdx);
	index = MECH_PTR_TO_S32(p_tab->m_data) + g_cpcFirstButton;
	if (g_cpcSelectedBinding < 0 || index >= device->m_info.m_buttonCount) {
		return;
	}
	if (device->m_info.m_buttonShortNames[index] == NULL) {
		return;
	}
	if (device->m_info.m_buttonNames[index] == NULL) {
		return;
	}
	if (*device->m_info.m_buttonShortNames[index] == '\0') {
		return;
	}
	if (*device->m_info.m_buttonNames[index] == '\0') {
		return;
	}
	if (*device->m_info.m_buttonShortNames[index] == '*') {
		return;
	}

	if (g_cpcSelectedPart == 1) {
		if (g_cpcShownBindings[g_cpcSelectedBinding].m_deviceSlot != g_curInputDeviceIdx) {
			g_cpcShownBindings[g_cpcSelectedBinding].m_deviceSlot = g_curInputDeviceIdx;
			g_cpcShownBindings[g_cpcSelectedBinding].m_controlIndex = -1;
		}

		g_cpcShownBindings[g_cpcSelectedBinding].m_mode = index;
		if (g_cpcShownBindings[g_cpcSelectedBinding].m_controlIndex < 0) {
			g_cpcSelectedPart = 0;
		}
	}
	else {
		if (g_cpcShownBindings[g_cpcSelectedBinding].m_channelKind == 0) {
			g_cpcShownBindings[g_cpcSelectedBinding].m_channelKind = 2;
			g_cpcShownBindings[g_cpcSelectedBinding].m_mode = -1;
		}
		if (g_cpcShownBindings[g_cpcSelectedBinding].m_deviceSlot != g_curInputDeviceIdx) {
			g_cpcShownBindings[g_cpcSelectedBinding].m_mode = -1;
		}

		g_cpcShownBindings[g_cpcSelectedBinding].m_deviceSlot = g_curInputDeviceIdx;
		g_cpcShownBindings[g_cpcSelectedBinding].m_controlIndex = index;
		if (g_cpcShownBindings[g_cpcSelectedBinding].m_channelKind == 2 &&
			g_cpcShownBindings[g_cpcSelectedBinding].m_mode < 0) {
			g_cpcSelectedPart = 1;
		}
	}
}

// The current device's name: "Joystick N" for a joystick.
// Stack-slot permutation: name and number.
// FUNCTION: MW2SHELL 0x1004059e
TextGlyph* CpcDrawDeviceName(ScreenField* p_tab)
{
	undefined* colors; // Set and never read.
	size_t number;
	MechChar name[12];
	InputDevice* device;

	colors = p_tab->m_colors;
	device = InputGetDevice(g_curInputDeviceIdx);
	if (device == NULL) {
		return NULL;
	}

	if (strncmp(device->m_info.m_shortName, "joystick", 8) == 0) {
		number = strcspn(device->m_info.m_shortName, "1234567890");
		sprintf(name, "Joystick %s", device->m_info.m_shortName + number);
	}
	else {
		strcpy(name, device->m_info.m_displayName);
	}

	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, name, p_tab->m_colors);
}

// An input device of the bindings screen, its name shortened to fit.
// FUNCTION: MW2SHELL 0x1004067c
TextGlyph* CpcDrawBindingDevice(ScreenField* p_tab)
{
	InputDevice* device;
	undefined* colors;
	MechS32 index;

	colors = p_tab->m_colors;
	if (MECH_PTR_TO_S32(p_tab->m_data) >= 0 && !g_inputDeviceActive[MECH_PTR_TO_S32(p_tab->m_data)]) {
		colors = g_cpcDisabledColors;
	}
	if (MECH_PTR_TO_S32(p_tab->m_data) == g_curInputDeviceIdx) {
		colors = g_cpcSelectedColors;
	}

	if (MECH_PTR_TO_S32(p_tab->m_data) < 0) {
		index = g_curInputDeviceIdx;
	}
	else {
		index = MECH_PTR_TO_S32(p_tab->m_data);
	}

	device = InputGetDevice(index);
	if (device == NULL) {
		return NULL;
	}

	while (g_textFont->GetTextWidth(device->m_info.m_displayName) > 175) {
		strcpy(device->m_info.m_displayName + strlen(device->m_info.m_displayName) - 4, "...");
	}

	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, device->m_info.m_displayName, colors);
}

// An input device of the devices screen: highlighted when active, disabled while four are.
// FUNCTION: MW2SHELL 0x1004079f
TextGlyph* CpcDrawDeviceEntry(ScreenField* p_tab)
{
	InputDevice* device;
	undefined* colors;
	MechS32 index;

	colors = p_tab->m_colors;
	if (MECH_PTR_TO_S32(p_tab->m_data) >= 0 && g_inputDeviceActive[MECH_PTR_TO_S32(p_tab->m_data)]) {
		colors = g_cpcSelectedColors;
	}
	else if (g_activeInputDeviceCount >= c_maxActiveDevices) {
		colors = g_cpcDisabledColors;
	}

	if (MECH_PTR_TO_S32(p_tab->m_data) < 0) {
		index = g_curInputDeviceIdx;
	}
	else {
		index = MECH_PTR_TO_S32(p_tab->m_data);
	}

	device = InputGetDevice(index);
	if (device == NULL) {
		return NULL;
	}

	while (g_textFont->GetTextWidth(device->m_info.m_displayName) > 175) {
		strcpy(device->m_info.m_displayName + strlen(device->m_info.m_displayName) - 4, "...");
	}

	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, device->m_info.m_displayName, colors);
}

// Activate or deactivate a device; the keyboard stays as it is, and at most four are active.
// FUNCTION: MW2SHELL 0x100408c2
void InputToggleDeviceActive(ScreenField* p_tab)
{
	MechS32 count;

	count = InputEnumDevices(FALSE);
	if (InputEnumDevices(TRUE) != count) {
		CpcLoadActiveDevices(NULL);
	}

	if (InputGetDevice(MECH_PTR_TO_S32(p_tab->m_data)) != NULL) {
		if (strcmp(g_cpcDeviceSlots[MECH_PTR_TO_S32(p_tab->m_data)].m_name, "keyboard")) {
			if (g_inputDeviceActive[MECH_PTR_TO_S32(p_tab->m_data)]) {
				g_activeInputDeviceCount--;
				g_inputConfigChanged = 1;
				g_inputDeviceActive[MECH_PTR_TO_S32(p_tab->m_data)] = 0;
				if (MECH_PTR_TO_S32(p_tab->m_data) == g_curInputDeviceIdx) {
					while (!g_inputDeviceActive[--g_curInputDeviceIdx]) {
					}
				}
			}
			else if (g_activeInputDeviceCount < c_maxActiveDevices) {
				g_activeInputDeviceCount++;
				g_inputConfigChanged = 1;
				g_inputDeviceActive[MECH_PTR_TO_S32(p_tab->m_data)] = 1;
				g_curInputDeviceIdx = MECH_PTR_TO_S32(p_tab->m_data);
			}
		}
	}
}

// Make an active device current.
// FUNCTION: MW2SHELL 0x10040a0d
void CpcSelectDevice(ScreenField* p_tab)
{
	if (g_inputDeviceActive[MECH_PTR_TO_S32(p_tab->m_data)]) {
		if (InputGetDevice(MECH_PTR_TO_S32(p_tab->m_data)) != NULL) {
			g_curInputDeviceIdx = MECH_PTR_TO_S32(p_tab->m_data);
			g_cpcFirstButton = 0;
		}
	}
}

// A vertical line from the field down to the bottom of the panel.
// FUNCTION: MW2SHELL 0x10040a5d
TextGlyph* CpcDrawDivider(ScreenField* p_tab)
{
	g_videoDriver->DrawLine(p_tab->m_left, p_tab->m_top, p_tab->m_left, 0x1d6, 0x10);
	return NULL;
}

// FUNCTION: MW2SHELL 0x10040a94
TextGlyph* CpcDrawSlotName(ScreenField* p_tab)
{
	MechS32 index;

	index = *(MechS32*) p_tab->m_data;
	if (index < 0 || index > 4) {
		index = 0;
	}

	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, g_cpcSlotNames[index], p_tab->m_colors);
}

// FUNCTION: MW2SHELL 0x10040af7
void CpcAcceptDevices(ScreenField*)
{
	if (g_inputConfigChanged) {
		CpcLoadActiveDevices(NULL);
	}

	CpcAcceptAndCommit(NULL);
	g_cpcConfigured = 1;
}

// Text shown only while fewer than three devices are enumerated.
// FUNCTION: MW2SHELL 0x10040b32
TextGlyph* CpcDrawNoJoysticks(ScreenField* p_tab)
{
	MechChar* text;

	if (InputEnumDevices(FALSE) <= 2) {
		text = (MechChar*) p_tab->m_data;
	}
	else {
		text = NULL;
	}

	return g_textFont->AddText(p_tab->m_left, p_tab->m_top, text, g_cpcSelectedColors);
}

// Enumerate the devices again, and explain when there is no joystick.
// Operand order: the original compares count <= g_curInputDeviceIdx with count loaded first; it
// follows the unit's symbol table.
// FUNCTION: MW2SHELL 0x10040b8e
void CpcCheckJoysticks(ScreenField*)
{
	MechS32 count;

	count = InputEnumDevices(FALSE);
	if (InputEnumDevices(TRUE) != count) {
		CpcLoadActiveDevices(NULL);
		count = InputEnumDevices(FALSE);
		if (count <= g_curInputDeviceIdx) {
			g_inputDeviceActive[g_curInputDeviceIdx] = 0;
			g_curInputDeviceIdx = count - 1;
		}
	}

	if (InputEnumDevices(FALSE) <= 2) {
		CpcShowDeviceMessage(c_messageNoJoysticks, NULL, NULL);
	}
}

// Enumerate the devices again; the current device falls back to the last one.
// FUNCTION: MW2SHELL 0x10040c21
void CpcRefreshDevices(ScreenField*)
{
	MechS32 count;

	count = InputEnumDevices(TRUE);
	if (count <= g_curInputDeviceIdx) {
		g_inputDeviceActive[g_curInputDeviceIdx] = 0;
		g_curInputDeviceIdx = count - 1;
	}

	CpcLoadActiveDevices(NULL);
}

// Leave the devices screen for the bindings screen.
// FUNCTION: MW2SHELL 0x10040c72
void CpcShowBindingsPage(ScreenField*)
{
	if (g_inputConfigChanged) {
		CpcLoadActiveDevices(NULL);
	}

	HideFields(g_cpcDevicesFields);
	ShowFields(g_cpcBindingsFields);
	g_cpcBindingsPage = 1;
	g_cpcConfigured = 0;
}

// The original opened the Windows joystick control panel (control.exe joy.cpl). Does nothing
// for now.
// FUNCTION: MW2SHELL 0x10040cc7
void CpcOpenJoystickControlPanel(ScreenField*)
{
}

// Write a binding's modifier keys to input.map: pressed for bits 0-2, released for bits 4-6.
// FUNCTION: MW2SHELL 0x10040d89
void CpcWriteModifiers(FILE* p_file, CpcBinding* p_binding)
{
	if (p_binding->m_flags & 1) {
		fprintf(p_file, "\t+ keyboard\tControl\n");
	}
	else if (p_binding->m_flags & 0x10) {
		fprintf(p_file, "\t- keyboard\tControl\n");
	}

	if (p_binding->m_flags & 2) {
		fprintf(p_file, "\t+ keyboard\tAlt\n");
	}
	else if (p_binding->m_flags & 0x20) {
		fprintf(p_file, "\t- keyboard\tAlt\n");
	}

	if (p_binding->m_flags & 4) {
		fprintf(p_file, "\t+ keyboard\tShift\n");
	}
	else if (p_binding->m_flags & 0x40) {
		fprintf(p_file, "\t- keyboard\tShift\n");
	}
}

// Write an axis binding to input.map.
// Stack-slot permutation: device and axis.
// FUNCTION: MW2SHELL 0x10040e5c
void CpcWriteAxisBinding(FILE* p_file, MechChar* p_name, CpcBinding* p_binding)
{
	InputDevice* device;
	MechS32 axis;

	device = InputGetDevice(p_binding->m_deviceSlot);
	axis = p_binding->m_controlIndex;
	if (axis < 0) {
		return;
	}

	g_cpcAnalogCount++;
	fprintf(p_file, "%s {\n", p_name);
	fprintf(
		p_file,
		"\t%c %s\t%s\n",
		p_binding->m_flags & CPC_FLAG_INVERTED ? '-' : '+',
		device->m_info.m_shortName,
		device->m_info.m_axisShortNames[axis]
	);
	CpcWriteModifiers(p_file, p_binding);
	fprintf(p_file, "}\n");
}

// Write a button binding to input.map; a jump jet button also enables the jump jets.
// FUNCTION: MW2SHELL 0x10040f14
void CpcWriteButtonBinding(FILE* p_file, MechChar* p_name, CpcBinding* p_binding)
{
	InputDevice* device;
	MechS32 button;

	device = InputGetDevice(p_binding->m_deviceSlot);
	button = p_binding->m_controlIndex;
	if (button < 0) {
		return;
	}

	g_cpcDiscreteCount++;
	fprintf(p_file, "%s {\n", p_name);
	fprintf(p_file, "\t+ %s\t%s\n", device->m_info.m_shortName, device->m_info.m_buttonShortNames[button]);
	CpcWriteModifiers(p_file, p_binding);
	fprintf(p_file, "}\n");
	if (strncmp(p_name, "jumpjet_fire_", 13) == 0) {
		CpcWriteButtonBinding(p_file, "jumpjet_enabled", p_binding);
	}
}

// Write a binding to an axis with a button to input.map, as two buttons: the control's _minus
// and _plus.
// Stack-slot permutation: saved, second, first and delta.
// FUNCTION: MW2SHELL 0x10040fe2
void CpcWriteAxisButtonBinding(FILE* p_file, MechChar* p_name, CpcBinding* p_binding)
{
	CpcBinding saved;
	MechChar second[80];
	MechChar first[80];
	MechChar* delta;

	saved = *p_binding;
	strcpy(first, p_name);
	delta = strstr(first, "_delta");
	if (delta != NULL) {
		*delta = '\0';
	}

	strcpy(second, first);
	if (p_binding->m_flags & CPC_FLAG_INVERTED) {
		strcat(second, "_minus");
		strcat(first, "_plus");
	}
	else {
		strcat(first, "_minus");
		strcat(second, "_plus");
	}

	CpcWriteButtonBinding(p_file, first, p_binding);
	p_binding->m_controlIndex = p_binding->m_mode;
	p_binding->m_flags = p_binding->m_modeFlags;
	CpcWriteButtonBinding(p_file, second, p_binding);
	*p_binding = saved;
}

// Write a binding to input.map, with the sim controls that follow it.
// FUNCTION: MW2SHELL 0x10041168
void CpcWriteBinding(FILE* p_file, MechChar* p_name, CpcBinding* p_binding)
{
	if (p_binding->m_controlIndex < 0) {
		return;
	}

	if (p_binding->m_channelKind == 0) {
		CpcWriteAxisBinding(p_file, p_name, p_binding);
	}
	else if (p_binding->m_channelKind == 1) {
		CpcWriteButtonBinding(p_file, p_name, p_binding);
	}
	else if (p_binding->m_channelKind == 2) {
		CpcWriteAxisButtonBinding(p_file, p_name, p_binding);
	}

	if (!strcmp(p_name, "pilot_tilt")) {
		CpcWriteBinding(p_file, "track_height_delta", p_binding);
	}
	else if (!strcmp(p_name, "pilot_pan")) {
		CpcWriteBinding(p_file, "eyepoint_pan_delta", p_binding);
	}
	else if (!strcmp(p_name, "zoom_factor")) {
		p_binding->m_flags ^= CPC_FLAG_INVERTED;
		CpcWriteBinding(p_file, "track_distance_delta", p_binding);
		p_binding->m_flags ^= CPC_FLAG_INVERTED;
	}
	else if (!strcmp(p_name, "torso_tilt_reset")) {
		CpcWriteBinding(p_file, "pilot_tilt_reset", p_binding);
		CpcWriteBinding(p_file, "torso_pan_reset", p_binding);
		CpcWriteBinding(p_file, "pilot_pan_reset", p_binding);
	}
	else if (!strcmp(p_name, "glance_up")) {
		CpcWriteBinding(p_file, "track_height_minus", p_binding);
	}
	else if (!strcmp(p_name, "glance_down")) {
		CpcWriteBinding(p_file, "track_height_plus", p_binding);
	}
	else if (!strcmp(p_name, "glance_right")) {
		CpcWriteBinding(p_file, "eyepoint_pan_plus", p_binding);
	}
	else if (!strcmp(p_name, "glance_left")) {
		CpcWriteBinding(p_file, "eyepoint_pan_minus", p_binding);
	}
}

// Works out the modifier flags each binding must exclude (the modifiers of the other bindings of
// the same axis or button), writes the bindings to temp.map, and returns whether the sim can
// take their analog and discrete counts.
// Not 100%: the stack slots of the locals are permuted.
// FUNCTION: MW2SHELL 0x1004154b
MechS32 CpcCheckControlCount()
{
	MechS32 mask;
	CpcBinding binding;
	MechU32 i;
	MechU32 j;
	FILE* file;

	for (i = 0; i < c_configCount * c_bindingCount; i++) {
		switch (g_cpcBindings[i].m_channelKind) {
		case 0:
		case 1:
			g_cpcBindings[i].m_modeFlags = 0;
			break;
		case 2:
			if (g_cpcBindings[i].m_mode == g_cpcBindings[i].m_controlIndex) {
				g_cpcBindings[i].m_modeFlags = (g_cpcBindings[i].m_flags << 4) & 0x70;
			}
			else {
				g_cpcBindings[i].m_modeFlags = g_cpcBindings[i].m_flags;
			}
		}
	}

	for (i = 0; i < c_configCount * c_bindingCount; i++) {
		if (g_cpcBindings[i].m_deviceSlot < 0) {
			continue;
		}

		mask = (g_cpcBindings[i].m_flags << 4) & 0x70;
		if (mask) {
			switch (g_cpcBindings[i].m_channelKind) {
			case 0:
				for (j = 0; j < c_configCount * c_bindingCount; j++) {
					if (j == i) {
						continue;
					}
					if (g_cpcBindings[j].m_deviceSlot < 0) {
						continue;
					}
					if (g_cpcBindings[j].m_channelKind != 0) {
						continue;
					}
					if (g_cpcBindings[j].m_controlIndex != g_cpcBindings[i].m_controlIndex) {
						continue;
					}

					g_cpcBindings[j].m_flags |= mask;
				}
				break;
			case 1:
			case 2:
				for (j = 0; j < c_configCount * c_bindingCount; j++) {
					if (j == i) {
						continue;
					}
					if (g_cpcBindings[j].m_deviceSlot < 0) {
						continue;
					}
					if (g_cpcBindings[j].m_channelKind == 0) {
						continue;
					}

					if (g_cpcBindings[j].m_controlIndex == g_cpcBindings[i].m_controlIndex) {
						g_cpcBindings[j].m_flags |= mask;
					}
					if (g_cpcBindings[j].m_channelKind == 2 &&
						g_cpcBindings[j].m_mode == g_cpcBindings[i].m_controlIndex) {
						g_cpcBindings[j].m_modeFlags |= mask;
					}
					if (g_cpcBindings[j].m_channelKind == 2 &&
						g_cpcBindings[i].m_mode == g_cpcBindings[j].m_controlIndex) {
						g_cpcBindings[j].m_flags |= (g_cpcBindings[i].m_modeFlags << 4) & 0x70;
					}
					if (g_cpcBindings[j].m_channelKind == 2 && g_cpcBindings[j].m_mode == g_cpcBindings[i].m_mode) {
						g_cpcBindings[j].m_modeFlags |= (g_cpcBindings[i].m_modeFlags << 4) & 0x70;
					}
				}
				break;
			}
		}
	}

	file = MechFopen("temp.map", "w");
	g_cpcAnalogCount = 0;
	g_cpcDiscreteCount = 0;
	if (file) {
		fprintf(file, "# mw2shell CockPit Config generated map file\n");
		for (i = 0; i < c_configCount * c_bindingCount; i++) {
			if (g_cpcBindings[i].m_deviceSlot >= 0) {
				CpcWriteBinding(file, g_cpcSimControlNames[i % c_bindingCount], &g_cpcBindings[i]);
				g_cpcBindings[i].m_flags &= CPC_FLAG_INVERTED | c_flagModifierMask;
				g_cpcBindings[i].m_modeFlags = 0;
			}
		}

		binding.m_channelKind = 1;
		binding.m_deviceSlot = g_cpcLegsPanSlot;
		binding.m_flags = 0;
		binding.m_mode = -1;
		binding.m_modeFlags = -1;
		binding.m_controlIndex = 0x66;
		CpcWriteBinding(file, "legs_pan_minus", &binding);
		CpcWriteBinding(file, "jumpjet_enabled", &binding);
		binding.m_controlIndex = 0x64;
		CpcWriteBinding(file, "legs_pan_plus", &binding);
		CpcWriteBinding(file, "jumpjet_enabled", &binding);
		fprintf(file, "# analog count = %d\n", g_cpcAnalogCount);
		fprintf(file, "# discrete count = %d\n", g_cpcDiscreteCount);
		fclose(file);
	}
	else {
		ShowDialog("Error: Could not write map file.#Ok", 0);
	}

	return g_cpcAnalogCount <= 40 && g_cpcDiscreteCount <= 90;
}

// Mark the keyboard and the devices the bindings use active, drop the bindings of missing
// devices, then keep at most four devices active; the last one becomes current.
// FUNCTION: MW2SHELL 0x10041abc
void CpcValidateActiveDevices()
{
	InputDevice* device;
	MechU32 index;

	for (index = 0; (MechS32) index < c_deviceSlotCount; index++) {
		device = InputGetDevice(index);
		if (device != NULL && !strcmp(device->m_info.m_shortName, "keyboard")) {
			g_inputDeviceActive[index] = 1;
		}
		else {
			g_inputDeviceActive[index] = 0;
		}
	}

	for (index = 0; index < c_configCount * c_bindingCount; index++) {
		if (g_cpcBindings[index].m_deviceSlot >= 0) {
			g_curInputDeviceIdx = g_cpcBindings[index].m_deviceSlot;
			if (InputGetDevice(g_curInputDeviceIdx) == NULL) {
				CpcResetBinding(index, &g_cpcBindings[index]);
			}
			else {
				g_inputDeviceActive[g_curInputDeviceIdx] = 1;
			}
		}
	}

	g_activeInputDeviceCount = 0;
	for (index = 0; (MechS32) index < c_deviceSlotCount; index++) {
		device = InputGetDevice(index);
		if (device != NULL && g_inputDeviceActive[index] && g_activeInputDeviceCount < c_maxActiveDevices) {
			g_activeInputDeviceCount++;
			g_curInputDeviceIdx = index;
		}
		else {
			g_inputDeviceActive[index] = 0;
		}
	}
}

// Maps the device slots of loaded bindings onto the devices present: by name, then for a
// missing joystick onto an unused one (explaining either way), and drops the bindings whose
// device, axis or button is gone. Then records the present devices in the slots.
// Not 100%: the stack slots of the locals are permuted.
// FUNCTION: MW2SHELL 0x10041c7e
void CpcRemapDeviceSlots(CpcBinding* p_bindings)
{
	InputDevice* device;
	MechU32 i;
	MechS32 j;
	MechS32 k;
	MechS32 missing[c_deviceSlotCount];

	for (i = 0; (MechS32) i < c_deviceSlotCount; i++) {
		if (g_cpcDeviceSlots[i].m_deviceId != -1) {
			missing[i] = 1;
			g_cpcDeviceSlots[i].m_deviceId = -1;
		}
		else {
			missing[i] = 0;
		}
	}

	for (i = 0; (MechS32) i < c_deviceSlotCount; i++) {
		device = InputGetDevice(i);
		if (!device) {
			break;
		}

		for (j = 0; j < c_deviceSlotCount; j++) {
			if (!device->m_info.m_matchName[0]) {
				break;
			}

			if (!strcasecmp(g_cpcDeviceSlots[j].m_name, device->m_info.m_matchName) &&
				g_cpcDeviceSlots[i].m_deviceId == -1) {
				g_cpcDeviceSlots[j].m_deviceId = i;
				break;
			}
		}
	}

	for (i = 0; (MechS32) i < c_deviceSlotCount; i++) {
		if (missing[i] && g_cpcDeviceSlots[i].m_deviceId == -1 && g_cpcDeviceSlots[i].m_name[0]) {
			for (j = 0; j < c_deviceSlotCount; j++) {
				device = InputGetDevice(j);
				if (device && device->m_info.m_shortName[0] == 'j') {
					for (k = 0; k < c_deviceSlotCount; k++) {
						if (g_cpcDeviceSlots[k].m_deviceId == j) {
							break;
						}
					}

					if (k == c_deviceSlotCount) {
						CpcShowDeviceMessage(
							c_messageRemapJoystick,
							g_cpcDeviceSlots[i].m_name,
							device->m_info.m_displayName
						);
						g_cpcDeviceSlots[i].m_deviceId = j;
						break;
					}
				}
			}

			if (j == c_deviceSlotCount) {
				CpcShowDeviceMessage(c_messageMissingJoystick, g_cpcDeviceSlots[i].m_name, NULL);
			}
		}
	}

	for (i = 0; i < c_configCount * c_bindingCount; i++) {
		if (p_bindings[i].m_deviceSlot >= 0) {
			device = InputGetDevice(g_cpcDeviceSlots[p_bindings[i].m_deviceSlot].m_deviceId);
			if (device) {
				if (p_bindings[i].m_channelKind == 0 && p_bindings[i].m_mode == -1) {
					if (p_bindings[i].m_controlIndex >= device->m_info.m_axisCount) {
						p_bindings[i].m_deviceSlot = -1;
					}
				}
				else {
					if (p_bindings[i].m_controlIndex >= device->m_info.m_buttonCount) {
						p_bindings[i].m_deviceSlot = -1;
					}
				}

				if (p_bindings[i].m_deviceSlot >= 0) {
					p_bindings[i].m_deviceSlot = g_cpcDeviceSlots[p_bindings[i].m_deviceSlot].m_deviceId;
				}
			}
		}
	}

	for (i = 0; (MechS32) i < c_deviceSlotCount; i++) {
		device = InputGetDevice(i);
		if (device) {
			g_cpcDeviceSlots[i].m_deviceId = i;
			strncpy(g_cpcDeviceSlots[i].m_name, device->m_info.m_matchName, 0x10);
		}
		else {
			g_cpcDeviceSlots[i].m_deviceId = -1;
			g_cpcDeviceSlots[i].m_name[0] = '\0';
		}
	}
}

// Reset a binding: the first seven controls are axes, the rest buttons.
// FUNCTION: MW2SHELL 0x1004207e
void CpcResetBinding(MechS32 p_index, CpcBinding* p_binding)
{
	if (p_binding != NULL) {
		p_binding->m_deviceSlot = -1;
		if (p_index >= c_axisBindingCount) {
			p_binding->m_channelKind = 1;
		}
		else {
			p_binding->m_channelKind = 0;
		}

		p_binding->m_flags = 0;
		p_binding->m_modeFlags = 0;
		p_binding->m_controlIndex = -1;
		p_binding->m_mode = -1;
	}
}

// Reset every binding of the four configurations.
// FUNCTION: MW2SHELL 0x100420eb
void CpcResetAllBindings(ScreenField* p_tab)
{
	MechS32 config;
	MechU32 index;

	if (p_tab) {
	}

	for (config = 0; config < c_configCount; config++) {
		for (index = 0; index < c_bindingCount; index++) {
			CpcResetBinding(index, &g_cpcBindings[config * c_bindingCount + index]);
		}
	}

	g_cpcConfigShown = 0;
	g_cpcShownBindings = &g_cpcBindings[g_cpcConfigShown * c_bindingCount];
	g_curCpcConfigSlot = 0;
}

// Reset the bindings to the current device.
// FUNCTION: MW2SHELL 0x10042199
void CpcResetBindingsToDevice(ScreenField* p_tab)
{
	MechS32 config;
	MechU32 index;

	if (p_tab) {
	}

	for (config = 0; config < c_configCount; config++) {
		for (index = 0; index < c_bindingCount; index++) {
			if (g_cpcBindings[config * c_bindingCount + index].m_deviceSlot != g_curInputDeviceIdx) {
				continue;
			}

			g_cpcBindings[config * c_bindingCount + index].m_deviceSlot = -1;
			if ((MechS32) index >= c_axisBindingCount) {
				g_cpcBindings[config * c_bindingCount + index].m_channelKind = 1;
			}
			else {
				g_cpcBindings[config * c_bindingCount + index].m_channelKind = 0;
			}

			g_cpcBindings[config * c_bindingCount + index].m_flags = 0;
			g_cpcBindings[config * c_bindingCount + index].m_modeFlags = 0;
			g_cpcBindings[config * c_bindingCount + index].m_controlIndex = 0;
			g_cpcBindings[config * c_bindingCount + index].m_mode = 0;
		}
	}

	g_cpcConfigShown = 0;
	g_cpcShownBindings = &g_cpcBindings[g_cpcConfigShown * c_bindingCount];
}

// Load the current device's .cpc file and merge its bindings into the free ones.
// Stack-slot permutation: path, other, config, index and binding.
// FUNCTION: MW2SHELL 0x10042314
void CpcLoadDeviceFile(ScreenField* p_tab)
{
	FILE* file;
	MechChar path[32];
	MechS32 other;
	MechS32 config;
	MechU32 index;
	MechS32 binding;

	if (p_tab) {
	}

	sprintf(path, "giddi\\%s.cpc", InputGetDevice(g_curInputDeviceIdx)->m_info.m_matchName);
	file = MechFopen(path, "rb");
	if (file == NULL) {
		return;
	}

	fread(g_cpcConfigName, sizeof(g_cpcConfigName), 1, file);
	fread(g_cpcDeviceSlots, sizeof(CpcDeviceSlot), c_deviceSlotCount, file);
	fread(g_cpcDeviceFileBindings, sizeof(g_cpcDeviceFileBindings), 1, file);
	fclose(file);
	CpcRemapDeviceSlots(g_cpcDeviceFileBindings);

	for (config = 0; config < c_configCount; config++) {
		for (index = 0; index < c_bindingCount; index++) {
			binding = config * c_bindingCount + index;
			if (g_cpcDeviceFileBindings[binding].m_deviceSlot == g_curInputDeviceIdx) {
				for (other = 0; other < c_configCount; other++) {
					if (g_cpcBindings[other * c_bindingCount + index].m_deviceSlot ==
							g_cpcDeviceFileBindings[binding].m_deviceSlot &&
						g_cpcBindings[other * c_bindingCount + index].m_channelKind ==
							g_cpcDeviceFileBindings[binding].m_channelKind &&
						g_cpcBindings[other * c_bindingCount + index].m_flags ==
							g_cpcDeviceFileBindings[binding].m_flags &&
						g_cpcBindings[other * c_bindingCount + index].m_controlIndex ==
							g_cpcDeviceFileBindings[binding].m_controlIndex &&
						g_cpcBindings[other * c_bindingCount + index].m_mode ==
							g_cpcDeviceFileBindings[binding].m_mode &&
						g_cpcBindings[other * c_bindingCount + index].m_modeFlags ==
							g_cpcDeviceFileBindings[binding].m_modeFlags) {
						break;
					}

					if (g_cpcBindings[other * c_bindingCount + index].m_deviceSlot < 0) {
						g_cpcBindings[other * c_bindingCount + index] = g_cpcDeviceFileBindings[binding];
						break;
					}
				}
			}
		}
	}

	g_cpcConfigShown = 0;
	g_cpcShownBindings = &g_cpcBindings[g_cpcConfigShown * c_bindingCount];
}

// Load giddi\configNN.cpc: slot 0 without a field, the field's slot, or the current slot.
// Stack-slot permutation: path and slot.
// FUNCTION: MW2SHELL 0x100425d3
void CpcLoadConfigSlot(ScreenField* p_tab)
{
	FILE* file;
	MechChar path[32];
	MechS32 slot;

	if (p_tab == NULL) {
		slot = 0;
	}
	else if (MECH_PTR_TO_S32(p_tab->m_arg) < 0) {
		slot = g_curCpcConfigSlot;
	}
	else {
		slot = g_curCpcConfigSlot = MECH_PTR_TO_S32(p_tab->m_arg);
	}

	sprintf(path, "giddi\\config%02d.cpc", slot);
	file = MechFopen(path, "rb");
	if (file == NULL) {
		return;
	}

	fread(g_cpcConfigName, sizeof(g_cpcConfigName), 1, file);
	fread(g_cpcDeviceSlots, sizeof(CpcDeviceSlot), c_deviceSlotCount, file);
	fread(g_cpcBindings, sizeof(g_cpcBindings), 1, file);
	fclose(file);
	CpcRemapDeviceSlots(g_cpcBindings);
	CpcValidateActiveDevices();
	g_cpcConfigShown = 0;
	g_cpcShownBindings = &g_cpcBindings[g_cpcConfigShown * c_bindingCount];
}

// Save giddi\configNN.cpc, naming a custom slot's configuration after the slot.
// FUNCTION: MW2SHELL 0x100426e7
void CpcSaveConfigSlot(ScreenField* p_tab)
{
	FILE* file;
	MechS32 slot;

	if (p_tab == NULL) {
		slot = 0;
	}
	else if (MECH_PTR_TO_S32(p_tab->m_arg) < 0) {
		slot = g_curCpcConfigSlot;
	}
	else {
		slot = g_curCpcConfigSlot = MECH_PTR_TO_S32(p_tab->m_arg);
	}

	sprintf(g_cpcSlotText, "giddi\\config%02d.cpc", slot);
	if (slot != 0) {
		if (!strcmp("Default Config", g_cpcConfigName)) {
			sprintf(g_cpcConfigName, "Custom Config #%d", slot);
		}
		else if (!strncmp("Custom Config #", g_cpcConfigName, 15)) {
			sprintf(g_cpcConfigName, "Custom Config #%d", slot);
		}
	}

	file = MechFopen(g_cpcSlotText, "wb");
	if (file != NULL) {
		fwrite(g_cpcConfigName, sizeof(g_cpcConfigName), 1, file);
		fwrite(g_cpcDeviceSlots, sizeof(CpcDeviceSlot), c_deviceSlotCount, file);
		fwrite(g_cpcBindings, sizeof(g_cpcBindings), 1, file);
		fclose(file);
	}

	fclose(file);
	if (slot != 0) {
		sprintf(g_cpcSlotText, "Configuration %d Saved.#Ok", slot);
		ShowDialog(g_cpcSlotText, 0);
	}
}

// Reset the bindings and load the .cpc file of every active device.
// FUNCTION: MW2SHELL 0x1004289c
void CpcLoadActiveDevices(ScreenField* p_tab)
{
	MechS32 current;

	current = g_curInputDeviceIdx;
	if (p_tab) {
	}

	CpcResetAllBindings(NULL);
	for (g_curInputDeviceIdx = 0; g_curInputDeviceIdx < c_deviceSlotCount; g_curInputDeviceIdx++) {
		if (g_inputDeviceActive[g_curInputDeviceIdx]) {
			CpcLoadDeviceFile(NULL);
		}
	}

	strcpy(g_cpcConfigName, "Default Config");
	g_curInputDeviceIdx = current;
}

// Install the new input.map (keeping the old one as input.bak) and save the configuration.
// FUNCTION: MW2SHELL 0x10042940
void CpcAcceptAndCommit(ScreenField* p_tab)
{
	if (p_tab) {
	}

	if (!CpcCheckControlCount()) {
		ShowDialog("Error: Sim can not|handle that many controls.#Ok", 0);
		return;
	}

	MechRemove("input.bak");
	MechRename("input.map", "input.bak");
	MechRename("temp.map", "input.map");
	CpcSaveConfigSlot(NULL);
	ShowDialog("Cockpit Control Configured.#Ok", 0);
	g_cpcConfigured = 1;
}

// Save the current device's bindings to its .cpc file.
// Stack-slot permutation: path, config, index and binding.
// FUNCTION: MW2SHELL 0x100429cf
void CpcSaveDeviceFile(ScreenField* p_tab)
{
	FILE* file;
	MechChar path[32];
	MechS32 config;
	MechU32 index;
	MechS32 binding;

	if (p_tab) {
	}

	for (config = 0; config < c_configCount; config++) {
		for (index = 0; index < c_bindingCount; index++) {
			binding = config * c_bindingCount + index;
			if (g_cpcBindings[binding].m_deviceSlot == g_curInputDeviceIdx) {
				g_cpcDeviceFileBindings[binding] = g_cpcBindings[binding];
			}
			else {
				g_cpcDeviceFileBindings[binding].m_deviceSlot = -1;
				if ((MechS32) index >= c_axisBindingCount) {
					g_cpcDeviceFileBindings[binding].m_channelKind = 1;
				}
				else {
					g_cpcDeviceFileBindings[binding].m_channelKind = 0;
				}

				g_cpcDeviceFileBindings[binding].m_flags = 0;
				g_cpcDeviceFileBindings[binding].m_modeFlags = 0;
				g_cpcDeviceFileBindings[binding].m_controlIndex = 0;
				g_cpcDeviceFileBindings[binding].m_mode = 0;
			}
		}
	}

	sprintf(path, "giddi\\%s.cpc", InputGetDevice(g_curInputDeviceIdx)->m_info.m_shortName);
	file = MechFopen(path, "wb");
	if (file != NULL) {
		fwrite(g_cpcConfigName, sizeof(g_cpcConfigName), 1, file);
		fwrite(g_cpcDeviceSlots, sizeof(CpcDeviceSlot), c_deviceSlotCount, file);
		fwrite(g_cpcDeviceFileBindings, sizeof(g_cpcDeviceFileBindings), 1, file);
		fclose(file);
	}

	fclose(file);
}

// FUNCTION: MW2SHELL 0x10042b99
void CpcWarnControlCount(ScreenField* p_tab)
{
	if (p_tab) {
	}

	if (!CpcCheckControlCount()) {
		ShowDialog("Error: Sim can not|handle that many controls.#Ok", 0);
	}
}

// Opens the cockpit controls screen over the current one: its palette and logo, the devices
// (the keyboard is required), the configuration of the first slot, and its fields.
// FUNCTION: MW2SHELL 0x10042bcf
void OpenCockpitControls()
{
	InputDevice* device;
	MechS32 i;

	g_videoDriver->GetPalette(g_savedScreenPalette);
	g_videoDriver->LoadPalette(4);
	g_videoDriver->ActivateFramebuffer();
	g_cpcLogoMovie = NULL;
	g_cpcLogoMovie = new LoopingMovie("amwlogo1", 0x78, 4);
	g_videoDriver->DrawShell();
	g_videoDriver->m_restoreColor = 0;
	g_curCpcConfigSlot = 0;
	if (!InputEnumDevices(1)) {
		return;
	}

	g_cpcLegsPanSlot = -1;
	for (i = 0; i < c_deviceSlotCount; i++) {
		device = InputGetDevice(i);
		if (device) {
			g_cpcDeviceSlots[i].m_deviceId = i;
			strcpy(g_cpcDeviceSlots[i].m_name, device->m_info.m_matchName);
			if (!strcmp(device->m_info.m_matchName, "keyboard")) {
				g_cpcLegsPanSlot = i;
				g_curInputDeviceIdx = i;
				g_inputDeviceActive[i] = 1;
			}
			else {
				g_inputDeviceActive[i] = 0;
			}
		}
		else {
			g_cpcDeviceSlots[i].m_deviceId = -1;
			strcpy(g_cpcDeviceSlots[i].m_name, "");
			g_inputDeviceActive[i] = 0;
		}
	}

	if (g_cpcLegsPanSlot < 0) {
		ShowDialog("Error: keyboard not initialized.#Ok", 0);
		InputFreeDevices();
		return;
	}

	strcpy(g_cpcConfigName, "NO CONFIG");
	CpcResetAllBindings(NULL);
	CpcLoadConfigSlot(NULL);
	CpcValidateActiveDevices();
	g_cpcFirstButton = 0;
	g_cpcSelectedBinding = -1;
	g_cpcSelectedPart = 0;
	g_cpcConfigShown = 0;
	g_cpcShownBindings = &g_cpcBindings[g_cpcConfigShown * c_bindingCount];
	g_keyboardInput->FlushKeys();

	g_cpcSelectedColors[0] = 0xff;
	g_cpcSelectedColors[1] = 0x10;
	g_cpcDisabledColors[0] = 0xff;
	g_cpcDisabledColors[1] = 7;
	for (i = 2; i < 0x100; i++) {
		g_cpcSelectedColors[i] = i;
		g_cpcDisabledColors[i] = i;
	}

	ShowFields(g_cpcDevicesFields);
	g_cpcBindingsPage = 0;
	g_cpcConfigured = 0;
	g_inputConfigChanged = 0;
	RegisterMenuFunction(CpcScreenTick);
}

// The cockpit controls screen's frame, run over the screen below: the clicks on its fields (the
// right button too on the bindings page, g_cpcBindingsPage). Closes the screen when p_active is
// cleared, once configured, on the key code 3 or on a right click on the first page.
// FUNCTION: MW2SHELL 0x10042f65
void CpcScreenTick(MechS32 p_active)
{
	ScreenField* tab;

	if (p_active) {
		if (g_cpcLogoMovie) {
			g_cpcLogoMovie->Update();
		}

		if (g_mouseState->GetLeftPressed() == 1 || (g_cpcBindingsPage == 1 && g_mouseState->GetRightPressed() == 1)) {
			tab = FindFieldAt(
				g_cpcBindingsPage ? g_cpcBindingsFields : g_cpcDevicesFields,
				g_mouseState->m_x,
				g_mouseState->m_y
			);
			if (tab && tab->m_click) {
				tab->m_click(tab);
				RedrawFields(g_cpcBindingsPage ? g_cpcBindingsFields : g_cpcDevicesFields);
			}
		}
	}

	if (!p_active || g_cpcConfigured || g_keyboardInput->PollKey() == 3 ||
		(!g_cpcBindingsPage && g_mouseState->GetRightPressed() == 1)) {
		UnregisterMenuFunction(CpcScreenTick);
		EnableShellMenuCommand(c_menuCockpitControls, TRUE);
		g_menuDialogOpen = 0;
		HideFields(g_cpcBindingsPage ? g_cpcBindingsFields : g_cpcDevicesFields);
		if (g_cpcLogoMovie) {
			delete g_cpcLogoMovie;
		}
		g_cpcLogoMovie = NULL;
		InputFreeDevices();
		g_videoDriver->m_restoreColor = -1;
		g_videoDriver->RestoreBackground(0, 0, 640, 480);
		UpdateVideos();
		g_videoDriver->SetPalette(g_savedScreenPalette, 1);
		if (p_active) {
			g_videoDriver->DrawShell();
			g_videoDriver->RestoreBackground(0, 0, 640, 480);
			UpdateVideos();
		}
	}
}

// Explain a problem with the input devices in a message box.
// FUNCTION: MW2SHELL 0x100431b4
void CpcShowDeviceMessage(MechS32 p_message, MechChar*, MechChar* p_name)
{
	// Set and never read.
	undefined4 result = 0;

	switch (p_message) {
	case c_messageNoJoysticks:
		sprintf(
			g_cpcMessageText,
			"There are no joystick devices currently configured in the Windows Joystick Control Panel.  You must "
			"configure, calibrate, and test your joystick in the Control Panel before MechWarrior 2 can use it."
		);
		break;
	case c_messageMissingJoystick:
		sprintf(
			g_cpcMessageText,
			"The current Cockpit Controls configuration includes a joystick which no longer exists in the system "
			"or is not configured properly.\n\nTo eliminate the problem, you can chose \"ABORT\" from the Cockpit "
			"Controls screen and check the Windows Joystick Control Panel settings.\n\nOtherwise, this device will "
			"be ignored.  If you choose \"ACCEPT\",\nit will be removed from the Cockpit Controls configuration."
		);
		break;
	case c_messageRemapJoystick:
		sprintf(
			g_cpcMessageText,
			"The current Cockpit Controls configuration includes a joystick which no longer exists in the "
			"system.\n\nMechWarrior 2 will attempt to remap its controls to the \"%s\" from the Windows Joystick "
			"Control Panel.",
			p_name
		);
		break;
	default:
		sprintf(
			g_cpcMessageText,
			"An input device has caused an undefined error. Sorry, no other information is available."
		);
		break;
	}

	// The original showed it in a message box
	MechLogError(g_cpcMessageText);
}
