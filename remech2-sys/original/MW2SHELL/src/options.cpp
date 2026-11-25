#include "options.h"

#include "app.h"
#include "audiosample.h"
#include "audiosubsystem.h"
#include "difficultyconfig.h"
#include "font.h"
#include "gamesettings.h"
#include "keyboardinput.h"
#include "loopingmovie.h"
#include "mechbay.h"
#include "mousestate.h"
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

void OptionsCallback(int32_t);
extern ScreenField g_optionFields[18];

LoopingMovie* g_optionsMovie = NULL;

char g_optionsMovieName[0x10] = "amwlogo1";

AudioSample* g_volumeTestSample;
PaletteColor g_savedPalette[0x100];
void* g_sliderImages;

char* g_skillNames[] = {"~EASY", "~MEDIUM", "~HARD"};

TextGlyph* DrawSkillOption(ScreenField* p_option)
{
	uint8_t value = *(uint8_t*) p_option->m_data;

	return g_titleFont->AddText(p_option->m_left + p_option->m_width / 2, p_option->m_top, g_skillNames[value], NULL);
}

TextGlyph* DrawIntToggle(ScreenField* p_option)
{
	int32_t value = *(int32_t*) p_option->m_data;

	return g_titleFont
		->AddText(p_option->m_left + p_option->m_width / 2, p_option->m_top, (char*) (value ? "~ON" : "~OFF"), NULL);
}

TextGlyph* DrawByteToggle(ScreenField* p_option)
{
	uint8_t value = *(uint8_t*) p_option->m_data;

	return g_titleFont
		->AddText(p_option->m_left + p_option->m_width / 2, p_option->m_top, (char*) (value ? "~ON" : "~OFF"), NULL);
}

TextGlyph* DrawDishonorableToggle(ScreenField* p_option)
{
	uint8_t value = *(uint8_t*) p_option->m_data;

	return g_titleFont->AddText(
		p_option->m_left + p_option->m_width / 2 - (value ? 14 : 0),
		p_option->m_top,
		(char*) (value ? "ON (Dishonorable)" : "~OFF"),
		NULL
	);
}

TextGlyph* DrawInvertedDishonorableToggle(ScreenField* p_option)
{
	uint8_t value = *(uint8_t*) p_option->m_data;

	return g_titleFont->AddText(
		p_option->m_left + p_option->m_width / 2 - (value ? 0 : 14),
		p_option->m_top,
		(char*) (value ? "~OFF" : "ON (Dishonorable)"),
		NULL
	);
}

TextGlyph* DrawHighLowToggle(ScreenField* p_option)
{
	int32_t value = *(int32_t*) p_option->m_data;

	return g_titleFont
		->AddText(p_option->m_left + p_option->m_width / 2, p_option->m_top, (char*) (value ? "~HIGH" : "~LOW"), NULL);
}

// DrawResolutionOption and ToggleRenderResolution are implemented on the Rust side
// (src/shell/screens/settings.rs)

void CycleByteOption(ScreenField* p_option)
{
	uint8_t* value;

	value = (uint8_t*) p_option->m_data;
	++*value;
	if (*value >= 3) {
		*value = 0;
	}
}

void ToggleIntOption(ScreenField* p_toggle)
{
	int32_t* value;

	value = (int32_t*) p_toggle->m_data;
	if (*value) {
		*value = 0;
	}
	else {
		*value = 1;
	}
}

void ToggleByteOption(ScreenField* p_option)
{
	uint8_t* value;

	value = (uint8_t*) p_option->m_data;
	if (*value) {
		*value = 0;
	}
	else {
		*value = 1;
	}
}

TextGlyph* RestoreFieldBackground(ScreenField* p_option)
{
	g_videoDriver->RestoreBackground(p_option->m_left, p_option->m_top, p_option->m_width, p_option->m_height);

	return NULL;
}

TextGlyph* DrawVolumeSlider(ScreenField* p_option)
{
	int32_t position;

	position = *(int32_t*) p_option->m_data;
	position /= 0x100;
	g_videoDriver
		->DrawShpFrame(g_sliderImages, 1, p_option->m_left, p_option->m_top, p_option->m_width, p_option->m_height);
	g_videoDriver->DrawShpFrame(g_sliderImages, 0, p_option->m_left + position + 8, p_option->m_top - 1, 0xf, 0x1d);

	return NULL;
}

