#include "options.h"

#include "audiosample.h"
#include "audiosubsystem.h"
#include "decomp.h"
#include "difficultyconfig.h"
#include "files.h"
#include "font.h"
#include "keyboardinput.h"
#include "loopingmovie.h"
#include "mainmenu.h"
#include "mechbay.h"
#include "mousestate.h"
#include "refreshmode.h"
#include "screenfield.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "soundconfig.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "video.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void OptionsCallback(MechS32);
extern ScreenField g_optionFields[16];

// A button with a centered caption. Nothing calls its two functions.
// SIZE 0x9c
struct PewterPlaque0x9c {
	MechS32 m_left;        // 0x00
	MechS32 m_top;         // 0x04
	MechS32 m_right;       // 0x08
	MechS32 m_bottom;      // 0x0c
	MechS32 m_textLeft;    // 0x10
	MechS32 m_textTop;     // 0x14
	MechChar m_text[0x80]; // 0x18
	undefined4 m_unk0x98;  // 0x98 — only cleared, by LayoutButton
};

DECOMP_SIZE_ASSERT(PewterPlaque0x9c, 0x9c)

// GLOBAL: MW2SHELL 0x10070d90
LoopingMovie* g_optionsMovie = NULL;

// GLOBAL: MW2SHELL 0x1007116c
MechChar g_optionsMovieName[0x10] = "amwlogo1";

DECOMP_SIZE_ASSERT(SoundConfig, 0x3c)
DECOMP_SIZE_ASSERT(DifficultyConfig, 0x17)

// GLOBAL: MW2SHELL 0x10092c18
AudioSample* g_volumeTestSample;
// GLOBAL: MW2SHELL 0x10092c30
PaletteColor g_savedPalette[0x100];
// GLOBAL: MW2SHELL 0x10092f30
void* g_sliderImages;

// GLOBAL: MW2SHELL 0x10070d98
MechChar* g_skillNames[] = {"~EASY", "~MEDIUM", "~HARD"};

// Lay out a button: its rectangle around a center, and its caption centered in it. Unused.
// FUNCTION: MW2SHELL 0x10043280
void LayoutButton(
	PewterPlaque0x9c* p_plaque,
	MechS32 p_centerX,
	MechS32 p_centerY,
	MechS32 p_width,
	MechS32 p_height,
	Font* p_font
)
{
	p_plaque->m_textLeft = p_centerX;
	p_plaque->m_textTop = p_centerY;
	p_plaque->m_left = p_plaque->m_textLeft - p_width / 2;
	p_plaque->m_right = p_plaque->m_left + p_width - 1;
	p_plaque->m_top = p_plaque->m_textTop - p_height / 2;
	p_plaque->m_bottom = p_plaque->m_top + p_height - 1;
	p_plaque->m_textTop -= p_font->m_height / 2;
	p_plaque->m_textLeft -= p_font->GetTextWidth(p_plaque->m_text) / 2;
	p_plaque->m_unk0x98 = 0;
}

// Whether a point lies in a button. Unused.
// FUNCTION: MW2SHELL 0x10043333
MechS32 IsPointInButton(PewterPlaque0x9c* p_plaque, MechS32 p_x, MechS32 p_y)
{
	return p_x >= p_plaque->m_left && p_x <= p_plaque->m_right && p_y >= p_plaque->m_top && p_y <= p_plaque->m_bottom;
}

// FUNCTION: MW2SHELL 0x1004338a
TextGlyph* DrawSkillOption(ScreenField* p_option)
{
	MechU8 value = *(MechU8*) p_option->m_data;

	return g_titleFont->AddText(p_option->m_left + p_option->m_width / 2, p_option->m_top, g_skillNames[value], NULL);
}

// FUNCTION: MW2SHELL 0x100433dc
TextGlyph* DrawIntToggle(ScreenField* p_option)
{
	MechS32 value = *(MechS32*) p_option->m_data;

	return g_titleFont->AddText(
		p_option->m_left + p_option->m_width / 2,
		p_option->m_top,
		(MechChar*) (value ? "~ON" : "~OFF"),
		NULL
	);
}

