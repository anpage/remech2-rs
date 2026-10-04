/* Menu actions (the eject menu) and the "dorcs" sequence's data files. */
#include "dorcs.h"

#include "clock.h"
#include "cockpit.h"
#include "config.h"
#include "displaybackend.h"
#include "gamekeys.h"
#include "hud.h"
#include "mechdamage.h"
#include "menu.h"
#include "menucontrol.h"
#include "menucontrols.h"
#include "menupage.h"
#include "menutextbox.h"
#include "mss.h"
#include "palette.h"
#include "palettecolor.h"
#include "players.h"
#include "polydraw.h"
#include "recttransition.h"
#include "refreshmode.h"
#include "render.h"
#include "rendersettings.h"
#include "screenscale.h"
#include "simmain.h"
#include "soundfx.h"
#include "targeting.h"
#include "types.h"
#include "vfxa.h"

#include <stdio.h>
#include <windows.h>

// GLOBAL: MW2 0x100ae760
MechChar g_dorcsPageTitle[] = "MW2 Programmer Dorcs Page";

// GLOBAL: MW2 0x100ae780
MechChar g_dorcsAboutItem[] = "About Dorcs";

// GLOBAL: MW2 0x100ae790
MechChar g_dorcsClarkeName[] = "John A. Clarke";

// GLOBAL: MW2 0x100ae7a0
MechChar g_dorcsDouglasName[] = "Michael H. Douglas";

// GLOBAL: MW2 0x100ae7b8
MechChar g_dorcsEthertonName[] = "Scott T. Etherton";

// GLOBAL: MW2 0x100ae7d0
MechChar g_dorcsHusebyName[] = "Sverre H. Huseby";

// GLOBAL: MW2 0x100ae7e8
MechChar g_dorcsKaminsName[] = "Dan Kamins";

// GLOBAL: MW2 0x100ae7f8
MechChar g_dorcsKeatingName[] = "John Keating";

// GLOBAL: MW2 0x100ae808
MechChar g_dorcsMilesName[] = "John Miles";

// GLOBAL: MW2 0x100ae818
MechChar g_dorcsMortenName[] = "Tim Morten";

// GLOBAL: MW2 0x100ae828
MechChar g_dorcsMortensenName[] = "Bob Mortensen";

// GLOBAL: MW2 0x100ae838
MechChar g_dorcsPetersonName[] = "Eric Peterson";

// GLOBAL: MW2 0x100ae848
MechChar g_dorcsStanfillName[] = "Dan Stanfill";

// GLOBAL: MW2 0x100ae858
MechChar g_dorcsWhiteName[] = "David White";

// GLOBAL: MW2 0x100ae868
MechChar g_dorcsZobelName[] = "Dave Zobel";

// GLOBAL: MW2 0x100ae878
MechChar g_dorcsAboutText[] =
	"Hi MW2 Fans,\n"
	"\n"
	"Welcome to the MW2 Sim Programmer Dorcs Web page.  We hope you'll spend a few minutes here to learn a bit about "
	"us.  Basically, we are just a bunch of guys who love to code. We get positively giddi watching the source files "
	"stream in after a well timed dorcs co.  We spend hours debating age old questions such as \"Which came first wasm "
	"or unwasm?\", \"Why do our game designers insist on using DOS Edit?\" and \"What does booyow mean?\".  Uh oh, "
	"here comes the \"ALL CLEAR FOR DORCS!\" signal.  We better get back to coding.\n"
	"\n"
	"So long for now, \n"
	"\n"
	"MW2 Programmers";

// GLOBAL: MW2 0x100aeab8
MechChar g_dorcsClarkeText[] =
	"Hi MW2 fans,\n"
	"\n"
	"I can't believe it finally over.  My tour of duty has been 18 months long.  I am looking forward to spending lots "
	"of time with my wife Carol Codekas and our son John Taylor.  Taylor became the official MechBaby when he was born "
	"seven months ago.  The delivery went smoothly except that in all the excitement I dropped my laptop and lost a "
	"few days worth of source code.\n"
	"\n"
	"Thanks to Bill, Dr. Bob and HP for keeping me sane and sober. Thanks Carol for your patience and Grandma Clarke "
	"and Codekas for babysitting Taylor. Hi John Clarke Sr! I have one last announcement for old time's sake.\n"
	"\n"
	"ATTENTION ALL PROGRAMMERS! ALL CLEAR FOR DORCS CO! GET GIDDI TOO!";