// Drags a volume slider while the left button is held: the value follows the mouse, and a
// change of 0xa00 or more plays the test sample at the new volume.
void DragVolumeSlider(ScreenField* p_option)
{
	int32_t previous;
	int32_t* value;
	int32_t volume;

	value = (int32_t*) p_option->m_data;
	previous = *value;
	do {
		MechAppPump();
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

void LoadSoundConfig()
{
	SoundSettings sound;
	DisplaySettings display;

	MechGetSoundSettings(&sound);
	MechGetDisplaySettings(&display);
	g_soundConfig.m_effectsVolume = sound.m_effectsVolume;
	g_soundConfig.m_voiceVolume = sound.m_voiceVolume;
	g_soundConfig.m_midiVolume = sound.m_musicVolume;
	g_soundConfig.m_objectTextmaps = display.m_objectTextmaps;
	g_soundConfig.m_terrainTextmaps = display.m_terrainTextmaps;
	g_soundConfig.m_displayDetail = display.m_highDetail;
	g_soundConfig.m_objectDensity = display.m_highObjectDensity;
	g_soundConfig.m_explosionChunks = display.m_explosionChunks;
	g_soundConfig.m_displayBrightness = display.m_brightness;
}

void LoadDifficultyConfig()
{
	DifficultySettings settings;

	MechGetDifficultySettings(&settings);
	g_difficultyConfig.m_unlimitedAmmo = settings.m_unlimitedAmmo;
	g_difficultyConfig.m_invulnerability = settings.m_invulnerable;
	g_difficultyConfig.m_splashDamage = settings.m_splashDamage;
	g_difficultyConfig.m_collisionDamage = settings.m_collisionDamage;
	g_difficultyConfig.m_heatTracking = settings.m_heatTracking;
	g_difficultyConfig.m_enemySkill = settings.m_enemySkill;
}

void SaveDifficultyConfig()
{
	DifficultySettings settings;

	settings.m_unlimitedAmmo = g_difficultyConfig.m_unlimitedAmmo;
	settings.m_invulnerable = g_difficultyConfig.m_invulnerability;
	settings.m_splashDamage = g_difficultyConfig.m_splashDamage;
	settings.m_collisionDamage = g_difficultyConfig.m_collisionDamage;
	settings.m_heatTracking = g_difficultyConfig.m_heatTracking;
	settings.m_enemySkill = g_difficultyConfig.m_enemySkill;
	MechSetDifficultySettings(&settings);
}

void SaveSoundConfig()
{
	SoundSettings sound;
	DisplaySettings display;

	sound.m_effectsVolume = g_soundConfig.m_effectsVolume;
	sound.m_voiceVolume = g_soundConfig.m_voiceVolume;
	sound.m_musicVolume = g_soundConfig.m_midiVolume;
	display.m_objectTextmaps = g_soundConfig.m_objectTextmaps;
	display.m_terrainTextmaps = g_soundConfig.m_terrainTextmaps;
	display.m_highDetail = g_soundConfig.m_displayDetail;
	display.m_highObjectDensity = g_soundConfig.m_objectDensity;
	display.m_explosionChunks = g_soundConfig.m_explosionChunks;
	display.m_brightness = g_soundConfig.m_displayBrightness;
	MechSetSoundSettings(&sound);
	MechSetDisplaySettings(&display);
}

void DrawOptions()
{
	int32_t paletteSize;
	void* audioData;
	int32_t audioSize;
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
// screen on a right click, a key, or when called with p_active false, saving the settings.
void OptionsCallback(int32_t p_active)
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
		EnableShellMenuCommand(c_menuCombatVariables, true);
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

// Callbacks and value pointers come from the original 15-entry options table. The widescreen row
// is new, in the empty row below the resolution, with a label the screen's art doesn't have.
#define OPTION_ROW(x, y, width, draw, click, value) {x, y, width, -1, NULL, NULL, draw, click, value, NULL}
#define OPTION_BAR(x, y, width, height, draw, click, value) {x, y, width, height, NULL, NULL, draw, click, value, NULL}
// A label right-aligned at x 0x16a, where the art's labels end
#define OPTION_LABEL(y, text) {0xb4, y, 0xb6, 0x13, NULL, NULL, DrawOptionLabel, NULL, (void*) text, NULL}
ScreenField g_optionFields[18] = {
	OPTION_ROW(0x189, 0xdb, 100, DrawSkillOption, CycleByteOption, &g_difficultyConfig.m_enemySkill),
	OPTION_ROW(0x189, 0xef, 100, DrawByteToggle, ToggleByteOption, &g_difficultyConfig.m_heatTracking),
	OPTION_ROW(0x189, 0x115, 100, DrawIntToggle, ToggleIntOption, &g_soundConfig.m_objectTextmaps),
	OPTION_ROW(0x189, 0x129, 100, DrawIntToggle, ToggleIntOption, &g_soundConfig.m_terrainTextmaps),
	OPTION_ROW(0x189, 0x13d, 100, DrawHighLowToggle, ToggleIntOption, &g_soundConfig.m_displayDetail),
	OPTION_ROW(0x189, 0x151, 100, DrawHighLowToggle, ToggleIntOption, &g_soundConfig.m_objectDensity),
	OPTION_ROW(0x189, 0x165, 100, DrawIntToggle, ToggleIntOption, &g_soundConfig.m_explosionChunks),
	OPTION_ROW(0x189, 0x179, 100, DrawResolutionOption, ToggleRenderResolution, NULL),
	OPTION_ROW(0x189, 0x18d, 100, DrawWidescreenOption, ToggleWidescreen, NULL),
	OPTION_LABEL(0x18d, "WIDESCREEN"),
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
	{-1, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL},
};
#undef OPTION_ROW
#undef OPTION_BAR
#undef OPTION_LABEL
