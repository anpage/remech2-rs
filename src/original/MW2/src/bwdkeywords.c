/* The BWD keywords: each node tag of a BWD stream and its name. A data-only object: its data
   follows keyboard.c's. */
#include "bwdkeywords.h"

#include "types.h"

// The BWD version this build reads: a mission stream's REV record must be at least this.
// GLOBAL: MW2 0x100a66b8
MechChar g_bwdVersion[] = "1.22";

// The name of each keyword in g_bwdTypeCodes.
// GLOBAL: MW2 0x100a66c0
MechChar* g_bwdKeywordNames[0x48] = {
	"bwd",           "rev",
	"dtbl",          "planet",
	"palette_grp",   "terrain",
	"planet",        "light",
	"window",        "start",
	"now",           "scenario_table",
	"mission_table", "gp_chassis_table",
	"3dbitmap_prj",  "3dbitmap_idlist",
	"3dbitmap_sec",  "3dbitmap_enable",
	"frame_prj",     "horizon_map",
	"ground_map",    "sky_map",
	"polyoffset",    "block_xform",
	"rep",           "endrep",
	"block",         "elseblock",
	"endblock",      "object",
	"animfile",      "scrounge",
	"thing",         "gamepiece",
	"cptfile",       "pitfile",
	"vptfile",       "hudfile",
	"mgdfile",       "eyeobj",
	"gamething",     "objloc",
	"booyowthing",   "xplode",
	"navpoint",      "navobject",
	"lightobj",      "task",
	"position",      "rotate",
	"include",       "group",
	"gpspec",        "mangle_off",
	"mangle_on",     "goal_on",
	"goal_off",      "anim_2d",
	"star",          "view",
	"pof",           "affiliation",
	"orders",        "music",
	"animsound",     "luma",
	"path",          "formation",
	"hidden_text",   "planet_desc",
	"star_desc",     "startupscreen",
};

// The extension OpenBwdStream gives a file name without one.
// GLOBAL: MW2 0x100a67e0
MechChar g_bwdExtension[8] = "BWD";

// The node tags, four characters each. The first is a stream's header node (BwdHeader).
// GLOBAL: MW2 0x100a67e8
MechU32 g_bwdTypeCodes[0x48] = {
	0x00445742, // BWD
	0x00564552, // REV
	0x4c425444, // DTBL
	0x544e4c50, // PLNT
	0x474c4150, // PALG
	0x52524554, // TERR
	0x4d494c43, // CLIM
	0x4554494c, // LITE
	0x50535756, // VWSP
	0x54535756, // VWST
	0x54494e49, // INIT
	0x4c425453, // STBL
	0x4c42544d, // MTBL
	0x4c425447, // GTBL
	0x4a504d42, // BMPJ
	0x44494d42, // BMID
	0x43455342, // BSEC
	0x4e454d42, // BMEN
	0x4a525046, // FPRJ
	0x4d5a5248, // HRZM
	0x4d444e47, // GNDM
	0x4d594b53, // SKYM
	0x4f474c50, // PLGO
	0x584b4c42, // BLKX
	0x52504552, // REPR
	0x52444e45, // ENDR
	0x004b4c42, // BLK
	0x42534c45, // ELSB
	0x42444e45, // ENDB
	0x004a424f, // OBJ
	0x4d494e41, // ANIM
	0x47524353, // SCRG
	0x474e4854, // THNG
	0x00005047, // GP
	0x46545043, // CPTF
	0x46544950, // PITF
	0x46545056, // VPTF
	0x46445548, // HUDF
	0x4644474d, // MGDF
	0x4f455945, // EYEO
	0x00005447, // GT
	0x4c4a424f, // OBJL
	0x47485442, // BTHG
	0x4f4c5058, // XPLO
	0x5056414e, // NAVP
	0x4f56414e, // NAVO
	0x4f54494c, // LITO
	0x004b5354, // TSK
	0x00534f50, // POS
	0x00544f52, // ROT
	0x4c434e49, // INCL
	0x00505247, // GRP
	0x00535047, // GPS
	0x46464f4d, // MOFF
	0x004e4f4d, // MON
	0x004e4f47, // GON
	0x46464f47, // GOFF
	0x324d4e41, // ANM2
	0x52415453, // STAR
	0x57454956, // VIEW
	0x4f464f50, // POFO
	0x4c464641, // AFFL
	0x5244524f, // ORDR
	0x4953554d, // MUSI
	0x444e5341, // ASND
	0x4c42544c, // LTBL
	0x4c425450, // PTBL
	0x4c425446, // FTBL
	0x54585448, // HTXT
	0x43534450, // PDSC
	0x43534453, // SDSC
	0x53505553, // SUPS
};