// GLOBAL: MW2 0x100aed58
MechChar g_dorcsDouglasText[] =
	"    If I had known six months ago how crazy working on MWII was going to be, I'm not sure I would have done it. "
	"But, after countless late nights and thousands of ornery bugs, we've finally got a killer game.\n"
	"    I'd like to thank John Spinale and Josh Resnick for bringing me on. They've put together the best development "
	"team anywhere, and I'm glad to have the opportunity to work with them.\n"
	"    Most of all, I'd like to thank my wife Rosaline for her infinite patience. Not everyone would understand "
	"postponing a honeymoon for a game.\n"
	"\n"
	"-- Michael H. Douglas     DEI/FEIF\n"
	"    mdouglas@activision.com\n"
	"\n"
	"P.S. - No, I'm not THAT Michael Douglas.";

// GLOBAL: MW2 0x100aefe0
MechChar g_dorcsEthertonText[] = "I just love to code.  That's all there is to it.";

// GLOBAL: MW2 0x100af018
MechChar g_dorcsHusebyText[] =
	"We don't know much about this guy except that he lives in Norway and probably has a thick Norwegian accent.  He "
	"wrote some handy freeware called GifSave and was kind enough to post it on the net.  You can thank him personally "
	"at sverrehu@ifi.uio.no for the cool high res screen shots.\n";

// GLOBAL: MW2 0x100af138
MechChar g_dorcsKaminsText[] =
	"  Thanks to all!\n"
	"  I'd take better advantage of my DORCSOpportunity (TM) here, except for the fact that I have 34 bugs to fix.\n"
	"  By the way, I wrote the NetDemo network shell.\n"
	"  If you liked it, I'd love to hear from you.\n"
	"--\n"
	"dkamins@husc.harvard.edu\n";

// GLOBAL: MW2 0x100af238
MechChar g_dorcsKeatingText[] =
	"John drew on his experience defending his turf and scrapping in the gutters as a kid in San Antonio's roughest "
	"neighborhood, \"Alamo Heights\", to create the battle AI for MechWarrior.  A Mac guy living in a hostile PC "
	"world, he is naturally short tempered and combative.  For the part of drill instructor John was influenced "
	"heavily by his big sister, Paula, as well as huge doses of MSG.  He welcomes your criticisms, offers of sympathy, "
	"or any reply from those lacking a \"Y\" chromosome at Marshall1@aol.com.";

// GLOBAL: MW2 0x100af438
MechChar g_dorcsMilesText[] =
	"This guy wrote all the graphics and sound packages (along with John Lemberger) We could dorcs co his API with "
	"confidence because we knew we were getting high quality, efficient and reliable code.  Once we converted to the "
	"world of PANES, high res was easy.  The DLL loader just made us giddi.  We even used it for these menus.\n"
	"\n"
	"Thanks to to John for all the support.";

// GLOBAL: MW2 0x100af5a8
MechChar g_dorcsMortenText[] =
	"Tim's Liner Notes\n"
	"\n"
	"Is it done?  Can I leave my cube?  Where is everybody?\n"
	"\n"
	"I'm grateful for the friendship of all the people I've worked with on this project, from start to finish.  This "
	"project was a labor of love (and at times, war) for everyone who worked on it.\n"
	"\n"
	"Do you like camping?\n"
	"\n"
	"Tim Morten";

