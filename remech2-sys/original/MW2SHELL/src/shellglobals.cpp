#include "shellglobals.h"

#include "audiosubsystem.h"
#include "difficultyconfig.h"
#include "font.h"
#include "keyboardinput.h"
#include "mousestate.h"
#include "palettecolor.h"
#include "pilotrecord.h"
#include "projectarchive.h"
#include "soundconfig.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "videodriver.h"

// The shell's global state. The original links this data as an object of its own (or as
// popuppicture.cpp's data, which has none otherwise), between page.cpp and brightness.c:
// its globals, then the rank and clan names they point to; the pilot roster is its .bss.

// GLOBAL: MW2SHELL 0x100711f8
KeyboardInput* g_keyboardInput = NULL;

// GLOBAL: MW2SHELL 0x100711fc
AudioSubsystem* g_audioSubsystem = NULL;

// The mouse cursor's shape, 29 by 25: database item 0x19. ShellMain loads it and passes it to
// MouseState, which stores it and never reads it. The overlay draws it (src/shell/overlay/ui.rs).
// GLOBAL: MW2SHELL 0x10071200
void* g_cursorShape = NULL;

// GLOBAL: MW2SHELL 0x10071204
MouseState* g_mouseState = NULL;

// GLOBAL: MW2SHELL 0x10071208
VideoDriver* g_videoDriver = NULL;

// The shell's fonts, loaded by ShellMain from the database. ShellMain loads item 0x1a into
// g_textFont, then points g_defaultFont, g_textFont and g_archiveFont at g_bodyFont (item 0x20),
// so those four draw in the same typeface; the first item 0x1a font is leaked.
// The body text: most likely Eurostile Regular or Square 721 Roman.
// GLOBAL: MW2SHELL 0x1007120c
Font* g_defaultFont = NULL;

// GLOBAL: MW2SHELL 0x10071210
Font* g_textFont = NULL;

// Titles and headings (item 0x1b): Helvetica Bold.
// GLOBAL: MW2SHELL 0x10071214
Font* g_titleFont = NULL;

// The menu buttons (item 0x1c): Bank Gothic Bold. ButtonMenu always uses it.
// GLOBAL: MW2SHELL 0x10071218
Font* g_buttonFont = NULL;

// Items 0x1e and 0x1f: loaded, never used, so they keep their placeholders.
// GLOBAL: MW2SHELL 0x1007121c
Font* g_unk0x1007121c = NULL;

// GLOBAL: MW2SHELL 0x10071220
Font* g_unk0x10071220 = NULL;

// The archive reader's font, in the archive, the briefing and the debriefing.
// GLOBAL: MW2SHELL 0x10071224
Font* g_archiveFont = NULL;

// Item 0x20, the body text font the others alias.
// GLOBAL: MW2SHELL 0x10071228
Font* g_bodyFont = NULL;

// GLOBAL: MW2SHELL 0x1007122c
TMPackDataBase* g_mw2Database = NULL;

// GLOBAL: MW2SHELL 0x10071230
ProjectArchive* g_projectArchive = NULL;

// GLOBAL: MW2SHELL 0x10071234
MechS32 g_midiAudio = 1;

// GLOBAL: MW2SHELL 0x10071238
MechS32 g_digitalAudio = 1;

// Cleared with the audio flags when there is no command line; nothing reads it.
// GLOBAL: MW2SHELL 0x1007123c
MechS32 g_unk0x1007123c = 1;

// Cleared when the shell runs without a command line, outside MECH2.EXE: LAUNCH then skips the
// simulator (ShellWindowProc posts the result message straight away) and the mouse shows its
// position.
// GLOBAL: MW2SHELL 0x10071240
MechS32 g_runSim = 1;

// The flags the shell opens Smacker movies with.
// GLOBAL: MW2SHELL 0x10071248
MechU32 g_movieOpenFlags = 0;

// GLOBAL: MW2SHELL 0x1007124c
MechU8 g_drawFmv = 0;

// The pilot roster clamps rank + 1 and rank + 2 to index 9: the NULL after Khan.
// GLOBAL: MW2SHELL 0x10071258
MechChar* g_rankNames[10] = {
	"Mechwarrior",
	"Star Commander",
	"Nova Commander",
	"Star Captain",
	"Nova Captain",
	"Star Colonel",
	"Nova Colonel",
	"Galaxy Commander",
	"Khan",
	NULL,
};

// GLOBAL: MW2SHELL 0x10071280
MechChar* g_clanNames[6] = {"Wolf", "Jade Falcon", "Ghost Bear", "Smoke Jaguar", "Nova Cat", "Steel Vipers"};

// The song of each shell message from c_msgBriefing (0x406) up, per campaign (PlayMidiSong): a
// database item (plus the base), 0 to keep the current one, 0x20000000 to stop the music.
// 0x10000000 restarts the song. The Trials of Grievance's...
// GLOBAL: MW2SHELL 0x10071298
MechS32 g_trialsSongs[18] =
	{0x23, 0, 0, 0, 0x20000000, 0, 0, 0x23, 0x20000000, 0x23, 0x20000000, 0, 0, 0x23, 0, 0x20000000, 0x20000000, 0};

// ...Wolf's...
// GLOBAL: MW2SHELL 0x100712e0
MechS32 g_wolfSongs[18] = {
	0x25,
	0x24,
	0,
	0,
	0x20000000,
	0x24,
	0,
	0x23,
	0x20000000,
	0x25,
	0x20000000,
	0x25,
	0x24,
	0x25,
	0x26,
	0x20000000,
	0x20000000,
	0
};

// ...and Jade Falcon's.
// GLOBAL: MW2SHELL 0x10071328
MechS32 g_jadeFalconSongs[18] = {
	0x28,
	0x27,
	0,
	0,
	0x20000000,
	0x27,
	0,
	0x23,
	0x20000000,
	0x28,
	0x20000000,
	0x28,
	0x27,
	0x28,
	0x29,
	0x20000000,
	0x20000000,
	0
};

// GLOBAL: MW2SHELL 0x10071370
PilotRecord* g_currentPilot = NULL;

// Set when a new pilot is registered, for the clan hall's welcome (g_welcomeSound).
// GLOBAL: MW2SHELL 0x10071374
MechS32 g_newPilotRegistered = 0;

// The palette the leaderboard, credits and cockpit controls screens, which load palettes of
// their own, restore on exit.
// GLOBAL: MW2SHELL 0x10071378
PaletteColor g_savedScreenPalette[0x100] = {0};

// The sound settings (MW2SND.CFG).
// GLOBAL: MW2SHELL 0x10071678
SoundConfig g_soundConfig = {0x10000, 0x10000, 0x10000, 0x10000, 0xf, 1, 1, 1, 1, 1, 8, {0}};

// The difficulty settings (MW2DIF.CFG).
// GLOBAL: MW2SHELL 0x100716b8
DifficultyConfig g_difficultyConfig = {0, 0, 1, 1, 1, 1, {0, 0, 0, 1}};

// GLOBAL: MW2SHELL 0x100946d0
PilotRecord g_pilotRoster[20];