// FUNCTION: MW2SHELL 0x1004343c
TextGlyph* DrawByteToggle(ScreenField* p_option)
{
	MechU8 value = *(MechU8*) p_option->m_data;

	return g_titleFont->AddText(
		p_option->m_left + p_option->m_width / 2,
		p_option->m_top,
		(MechChar*) (value ? "~ON" : "~OFF"),
		NULL
	);
}

// FUNCTION: MW2SHELL 0x1004349f
TextGlyph* DrawDishonorableToggle(ScreenField* p_option)
{
	MechU8 value = *(MechU8*) p_option->m_data;

	return g_titleFont->AddText(
		p_option->m_left + p_option->m_width / 2 - (value ? 14 : 0),
		p_option->m_top,
		(MechChar*) (value ? "ON (Dishonorable)" : "~OFF"),
		NULL
	);
}

// FUNCTION: MW2SHELL 0x10043517
TextGlyph* DrawInvertedDishonorableToggle(ScreenField* p_option)
{
	MechU8 value = *(MechU8*) p_option->m_data;

	return g_titleFont->AddText(
		p_option->m_left + p_option->m_width / 2 - (value ? 0 : 14),
		p_option->m_top,
		(MechChar*) (value ? "~OFF" : "ON (Dishonorable)"),
		NULL
	);
}

// FUNCTION: MW2SHELL 0x10043589
TextGlyph* DrawHighLowToggle(ScreenField* p_option)
{
	MechS32 value = *(MechS32*) p_option->m_data;

	return g_titleFont->AddText(
		p_option->m_left + p_option->m_width / 2,
		p_option->m_top,
		(MechChar*) (value ? "~HIGH" : "~LOW"),
		NULL
	);
}

// FUNCTION: MW2SHELL 0x100435e9
TextGlyph* DrawResolutionOption(ScreenField* p_option)
{
	MechChar* value;
	MechChar* label;

	value = (MechChar*) p_option->m_data;
	if (*value == '\0') {
		label = "~320x200";
	}
	else {
		label = "~640x480";
	}

	return g_titleFont->AddText(p_option->m_left + p_option->m_width / 2, p_option->m_top, label, NULL);
}

// FUNCTION: MW2SHELL 0x10043651
void CycleByteOption(ScreenField* p_option)
{
	MechU8* value;

	value = (MechU8*) p_option->m_data;
	++*value;
	if (*value >= 3) {
		*value = 0;
	}
}

// FUNCTION: MW2SHELL 0x10043688
void ToggleIntOption(ScreenField* p_toggle)
{
	MechS32* value;

	value = (MechS32*) p_toggle->m_data;
	if (*value) {
		*value = 0;
	}
	else {
		*value = 1;
	}
}

// FUNCTION: MW2SHELL 0x100436c7
void ToggleByteOption(ScreenField* p_option)
{
	MechU8* value;

	value = (MechU8*) p_option->m_data;
	if (*value) {
		*value = 0;
	}
	else {
		*value = 1;
	}
}

// FUNCTION: MW2SHELL 0x10043703
void ToggleVesaDriver(ScreenField* p_option)
{
	MechChar* value = (MechChar*) p_option->m_data;
	if (*value == '\0') {
		strncpy(value, "vesa480.dll", 0xf);
	}
	else {
		strncpy(value, "", 0xf);
	}
}

// FUNCTION: MW2SHELL 0x10043758
TextGlyph* RestoreFieldBackground(ScreenField* p_option)
{
	g_videoDriver->RestoreBackground(p_option->m_left, p_option->m_top, p_option->m_width, p_option->m_height);

	return NULL;
}

// FUNCTION: MW2SHELL 0x10043790
TextGlyph* DrawVolumeSlider(ScreenField* p_option)
{
	MechS32 position;

	position = *(MechS32*) p_option->m_data;
	position /= 0x100;
	g_videoDriver
		->DrawShpFrame(g_sliderImages, 1, p_option->m_left, p_option->m_top, p_option->m_width, p_option->m_height);
	g_videoDriver->DrawShpFrame(g_sliderImages, 0, p_option->m_left + position + 8, p_option->m_top - 1, 0xf, 0x1d);

	return NULL;
}