// GLOBAL: MW2 0x100af6d8
MechChar g_dorcsMortensenText[] =
	"I'm so HONORED!  Only three months on the project and I'm an official DORCS.\n"
	"\n"
	"Of course I'll have to thank my wife and kids, that is if I can ever remember their names....  Hmmmm, oh yeah.... "
	"Thanks Debra (8) and Timmy (4).  But mostly thanks to Helen (], with out your support and help I never would have "
	"been able to contribute as much as I have to this ROCKING GAME!\n"
	"\n"
	"Later,\n"
	"BobM";

// GLOBAL: MW2 0x100af858
MechChar g_dorcsPetersonText[] =
	"Eric no longer works here but he is really the grand-pappy of the Sim engine.  The original concept was his and "
	"he single-handedly wrote the first real mode version (including all tools and much of the art).  Without Eric's "
	"dedication to Warthink, we would'nt have a snowball's chance in heck of completing this game.\n"
	"\n"
	"By the way, if anyone sees Eric, would they mind asking him what \"booyow\" means?";

// GLOBAL: MW2 0x100af9e8
MechChar g_dorcsStanfillText[] =
	"We made it!\n"
	"\n"
	"I want to thank John Spinale, Josh Resnick, and Howard Marks for putting together such a stellar team for this "
	"project.\n"
	"\n"
	"But more than anything I want to thank my wife Kyung Ah and son D.J. for supporting me during the past seven "
	"months and for tolerating my constant absence while we built the coolest game ever!";

// GLOBAL: MW2 0x100afb30
MechChar g_dorcsWhiteText[] = "I just love to code.  That's all there is to it.";

// GLOBAL: MW2 0x100afb68
MechChar g_dorcsZobelText[] = "Dave Zobel was last seen plunging into the darkness beyond the Wall of Testosterone.";

// GLOBAL: MW2 0x100afbc0
MechChar g_dorcsExitItem[] = "This is boring!";

// GLOBAL: MW2 0x100afbd0
PANE g_dorcsTextRect = {NULL, 0, 0x2666, 0x10000, 0x10000};

// GLOBAL: MW2 0x100afbe8
MenuTextBox g_dorcsAboutTextBox = {&g_dorcsTextRect, g_dorcsAboutText};

// GLOBAL: MW2 0x100afbf0
MenuTextBox g_dorcsClarkeTextBox = {&g_dorcsTextRect, g_dorcsClarkeText};

// GLOBAL: MW2 0x100afbf8
MenuTextBox g_dorcsDouglasTextBox = {&g_dorcsTextRect, g_dorcsDouglasText};

// GLOBAL: MW2 0x100afc00
MenuTextBox g_dorcsEthertonTextBox = {&g_dorcsTextRect, g_dorcsEthertonText};

// GLOBAL: MW2 0x100afc08
MenuTextBox g_dorcsHusebyTextBox = {&g_dorcsTextRect, g_dorcsHusebyText};

// GLOBAL: MW2 0x100afc10
MenuTextBox g_dorcsKaminsTextBox = {&g_dorcsTextRect, g_dorcsKaminsText};

// GLOBAL: MW2 0x100afc18
MenuTextBox g_dorcsKeatingTextBox = {&g_dorcsTextRect, g_dorcsKeatingText};

// GLOBAL: MW2 0x100afc20
MenuTextBox g_dorcsMilesTextBox = {&g_dorcsTextRect, g_dorcsMilesText};

// GLOBAL: MW2 0x100afc28
MenuTextBox g_dorcsMortenTextBox = {&g_dorcsTextRect, g_dorcsMortenText};

// GLOBAL: MW2 0x100afc30
MenuTextBox g_dorcsMortensenTextBox = {&g_dorcsTextRect, g_dorcsMortensenText};

// GLOBAL: MW2 0x100afc38
MenuTextBox g_dorcsPetersonTextBox = {&g_dorcsTextRect, g_dorcsPetersonText};

// GLOBAL: MW2 0x100afc40
MenuTextBox g_dorcsStanfillTextBox = {&g_dorcsTextRect, g_dorcsStanfillText};

// GLOBAL: MW2 0x100afc48
MenuTextBox g_dorcsWhiteTextBox = {&g_dorcsTextRect, g_dorcsWhiteText};