// Drags a volume slider while the left button is held: the value follows the mouse, and a
// change of 0xa00 or more plays the test sample at the new volume.
// Not 100%: the stack slots of previous, value and volume are permuted.
// FUNCTION: MW2SHELL 0x1004381b
void DragVolumeSlider(ScreenField* p_option)
{
	MechS32 previous;
	MechS32* value;
	MechS32 volume;

	value = (MechS32*) p_option->m_data;
	previous = *value;
	do {
		*value = g_mouseState->m_x - (p_option->m_left + 0xf);
		if (*value < 0) {
			*value = 0;
		}
		else if (*value > 0x100) {
			*value = 0x100;
		}
		*value <<= 8;

		if (abs(previous - *value) >= 0xa00) {
			previous = *value;
			volume = g_soundConfig.m_effectsVolume;
			g_soundConfig.m_effectsVolume = previous;
			g_volumeTestSample->Start();
			g_soundConfig.m_effectsVolume = volume;
		}

		RedrawFields(g_optionFields);
		if (g_optionsMovie) {
			g_optionsMovie->Update();
		}
		g_mouseState->ReadMouseState();
		g_videoDriver->DrawShell();
		g_audioSubsystem->ApplyMidiVolume();
	} while (g_mouseState->m_leftDown == 1);
}

// FUNCTION: MW2SHELL 0x10043926
void LoadSoundConfig()
{
	FILE* file;

	file = MechFopen("MW2SND.CFG", "rb");
	if (file != NULL) {
		fread(&g_soundConfig, sizeof(g_soundConfig), 1, file);
		fclose(file);
	}
}

// FUNCTION: MW2SHELL 0x10043979
void LoadDifficultyConfig()
{
	FILE* file;

	file = MechFopen("MW2DIF.CFG", "rb");
	if (file != NULL) {
		fread(&g_difficultyConfig, sizeof(g_difficultyConfig), 1, file);
		fclose(file);
	}
}

// FUNCTION: MW2SHELL 0x100439cc
void SaveDifficultyConfig()
{
	FILE* file;

	file = MechFopen("MW2DIF.CFG", "wb");
	if (file != NULL) {
		fwrite(&g_difficultyConfig, sizeof(g_difficultyConfig), 1, file);
		fclose(file);
	}
}

// FUNCTION: MW2SHELL 0x10043a1f
void SaveSoundConfig()
{
	FILE* file;

	file = MechFopen("MW2SND.CFG", "wb");
	if (file != NULL) {
		fwrite(&g_soundConfig, sizeof(g_soundConfig), 1, file);
		fclose(file);
	}
}

// Stack-slot permutation: original paletteSize is at [ebp-0x10] and audioSize at
// [ebp-0x14]; VC++ assigns them [ebp-0x14] and [ebp-0x10] here.
// FUNCTION: MW2SHELL 0x10043a72
void DrawOptions()
{
	MechS32 paletteSize;
	void* audioData;
	MechS32 audioSize;
	g_videoDriver->GetPalette(g_savedPalette);
	g_mw2Database->GetDBItem(8, &g_sliderImages, &paletteSize);
	g_mw2Database->GetDBItem(0x66, &audioData, &audioSize);
	g_volumeTestSample = new AudioSample(g_audioSubsystem, audioData, audioSize);
	g_volumeTestSample->SetVolume(0x32);
	g_videoDriver->LoadPalette(3);
	g_videoDriver->m_restoreColor = 0;
	g_videoDriver->RestoreBackground(0x177, 0x7c, 0x102, 0x160);
	g_optionsMovie = NULL;
	g_optionsMovie = new LoopingMovie(g_optionsMovieName, 0x78, 4);
	g_keyboardInput->FlushKeys();
	LoadSoundConfig();
	LoadDifficultyConfig();
	ShowFields(g_optionFields);
	RegisterMenuFunction(OptionsCallback);
}

// The options screen's per-frame callback: handles clicks on the options, and closes the
// screen on a right click, a key, or when called with p_active FALSE, saving the settings.
// FUNCTION: MW2SHELL 0x10043c1f
void OptionsCallback(MechS32 p_active)
{
	ScreenField* option;

	if (p_active) {
		if (g_optionsMovie) {
			g_optionsMovie->Update();
		}

		if (g_mouseState->GetLeftPressed() == 1) {
			option = FindFieldAt(g_optionFields, g_mouseState->m_x, g_mouseState->m_y);
			if (option && option->m_click) {
				option->m_click(option);
				RedrawFields(g_optionFields);
			}
		}
	}

	if (!p_active || g_mouseState->GetRightPressed() == 1 || g_keyboardInput->PollKey()) {
		UnregisterMenuFunction(OptionsCallback);
		EnableShellMenuCommand(c_menuCombatVariables, TRUE);
		g_menuDialogOpen = 0;
		HideFields(g_optionFields);
		SaveSoundConfig();
		SaveDifficultyConfig();

		if (g_optionsMovie) {
			delete g_optionsMovie;
		}
		if (g_volumeTestSample) {
			delete g_volumeTestSample;
		}

		g_videoDriver->m_restoreColor = -1;
		g_videoDriver->RestoreBackground(0, 0, 0x280, 0x1e0);
		UpdateVideos();
		g_videoDriver->SetPalette(g_savedPalette, 1);

		if (p_active) {
			g_videoDriver->DrawShell();
			g_videoDriver->RestoreBackground(0, 0, 0x280, 0x1e0);
			UpdateVideos();
		}
	}
}

// Callbacks and value pointers come from the original 15-entry options table.
#define OPTION_ROW(x, y, width, draw, click, value) {x, y, width, -1, 0, NULL, NULL, draw, click, value, NULL}
#define OPTION_BAR(x, y, width, height, draw, click, value)                                                            \
	{x, y, width, height, 0, NULL, NULL, draw, click, value, NULL}
// GLOBAL: MW2SHELL 0x10070da8
ScreenField g_optionFields[16] = {
	OPTION_ROW(0x189, 0xdb, 100, DrawSkillOption, CycleByteOption, &g_difficultyConfig.m_enemySkill),
	OPTION_ROW(0x189, 0xef, 100, DrawByteToggle, ToggleByteOption, &g_difficultyConfig.m_heatTracking),
	OPTION_ROW(0x189, 0x115, 100, DrawIntToggle, ToggleIntOption, &g_soundConfig.m_objectTextmaps),
	OPTION_ROW(0x189, 0x129, 100, DrawIntToggle, ToggleIntOption, &g_soundConfig.m_terrainTextmaps),
	OPTION_ROW(0x189, 0x13d, 100, DrawHighLowToggle, ToggleIntOption, &g_soundConfig.m_displayDetail),
	OPTION_ROW(0x189, 0x151, 100, DrawHighLowToggle, ToggleIntOption, &g_soundConfig.m_objectDensity),
	OPTION_ROW(0x189, 0x165, 100, DrawIntToggle, ToggleIntOption, &g_soundConfig.m_explosionChunks),
	OPTION_ROW(0x189, 0x179, 100, DrawResolutionOption, ToggleVesaDriver, g_soundConfig.m_videoDriver),
	OPTION_ROW(0x189, 0x1a0, 100, DrawDishonorableToggle, ToggleByteOption, &g_difficultyConfig.m_invulnerability),
	OPTION_ROW(0x189, 0x1b4, 100, DrawDishonorableToggle, ToggleByteOption, &g_difficultyConfig.m_unlimitedAmmo),
	OPTION_ROW(
		0x189,
		0x1c8,
		100,
		DrawInvertedDishonorableToggle,
		ToggleByteOption,
		&g_difficultyConfig.m_collisionDamage
	),
	OPTION_BAR(0x14f, 0x80, 0x11d, 0x4e, RestoreFieldBackground, NULL, NULL),
	OPTION_BAR(0x14f, 0x80, 0x11d, 0x15, DrawVolumeSlider, DragVolumeSlider, &g_soundConfig.m_midiVolume),
	OPTION_BAR(0x14f, 0x98, 0x11d, 0x15, DrawVolumeSlider, DragVolumeSlider, &g_soundConfig.m_effectsVolume),
	OPTION_BAR(0x14f, 0xb0, 0x11d, 0x15, DrawVolumeSlider, DragVolumeSlider, &g_soundConfig.m_voiceVolume),
	{-1, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL},
};
#undef OPTION_ROW
#undef OPTION_BAR