// GLOBAL: MW2 0x100afc50
MenuTextBox g_dorcsZobelTextBox = {&g_dorcsTextRect, g_dorcsZobelText};

// GLOBAL: MW2 0x100afc58
MenuControl g_dorcsAboutControl = {2, 0, &g_dorcsAboutTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afc80
MenuControl g_dorcsClarkeControl = {2, 0, &g_dorcsClarkeTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afca8
MenuControl g_dorcsDouglasControl = {2, 0, &g_dorcsDouglasTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afcd0
MenuControl g_dorcsEthertonControl = {2, 0, &g_dorcsEthertonTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afcf8
MenuControl g_dorcsHusebyControl = {2, 0, &g_dorcsHusebyTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afd20
MenuControl g_dorcsKaminsControl = {2, 0, &g_dorcsKaminsTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afd48
MenuControl g_dorcsKeatingControl = {2, 0, &g_dorcsKeatingTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afd70
MenuControl g_dorcsMilesControl = {2, 0, &g_dorcsMilesTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afd98
MenuControl g_dorcsMortenControl = {2, 0, &g_dorcsMortenTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afdc0
MenuControl g_dorcsMortensenControl = {2, 0, &g_dorcsMortensenTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afde8
MenuControl g_dorcsPetersonControl = {2, 0, &g_dorcsPetersonTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afe10
MenuControl g_dorcsStanfillControl = {2, 0, &g_dorcsStanfillTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afe38
MenuControl g_dorcsWhiteControl = {2, 0, &g_dorcsWhiteTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afe60
MenuControl g_dorcsZobelControl = {2, 0, &g_dorcsZobelTextBox, 0, NULL, NULL, NULL, NULL, NULL};

// GLOBAL: MW2 0x100afe88
MenuPage g_dorcsAboutPage = {
	0,
	g_dorcsAboutItem,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsAboutControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100affe0
MenuPage g_dorcsClarkePage = {
	0,
	g_dorcsClarkeName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsClarkeControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b0138
MenuPage g_dorcsDouglasPage = {
	0,
	g_dorcsDouglasName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsDouglasControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b0290
MenuPage g_dorcsEthertonPage = {
	0,
	g_dorcsEthertonName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsEthertonControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b03e8
MenuPage g_dorcsHusebyPage = {
	0,
	g_dorcsHusebyName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsHusebyControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b0540
MenuPage g_dorcsKaminsPage = {
	0,
	g_dorcsKaminsName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsKaminsControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b0698
MenuPage g_dorcsKeatingPage = {
	0,
	g_dorcsKeatingName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsKeatingControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b07f0
MenuPage g_dorcsMilesPage = {
	0,
	g_dorcsMilesName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsMilesControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b0948
MenuPage g_dorcsMortenPage = {
	0,
	g_dorcsMortenName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsMortenControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b0aa0
MenuPage g_dorcsMortensenPage = {
	0,
	g_dorcsMortensenName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsMortensenControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b0bf8
MenuPage g_dorcsPetersonPage = {
	0,
	g_dorcsPetersonName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsPetersonControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b0d50
MenuPage g_dorcsStanfillPage = {
	0,
	g_dorcsStanfillName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsStanfillControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b0ea8
MenuPage g_dorcsWhitePage = {
	0,
	g_dorcsWhiteName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsWhiteControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b1000
MenuPage g_dorcsZobelPage = {
	0,
	g_dorcsZobelName,
	0,
	2,
	0,
	NULL,
	{{3, NULL, RunMenuTextBox, &g_dorcsZobelControl, NULL}, {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b1158
MenuPage g_dorcsPage = {
	0,
	g_dorcsPageTitle,
	0,
	15,
	0,
	NULL,
	{{0, g_dorcsAboutItem, NULL, NULL, &g_dorcsAboutPage},
	 {0, g_dorcsClarkeName, NULL, NULL, &g_dorcsClarkePage},
	 {0, g_dorcsDouglasName, NULL, NULL, &g_dorcsDouglasPage},
	 {0, g_dorcsEthertonName, NULL, NULL, &g_dorcsEthertonPage},
	 {0, g_dorcsHusebyName, NULL, NULL, &g_dorcsHusebyPage},
	 {0, g_dorcsKaminsName, NULL, NULL, &g_dorcsKaminsPage},
	 {0, g_dorcsKeatingName, NULL, NULL, &g_dorcsKeatingPage},
	 {0, g_dorcsMilesName, NULL, NULL, &g_dorcsMilesPage},
	 {0, g_dorcsMortenName, NULL, NULL, &g_dorcsMortenPage},
	 {0, g_dorcsMortensenName, NULL, NULL, &g_dorcsMortensenPage},
	 {0, g_dorcsPetersonName, NULL, NULL, &g_dorcsPetersonPage},
	 {0, g_dorcsStanfillName, NULL, NULL, &g_dorcsStanfillPage},
	 {0, g_dorcsWhiteName, NULL, NULL, &g_dorcsWhitePage},
	 {0, g_dorcsZobelName, NULL, NULL, &g_dorcsZobelPage},
	 {2, g_dorcsExitItem, NULL, NULL, NULL}}
};

// GLOBAL: MW2 0x100b12b0
PANE g_dorcsMenuTarget = {NULL, 0x199a, 0x199a, 0xe666, 0xe666};

// GLOBAL: MW2 0x100b12c8
PANE g_dorcsMenuBackgroundTarget = {NULL, 0x199a, 0x199a, 0xe666, 0xe666};

// GLOBAL: MW2 0x100b12e0
MenuDefinition g_dorcsMenu = {
	&g_dorcsMenuTarget,
	37,
	g_dorcsMenuPageStack,
	0,
	-1,
	NULL,
	&g_dorcsMenuBackgroundTarget,
	-1,
	NULL,
	225,
	219,
	1,
	NULL,
	16,
	1,
	15,
	{0, 0},
	{0, 0},
	{0x51f, 0},
	{0x51f, 0},
	{0xc000, 0},
	&g_dorcsPage
};

// GLOBAL: MW2 0x100b1350
MechS32 g_fledToWindows = 0;

// The frame draw callback ShowDorcs replaces.
// GLOBAL: MW2 0x100b1354
void (*g_dorcsPreviousDrawCallback)(void) = DrawScene;

// GLOBAL: MW2 0x100c2d00
MenuPage* g_dorcsMenuPageStack[8];

// The dorcs sequence (ShowDorcs): the view shrinks to a point (g_dorcsTransition), a picture
// shows, then another, and a menu.

// GLOBAL: MW2 0x100b1358
PANE g_dorcsPoint = {NULL, 0x8000, 0x8000, 0x8000, 0x8000};

// GLOBAL: MW2 0x100b1370
PANE g_dorcsRectFrom = {NULL, 0x7d71, 0x7d71, 0x828f, 0x828f};

// GLOBAL: MW2 0x100b1388
PANE g_dorcsRectTo = {NULL, 0, 0, 0x10000, 0x10000};

// GLOBAL: MW2 0x100b13a0
PANE g_dorcsRect = {NULL, 0, 0, 0, 0};

// GLOBAL: MW2 0x100b13b8
RectTransitionState g_dorcsTransitionState = {0, 0, 0};

// GLOBAL: MW2 0x100b13c8
RectTransitionDef g_dorcsTransitionDef = {0xb5, &g_dorcsRectFrom, &g_dorcsRectTo, &g_dorcsRect};

// GLOBAL: MW2 0x100b13d8
RectTransition g_dorcsTransition = {&g_dorcsTransitionState, &g_dorcsTransitionDef};

// GLOBAL: MW2 0x100b13e0
void* g_dorcsGif = NULL;

// GLOBAL: MW2 0x100b13e4
PaletteColor* g_dorcsPalette = NULL;

// GLOBAL: MW2 0x100b13e8
MechU8* g_dorcsGifState = NULL;

// GLOBAL: MW2 0x100b13ec
MechS32 g_dorcsGifLoaded = 0;

// GLOBAL: MW2 0x100b13f0
MechS32 g_dorcsTime = 0;

// GLOBAL: MW2 0x100b13f4
MechS32 g_dorcsReverse = 1;

// A menu item's action: ejects the local player (game key 0x3b) without its sound.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10073af0
void AbortMissionAction(MenuDefinition* p_menu, MenuControl* p_control, MechS32 p_index, Point p_pos, MenuPage* p_page)
{
	MechS32 saved;
	MechS32 digit;
	MechS32 selected;

	if (!p_page) {
		return;
	}

	digit = g_menuKey - '1' == p_index;
	selected = p_page->m_selected == p_index;
	if ((selected && g_menuKey == '\r') || digit) {
		g_difficulty->m_invulnerable = 0;
		saved = g_hostileAtmosphere;
		g_hostileAtmosphere = 0;
		RunGameKey(0x3b);
		g_hostileAtmosphere = saved;
		FreeMenus();
	}
}

// A menu item's action: ejects the local player's mech.
// The only diff is a stack-slot permutation of the locals.
// FUNCTION: MW2 0x10073ba6
void FleeToWindowsAction(MenuDefinition* p_menu, MenuControl* p_control, MechS32 p_index, Point p_pos, MenuPage* p_page)
{
	MechS32 digit;
	MechS32 selected;
	Player* player;

	if (!p_page) {
		return;
	}

	digit = g_menuKey - '1' == p_index;
	selected = p_page->m_selected == p_index;
	if ((selected && g_menuKey == '\r') || digit) {
		g_fledToWindows = 1;
		g_difficulty->m_invulnerable = 0;
		player = g_players[g_localPlayerId];
		EjectPlayer(player->m_mech, 0);
		FreeMenus();
	}
}

// Reads vfx/<p_name>.bin whole.
// FUNCTION: MW2 0x10073c62
void* ReadVfxBin(MechChar* p_name)
{
	void* data;
	MechChar path[256];

	sprintf(path, "%s/%s.%s", "vfx", p_name, "bin");
	data = FILE_read(path, NULL);
	return data;
}

// FUNCTION: MW2 0x10073cb5
void CloseInGameMenus(void)
{
	RequestMenuClose(6);
	RequestMenuClose(2);
	RequestMenuClose(4);
	RequestMenuClose(7);
	RequestMenuClose(8);
	RequestMenuClose(5);
}

// The dorcs sequence's state.
// GLOBAL: MW2 0x100bf084
MechS32 g_dorcsState;

// GLOBAL: MW2 0x100bf058
PANE g_dorcsSavedTarget;

// GLOBAL: MW2 0x100bf070
PANE g_dorcsGifTarget;

// GLOBAL: MW2 0x100c2cec
PaletteColor g_dorcsBlack;

// The dorcs sequence's frame draw callback: steps the sequence (g_dorcsState) each frame.
// FUNCTION: MW2 0x10073cfc
void UpdateDorcs(void)
{
	PANE* rect;
	MechS32 index;
	PANE saved;
	PANE* target;
	MechS32 i;

	switch (g_dorcsState) {
	case 0:
		g_dorcsSavedTarget = g_currentPane;
		g_dorcsGifLoaded = 0;
		g_dorcsTransition.m_def->m_first = &g_dorcsRectFrom;
		if (!g_dorcsTransition.m_def->m_first->m_window) {
			target = g_dorcsTransition.m_def->m_first;
			ScaleRectToScreen(&g_mainPixelBuffer, target, target);
			target->m_window = &g_mainPixelBuffer;
			target = g_dorcsTransition.m_def->m_second;
			ScaleRectToScreen(&g_mainPixelBuffer, target, target);
			target->m_window = &g_mainPixelBuffer;
			target = g_dorcsTransition.m_def->m_out;
			target->m_window = &g_mainPixelBuffer;
			target = &g_dorcsPoint;
			target->m_window = &g_mainPixelBuffer;
			ScaleRectToScreen(&g_mainPixelBuffer, target, target);
		}

		g_dorcsTime = g_currentClock + 0x389;
		g_dorcsState = 1;
		if (g_dorcsPreviousDrawCallback) {
			g_dorcsPreviousDrawCallback();
		}
		break;
	case 1:
		if (g_currentClock < g_dorcsTime) {
			if (g_dorcsPreviousDrawCallback) {
				g_dorcsPreviousDrawCallback();
			}
			break;
		}

		g_dorcsTransition.m_def->m_duration = 0x279;
		g_dorcsReverse = 1;
		StartRectTransition(&g_dorcsTransition);
		g_dorcsState = 2;
	case 2:
		g_savedShowHud = g_showHud;
		g_showHud = 0;
		CloseInGameMenus();
		rect = UpdateRectTransition(g_dorcsReverse, &g_dorcsTransition);
		if (rect) {
			saved = g_panes[g_paneIndex];
			g_panes[g_paneIndex] = *rect;
			index = g_paneIndex;
			g_paneIndex = -1;
			SelectPane(index);
			if (g_dorcsPreviousDrawCallback) {
				g_dorcsPreviousDrawCallback();
			}

			g_stretchPending = 1;
			g_panes[g_paneIndex] = saved;
			break;
		}
		else if (g_dorcsReverse == 1) {
			g_dorcsReverse = 0;
			StartRectTransition(&g_dorcsTransition);
			break;
		}
		else {
			g_dorcsTransition.m_def->m_duration = 0x2d4;
			g_dorcsTransition.m_def->m_first = &g_dorcsPoint;
			StartRectTransition(&g_dorcsTransition);
			g_dorcsState = 3;
		}
	case 3:
		g_showHud = 0;
		CloseInGameMenus();
		rect = UpdateRectTransitionByAxis(1, &g_dorcsTransition);
		if (rect) {
			saved = g_panes[g_paneIndex];
			g_panes[g_paneIndex] = *rect;
			index = g_paneIndex;
			g_paneIndex = -1;
			SelectPane(index);
			VFX_pane_wipe(&g_dorcsSavedTarget, 0);
			if (g_dorcsPreviousDrawCallback) {
				g_dorcsPreviousDrawCallback();
			}

			OutlinePane(&g_currentPane, 10);
			g_panes[g_paneIndex] = saved;
			g_currentPane = g_dorcsSavedTarget;
			break;
		}
		else {
			index = g_paneIndex;
			g_paneIndex = -1;
			SelectPane(index);
			g_dorcsState = 4;
		}
	case 4:
		g_showHud = 0;
		CloseInGameMenus();
		g_dorcsGifTarget = g_currentPane;
		g_dorcsGifState = MechHeapAlloc(g_primaryHeap, 0x502e);
		if (g_dorcsGifState) {
			g_dorcsPalette =
				MechHeapAllocZeroed(g_primaryHeap, 0x100 * sizeof(PaletteColor));
			if (g_dorcsPalette) {
				g_currentDisplayBackend->m_setPalette(0, 0x100, g_dorcsPalette, 1);
				g_dorcsGif = ReadVfxBin("vfxjk");
				if (g_dorcsGif) {
					FitRectToGif(&g_dorcsGifTarget, &g_dorcsGifTarget, g_dorcsGif);
					VFX_pane_wipe(&g_currentPane, 0);
					VFX_GIF_draw(&g_dorcsGifTarget, g_dorcsGif, g_dorcsGifState);
					if (g_windowActive) {
						g_currentRefreshMode->m_flip();
					}

					VFX_GIF_palette(g_dorcsGif, (MechU8*) g_dorcsPalette);
					g_currentDisplayBackend->m_blendPalettes(g_dorcsPalette, 0xb5);
					g_dorcsGifLoaded = 1;
				}
			}
		}

		g_dorcsTime = g_currentClock + 0x10f;
		g_dorcsState = 5;
		break;
	case 5:
		g_showHud = 0;
		CloseInGameMenus();
		if (g_currentClock < g_dorcsTime) {
			if (g_dorcsGifLoaded) {
				VFX_GIF_draw(&g_dorcsGifTarget, g_dorcsGif, g_dorcsGifState);
				if (g_windowActive) {
					g_currentRefreshMode->m_flip();
				}
			}
		}
		else {
			if (g_dorcsGif) {
				MEM_free_lock(g_dorcsGif);
			}

			g_dorcsGif = ReadVfxBin("vfxhd");
			if (g_dorcsGif && g_dorcsGifLoaded) {
				for (i = 0; i < 0x100; i++) {
					g_dorcsPalette[i] = g_dorcsBlack;
				}

				g_currentDisplayBackend->m_blendPalettes(g_dorcsPalette, 0x5a);
				VFX_pane_wipe(&g_currentPane, 0);
				VFX_GIF_draw(&g_dorcsGifTarget, g_dorcsGif, g_dorcsGifState);
				if (g_windowActive) {
					g_currentRefreshMode->m_flip();
				}

				VFX_GIF_palette(g_dorcsGif, (MechU8*) g_dorcsPalette);
				g_currentDisplayBackend->m_blendPalettes(g_dorcsPalette, 0xb5);
			}
			else {
				g_dorcsGifLoaded = 0;
			}

			g_dorcsTime = g_currentClock + 0x10f;
			g_dorcsState = 6;
		}
		break;
	case 6:
		g_showHud = 0;
		CloseInGameMenus();
		if (g_currentClock < g_dorcsTime) {
			if (g_dorcsGifLoaded) {
				VFX_GIF_draw(&g_dorcsGifTarget, g_dorcsGif, g_dorcsGifState);
			}
		}
		else {
			if (!g_dorcsGifLoaded) {
				ApplyPaletteResource(g_currentPalette);
			}

			RequestMenu(3);
			g_dorcsState = 7;
		}
		break;
	case 7:
		VFX_pane_wipe(&g_currentPane, 0);
		if (g_dorcsGifLoaded) {
			VFX_GIF_draw(&g_dorcsGifTarget, g_dorcsGif, g_dorcsGifState);
		}

		if (!GetOpenMenu()) {
			g_dorcsTime = g_currentClock + 0x10f;
			g_dorcsState = -1;
		}
		break;
	case -1:
		g_showHud = 0;
		CloseInGameMenus();
		if (g_currentClock < g_dorcsTime) {
			VFX_pane_wipe(&g_currentPane, 0);
			if (g_dorcsGifLoaded) {
				VFX_GIF_draw(&g_dorcsGifTarget, g_dorcsGif, g_dorcsGifState);
			}
		}
		else {
			VFX_pane_wipe(&g_currentPane, 0);
			if (g_windowActive) {
				g_currentRefreshMode->m_flip();
			}

			ApplyPaletteResource(g_currentPalette);
			if (g_dorcsGif) {
				MEM_free_lock(g_dorcsGif);
			}

			g_dorcsGif = NULL;
			if (g_dorcsPalette) {
				MechHeapFree(g_primaryHeap, g_dorcsPalette);
			}

			g_dorcsPalette = NULL;
			if (g_dorcsGifState) {
				MechHeapFree(g_primaryHeap, g_dorcsGifState);
			}

			g_dorcsGifState = NULL;
			g_renderSettings.m_frameDrawCallback = g_dorcsPreviousDrawCallback;
			g_savedShowHud = 1;
			g_stretchPending = 1;
			g_dorcsState = 0;
		}
		break;
	}
}

// Starts the dorcs sequence: UpdateDorcs draws the frames in place of the frame draw callback.
// FUNCTION: MW2 0x100745b2
void ShowDorcs(void)
{
	g_dorcsPreviousDrawCallback = g_renderSettings.m_frameDrawCallback;
	g_renderSettings.m_frameDrawCallback = UpdateDorcs;
	g_dorcsState = 0;
}
