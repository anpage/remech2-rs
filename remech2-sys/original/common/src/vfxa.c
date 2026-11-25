/* VFXA, Miles Design VFX's 2D primitives, reimplemented in C, which both DLLs compile through their
   own vfxa.h. The names are VFX's; the C's own helpers have names of their own.

   Most routines start by clipping their pane to its window: a window whose x_max or y_max is
   negative returns -1, a pane that leaves nothing of it -2. Coordinates are relative to the
   pane's unclipped top left. The C does the assembly's arithmetic, in 32 bits that wrap.

   The assembly's globals are mostly scratch that a routine fills before it reads: row buffers,
   the GIF decoder's row, the dissolve's row tables, the rotation's corners, the run-length
   encoder's cursors. The C keeps those in locals. What a call leaves for later ones stays
   global: the display driver's entry points and its name, the shape lookaside table, and the
   fade's error terms.

   Return values the assembly leaves behind without setting them (stack garbage, a pointer it
   last computed) aren't reproduced: those routines return 0 there, and the callers don't read
   them. Where the DLLs' headers declare a routine void, the C returns nothing. */
#include "vfxa.h"

#include "pane.h"
#include "portable.h"
#include "types.h"
#include "window.h"

#include <stddef.h>
#include <string.h>

// --- Data ---

// The registered display driver's entry points (VFX.H's hardware-specific functions), which
// VFX_register_driver copies from the driver's table, in this order. VFX_window_fade calls
// VFX_wait_vblank_leading, VFX_DAC_read and VFX_DAC_write.
void* (*VFX_describe_driver)(void);
void (*VFX_init_driver)(void);
void (*VFX_shutdown_driver)(void);
void (*VFX_area_wipe)(int32_t p_x0, int32_t p_y0, int32_t p_x1, int32_t p_y1, int32_t p_color);
void (*VFX_wait_vblank)(void);
void (*VFX_wait_vblank_leading)(void);
void (*VFX_window_refresh)(WINDOW* p_target, int32_t p_x0, int32_t p_y0, int32_t p_x1, int32_t p_y1);
void (*VFX_window_read)(WINDOW* p_destination, int32_t p_x0, int32_t p_y0, int32_t p_x1, int32_t p_y1);
void (*VFX_DAC_read)(int32_t p_colorNumber, uint8_t* p_triplet);
void (*VFX_DAC_write)(int32_t p_colorNumber, uint8_t* p_triplet);
void (*VFX_bank_reset)(void);
void (*VFX_pane_refresh)(PANE* p_target, int32_t p_x0, int32_t p_y0, int32_t p_x1, int32_t p_y1);
void (*VFX_line_address)(int32_t p_x, int32_t p_y, uint8_t** p_addr, uint32_t* p_nbytes);

// A driver's table: its entry points in the order above, as the first's type. The first returns
// the driver's name to VFX_driver_name.
typedef char* (*VfxDriverEntry)(void);

char driver_name[0xd] = "MCGA.DLL";

// VFX_shape_lookaside's table, for the remapped blits and VFX_shape_remap_colors.
uint8_t lookaside[0x100] = {0};

// VFX_window_fade's error terms, three per color. Each fade sets only as many as it found colors
// less one, from the start, and reads those of the colors it found: the rest are what earlier
// fades left.
uint8_t color_error[0x300] = {0};

// The 16.16 cosine of 0 to 90 degrees in tenths of a degree; read backwards, the sine.
static const int32_t g_cosTable[0x385] = {
	0x10000, 0x10000, 0x10000, 0xffff, 0xfffe, 0xfffe, 0xfffc, 0xfffb, 0xfffa, 0xfff8, 0xfff6, 0xfff4, 0xfff2, 0xffef,
	0xffec,  0xffea,  0xffe6,  0xffe3, 0xffe0, 0xffdc, 0xffd8, 0xffd4, 0xffd0, 0xffcb, 0xffc7, 0xffc2, 0xffbd, 0xffb7,
	0xffb2,  0xffac,  0xffa6,  0xffa0, 0xff9a, 0xff93, 0xff8d, 0xff86, 0xff7f, 0xff77, 0xff70, 0xff68, 0xff60, 0xff58,
	0xff50,  0xff48,  0xff3f,  0xff36, 0xff2d, 0xff24, 0xff1a, 0xff10, 0xff07, 0xfefd, 0xfef2, 0xfee8, 0xfedd, 0xfed2,
	0xfec7,  0xfebc,  0xfeb1,  0xfea5, 0xfe99, 0xfe8d, 0xfe81, 0xfe74, 0xfe68, 0xfe5b, 0xfe4e, 0xfe40, 0xfe33, 0xfe25,
	0xfe18,  0xfe09,  0xfdfb,  0xfded, 0xfdde, 0xfdcf, 0xfdc0, 0xfdb1, 0xfda2, 0xfd92, 0xfd82, 0xfd72, 0xfd62, 0xfd52,
	0xfd41,  0xfd30,  0xfd1f,  0xfd0e, 0xfcfd, 0xfceb, 0xfcd9, 0xfcc7, 0xfcb5, 0xfca3, 0xfc90, 0xfc7d, 0xfc6a, 0xfc57,
	0xfc44,  0xfc30,  0xfc1c,  0xfc08, 0xfbf4, 0xfbe0, 0xfbcb, 0xfbb7, 0xfba2, 0xfb8d, 0xfb77, 0xfb62, 0xfb4c, 0xfb36,
	0xfb20,  0xfb0a,  0xfaf3,  0xfadc, 0xfac5, 0xfaae, 0xfa97, 0xfa80, 0xfa68, 0xfa50, 0xfa38, 0xfa20, 0xfa07, 0xf9ef,
	0xf9d6,  0xf9bd,  0xf9a3,  0xf98a, 0xf970, 0xf956, 0xf93c, 0xf922, 0xf908, 0xf8ed, 0xf8d2, 0xf8b7, 0xf89c, 0xf881,
	0xf865,  0xf84a,  0xf82e,  0xf811, 0xf7f5, 0xf7d9, 0xf7bc, 0xf79f, 0xf782, 0xf764, 0xf747, 0xf729, 0xf70b, 0xf6ed,
	0xf6cf,  0xf6b0,  0xf692,  0xf673, 0xf654, 0xf635, 0xf615, 0xf5f6, 0xf5d6, 0xf5b6, 0xf596, 0xf575, 0xf555, 0xf534,
	0xf513,  0xf4f2,  0xf4d0,  0xf4af, 0xf48d, 0xf46b, 0xf449, 0xf427, 0xf404, 0xf3e2, 0xf3bf, 0xf39c, 0xf378, 0xf355,
	0xf331,  0xf30e,  0xf2ea,  0xf2c5, 0xf2a1, 0xf27c, 0xf258, 0xf233, 0xf20e, 0xf1e8, 0xf1c3, 0xf19d, 0xf177, 0xf151,
	0xf12b,  0xf104,  0xf0de,  0xf0b7, 0xf090, 0xf068, 0xf041, 0xf019, 0xeff2, 0xefca, 0xefa2, 0xef79, 0xef51, 0xef28,
	0xeeff,  0xeed6,  0xeead,  0xee83, 0xee5a, 0xee30, 0xee06, 0xeddc, 0xedb1, 0xed87, 0xed5c, 0xed31, 0xed06, 0xecdb,
	0xecaf,  0xec83,  0xec58,  0xec2b, 0xebff, 0xebd3, 0xeba6, 0xeb79, 0xeb4c, 0xeb1f, 0xeaf2, 0xeac4, 0xea97, 0xea69,
	0xea3b,  0xea0d,  0xe9de,  0xe9b0, 0xe981, 0xe952, 0xe923, 0xe8f3, 0xe8c4, 0xe894, 0xe864, 0xe834, 0xe804, 0xe7d3,
	0xe7a3,  0xe772,  0xe741,  0xe710, 0xe6de, 0xe6ad, 0xe67b, 0xe649, 0xe617, 0xe5e5, 0xe5b3, 0xe580, 0xe54d, 0xe51a,
	0xe4e7,  0xe4b4,  0xe481,  0xe44d, 0xe419, 0xe3e5, 0xe3b1, 0xe37c, 0xe348, 0xe313, 0xe2de, 0xe2a9, 0xe274, 0xe23e,
	0xe209,  0xe1d3,  0xe19d,  0xe167, 0xe131, 0xe0fa, 0xe0c3, 0xe08d, 0xe056, 0xe01e, 0xdfe7, 0xdfb0, 0xdf78, 0xdf40,
	0xdf08,  0xded0,  0xde97,  0xde5f, 0xde26, 0xdded, 0xddb4, 0xdd7b, 0xdd41, 0xdd07, 0xdcce, 0xdc94, 0xdc5a, 0xdc1f,
	0xdbe5,  0xdbaa,  0xdb6f,  0xdb34, 0xdaf9, 0xdabe, 0xda82, 0xda47, 0xda0b, 0xd9cf, 0xd993, 0xd956, 0xd91a, 0xd8dd,
	0xd8a0,  0xd863,  0xd826,  0xd7e9, 0xd7ab, 0xd76d, 0xd72f, 0xd6f1, 0xd6b3, 0xd675, 0xd636, 0xd5f7, 0xd5b9, 0xd57a,
	0xd53a,  0xd4fb,  0xd4bb,  0xd47c, 0xd43c, 0xd3fc, 0xd3bc, 0xd37b, 0xd33b, 0xd2fa, 0xd2b9, 0xd278, 0xd237, 0xd1f5,
	0xd1b4,  0xd172,  0xd130,  0xd0ee, 0xd0ac, 0xd06a, 0xd027, 0xcfe5, 0xcfa2, 0xcf5f, 0xcf1c, 0xced8, 0xce95, 0xce51,
	0xce0e,  0xcdca,  0xcd85,  0xcd41, 0xccfd, 0xccb8, 0xcc73, 0xcc2e, 0xcbe9, 0xcba4, 0xcb5f, 0xcb19, 0xcad3, 0xca8e,
	0xca48,  0xca01,  0xc9bb,  0xc975, 0xc92e, 0xc8e7, 0xc8a0, 0xc859, 0xc812, 0xc7ca, 0xc783, 0xc73b, 0xc6f3, 0xc6ab,
	0xc663,  0xc61a,  0xc5d2,  0xc589, 0xc540, 0xc4f7, 0xc4ae, 0xc465, 0xc41b, 0xc3d2, 0xc388, 0xc33e, 0xc2f4, 0xc2aa,
	0xc260,  0xc215,  0xc1ca,  0xc180, 0xc135, 0xc0ea, 0xc09e, 0xc053, 0xc007, 0xbfbc, 0xbf70, 0xbf24, 0xbed8, 0xbe8b,
	0xbe3f,  0xbdf2,  0xbda5,  0xbd58, 0xbd0b, 0xbcbe, 0xbc71, 0xbc23, 0xbbd6, 0xbb88, 0xbb3a, 0xbaec, 0xba9e, 0xba4f,
	0xba01,  0xb9b2,  0xb963,  0xb914, 0xb8c5, 0xb876, 0xb827, 0xb7d7, 0xb787, 0xb738, 0xb6e8, 0xb698, 0xb647, 0xb5f7,
	0xb5a6,  0xb556,  0xb505,  0xb4b4, 0xb463, 0xb412, 0xb3c0, 0xb36f, 0xb31d, 0xb2cb, 0xb279, 0xb227, 0xb1d5, 0xb183,
	0xb130,  0xb0de,  0xb08b,  0xb038, 0xafe5, 0xaf92, 0xaf3e, 0xaeeb, 0xae97, 0xae44, 0xadf0, 0xad9c, 0xad48, 0xacf3,
	0xac9f,  0xac4b,  0xabf6,  0xaba1, 0xab4c, 0xaaf7, 0xaaa2, 0xaa4d, 0xa9f7, 0xa9a1, 0xa94c, 0xa8f6, 0xa8a0, 0xa84a,
	0xa7f3,  0xa79d,  0xa747,  0xa6f0, 0xa699, 0xa642, 0xa5eb, 0xa594, 0xa53d, 0xa4e5, 0xa48e, 0xa436, 0xa3de, 0xa386,
	0xa32e,  0xa2d6,  0xa27e,  0xa225, 0xa1cd, 0xa174, 0xa11b, 0xa0c2, 0xa069, 0xa010, 0x9fb7, 0x9f5d, 0x9f04, 0x9eaa,
	0x9e50,  0x9df6,  0x9d9c,  0x9d42, 0x9ce7, 0x9c8d, 0x9c32, 0x9bd8, 0x9b7d, 0x9b22, 0x9ac7, 0x9a6c, 0x9a11, 0x99b5,
	0x995a,  0x98fe,  0x98a2,  0x9846, 0x97ea, 0x978e, 0x9732, 0x96d6, 0x9679, 0x961c, 0x95c0, 0x9563, 0x9506, 0x94a9,
	0x944c,  0x93ee,  0x9391,  0x9334, 0x92d6, 0x9278, 0x921a, 0x91bc, 0x915e, 0x9100, 0x90a2, 0x9043, 0x8fe5, 0x8f86,
	0x8f27,  0x8ec8,  0x8e69,  0x8e0a, 0x8dab, 0x8d4c, 0x8cec, 0x8c8d, 0x8c2d, 0x8bcd, 0x8b6d, 0x8b0d, 0x8aad, 0x8a4d,
	0x89ed,  0x898c,  0x892c,  0x88cb, 0x886b, 0x880a, 0x87a9, 0x8748, 0x86e7, 0x8685, 0x8624, 0x85c2, 0x8561, 0x84ff,
	0x849d,  0x843c,  0x83da,  0x8377, 0x8315, 0x82b3, 0x8251, 0x81ee, 0x818b, 0x8129, 0x80c6, 0x8063, 0x8000, 0x7f9d,
	0x7f3a,  0x7ed6,  0x7e73,  0x7e0f, 0x7dac, 0x7d48, 0x7ce4, 0x7c80, 0x7c1c, 0x7bb8, 0x7b54, 0x7af0, 0x7a8c, 0x7a27,
	0x79c3,  0x795e,  0x78f9,  0x7894, 0x782f, 0x77ca, 0x7765, 0x7700, 0x769b, 0x7635, 0x75d0, 0x756a, 0x7504, 0x749f,
	0x7439,  0x73d3,  0x736d,  0x7307, 0x72a0, 0x723a, 0x71d4, 0x716d, 0x7107, 0x70a0, 0x7039, 0x6fd2, 0x6f6b, 0x6f04,
	0x6e9d,  0x6e36,  0x6dcf,  0x6d67, 0x6d00, 0x6c98, 0x6c31, 0x6bc9, 0x6b61, 0x6af9, 0x6a91, 0x6a29, 0x69c1, 0x6959,
	0x68f1,  0x6888,  0x6820,  0x67b7, 0x674f, 0x66e6, 0x667d, 0x6614, 0x65ab, 0x6542, 0x64d9, 0x6470, 0x6407, 0x639e,
	0x6334,  0x62cb,  0x6261,  0x61f8, 0x618e, 0x6124, 0x60ba, 0x6050, 0x5fe6, 0x5f7c, 0x5f12, 0x5ea8, 0x5e3d, 0x5dd3,
	0x5d69,  0x5cfe,  0x5c93,  0x5c29, 0x5bbe, 0x5b53, 0x5ae8, 0x5a7d, 0x5a12, 0x59a7, 0x593c, 0x58d1, 0x5865, 0x57fa,
	0x578f,  0x5723,  0x56b8,  0x564c, 0x55e0, 0x5574, 0x5509, 0x549d, 0x5431, 0x53c5, 0x5358, 0x52ec, 0x5280, 0x5214,
	0x51a7,  0x513b,  0x50ce,  0x5062, 0x4ff5, 0x4f88, 0x4f1c, 0x4eaf, 0x4e42, 0x4dd5, 0x4d68, 0x4cfb, 0x4c8e, 0x4c21,
	0x4bb4,  0x4b46,  0x4ad9,  0x4a6b, 0x49fe, 0x4990, 0x4923, 0x48b5, 0x4848, 0x47da, 0x476c, 0x46fe, 0x4690, 0x4622,
	0x45b4,  0x4546,  0x44d8,  0x446a, 0x43fb, 0x438d, 0x431f, 0x42b0, 0x4242, 0x41d3, 0x4165, 0x40f6, 0x4088, 0x4019,
	0x3faa,  0x3f3b,  0x3ecc,  0x3e5e, 0x3def, 0x3d80, 0x3d11, 0x3ca1, 0x3c32, 0x3bc3, 0x3b54, 0x3ae5, 0x3a75, 0x3a06,
	0x3996,  0x3927,  0x38b7,  0x3848, 0x37d8, 0x3769, 0x36f9, 0x3689, 0x3619, 0x35aa, 0x353a, 0x34ca, 0x345a, 0x33ea,
	0x337a,  0x330a,  0x329a,  0x322a, 0x31b9, 0x3149, 0x30d9, 0x3069, 0x2ff8, 0x2f88, 0x2f17, 0x2ea7, 0x2e37, 0x2dc6,
	0x2d55,  0x2ce5,  0x2c74,  0x2c04, 0x2b93, 0x2b22, 0x2ab1, 0x2a41, 0x29d0, 0x295f, 0x28ee, 0x287d, 0x280c, 0x279b,
	0x272a,  0x26b9,  0x2648,  0x25d7, 0x2566, 0x24f5, 0x2483, 0x2412, 0x23a1, 0x2330, 0x22be, 0x224d, 0x21dc, 0x216a,
	0x20f9,  0x2087,  0x2016,  0x1fa4, 0x1f33, 0x1ec1, 0x1e50, 0x1dde, 0x1d6d, 0x1cfb, 0x1c89, 0x1c18, 0x1ba6, 0x1b34,
	0x1ac2,  0x1a51,  0x19df,  0x196d, 0x18fb, 0x1889, 0x1817, 0x17a6, 0x1734, 0x16c2, 0x1650, 0x15de, 0x156c, 0x14fa,
	0x1488,  0x1416,  0x13a4,  0x1332, 0x12c0, 0x124e, 0x11dc, 0x1169, 0x10f7, 0x1085, 0x1013, 0xfa1,  0xf2f,  0xebd,
	0xe4a,   0xdd8,   0xd66,   0xcf4,  0xc81,  0xc0f,  0xb9d,  0xb2b,  0xab8,  0xa46,  0x9d4,  0x961,  0x8ef,  0x87d,
	0x80b,   0x798,   0x726,   0x6b4,  0x641,  0x5cf,  0x55c,  0x4ea,  0x478,  0x405,  0x393,  0x321,  0x2ae,  0x23c,
	0x1ca,   0x157,   0xe5,    0x72,   0x0
};

static char g_bmhdProp[4] = {'B', 'M', 'H', 'D'};
static char g_cmapProp[4] = {'C', 'M', 'A', 'P'};
static char g_bodyProp[4] = {'B', 'O', 'D', 'Y'};

// The GIF decoder's masks of 0 to 8 low bits, and its interlaced passes' row steps and first rows.
static const uint8_t g_gifCmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
static const uint8_t g_gifIncTable[5] = {8, 8, 4, 2, 0};
static const uint8_t g_gifStartTable[5] = {0, 4, 2, 1, 0};

// The tap masks of maximal-length LFSRs, for 2 to 32 bits.
static const uint32_t g_pfConstants[0x1f] = {0x3,       0x6,       0xc,       0x14,       0x30,       0x60,
											 0xb8,      0x110,     0x240,     0x500,      0xca0,      0x1b00,
											 0x3500,    0x6000,    0xb400,    0x12000,    0x20400,    0x72000,
											 0x90000,   0x140000,  0x300000,  0x420000,   0xd80000,   0x1200000,
											 0x3880000, 0x7200000, 0x9000000, 0x14000000, 0x32800000, 0x48000000,
											 0xa3000000};

// --- Helpers ---

// A little-endian dword of the resources the routines read (shapes, fonts), at any alignment.
static uint32_t ReadU32(const uint8_t* p_data)
{
	return p_data[0] | ((uint32_t) p_data[1] << 8) | ((uint32_t) p_data[2] << 16) | ((uint32_t) p_data[3] << 24);
}

static int32_t ReadS32(const uint8_t* p_data)
{
	return PortableS32(ReadU32(p_data));
}

static void WriteU32(uint8_t* p_data, uint32_t p_value)
{
	p_data[0] = (uint8_t) p_value;
	p_data[1] = (uint8_t) (p_value >> 8);
	p_data[2] = (uint8_t) (p_value >> 16);
	p_data[3] = (uint8_t) (p_value >> 24);
}

static uint32_t ReadU16(const uint8_t* p_data)
{
	return p_data[0] | ((uint32_t) p_data[1] << 8);
}

// A big-endian word (IFF).
static uint32_t ReadBigU16(const uint8_t* p_data)
{
	return ((uint32_t) p_data[0] << 8) | p_data[1];
}

static int32_t Add(int32_t p_a, int32_t p_b)
{
	return PortableS32((uint32_t) p_a + (uint32_t) p_b);
}

static int32_t Sub(int32_t p_a, int32_t p_b)
{
	return PortableS32((uint32_t) p_a - (uint32_t) p_b);
}

// `cdq; xor eax, edx; sub eax, edx`: the most negative value stays as it is.
static int32_t Abs(int32_t p_value)
{
	return p_value < 0 ? PortableS32(0 - (uint32_t) p_value) : p_value;
}

// A view clipped to its pixel buffer, in buffer coordinates.
typedef struct ClippedView {
	uint8_t* m_pixels;
	uint32_t m_pitch;
	int32_t m_left;
	int32_t m_top;
	int32_t m_right;
	int32_t m_bottom;
	int32_t m_originX; // the view's own top left, unclipped
	int32_t m_originY;
} ClippedView;

static int32_t ClipView(ClippedView* p_clip, const PANE* p_view)
{
	const WINDOW* buffer = p_view->m_window;

	if (buffer->m_xMax < 0 || buffer->m_yMax < 0) {
		return -1;
	}

	p_clip->m_pixels = buffer->m_buffer;
	p_clip->m_pitch = (uint32_t) buffer->m_xMax + 1;
	p_clip->m_originX = p_view->m_x0;
	p_clip->m_originY = p_view->m_y0;
	p_clip->m_left = p_view->m_x0 > 0 ? p_view->m_x0 : 0;
	p_clip->m_top = p_view->m_y0 > 0 ? p_view->m_y0 : 0;
	p_clip->m_right = p_view->m_x1 < buffer->m_xMax ? p_view->m_x1 : buffer->m_xMax;
	p_clip->m_bottom = p_view->m_y1 < buffer->m_yMax ? p_view->m_y1 : buffer->m_yMax;
	if (p_clip->m_right < p_clip->m_left || p_clip->m_bottom < p_clip->m_top) {
		return -2;
	}

	return 0;
}

// A pixel's offset in the buffer: `imul` by the pitch, which wraps like the unsigned product.
static uint32_t PixelOffset(const ClippedView* p_clip, int32_t p_x, int32_t p_y)
{
	return (uint32_t) p_y * p_clip->m_pitch + (uint32_t) p_x;
}

// The sides of the clipped view a point lies beyond: left 8, right 4, top 2, bottom 1, from the
// signs of the differences (which wrap).
static uint32_t OutCode(const ClippedView* p_clip, int32_t p_x, int32_t p_y)
{
	return ((uint32_t) Sub(p_x, p_clip->m_left) >> 31) << 3 | ((uint32_t) Sub(p_clip->m_right, p_x) >> 31) << 2 |
		   ((uint32_t) Sub(p_y, p_clip->m_top) >> 31) << 1 | (uint32_t) Sub(p_clip->m_bottom, p_y) >> 31;
}

// --- The display driver ---

// Calls the driver's first entry point, and copies the name it returns into driver_name,
// which it returns. The names are at most 12 characters (the assembly writes on into the
// variables after the buffer).
char* VFX_driver_name(char* (**p_vfxScanDll)(void) )
{
	char* name = p_vfxScanDll[0]();

	PORTABLE_ASSERT(strlen(name) < sizeof(driver_name));
	strcpy(driver_name, name);
	return driver_name;
}

// Each entry goes through void (*)(void), which any function pointer converts through.
void VFX_register_driver(char* (**p_dllBase)(void) )
{
	VfxDriverEntry* table = p_dllBase;

	VFX_describe_driver = (void* (*) (void) )(void (*)(void)) table[0];
	VFX_init_driver = (void (*)(void)) table[1];
	VFX_shutdown_driver = (void (*)(void)) table[2];
	VFX_area_wipe = (void (*)(int32_t, int32_t, int32_t, int32_t, int32_t))(void (*)(void)) table[3];
	VFX_wait_vblank = (void (*)(void)) table[4];
	VFX_wait_vblank_leading = (void (*)(void)) table[5];
	VFX_window_refresh = (void (*)(WINDOW*, int32_t, int32_t, int32_t, int32_t))(void (*)(void)) table[6];
	VFX_window_read = (void (*)(WINDOW*, int32_t, int32_t, int32_t, int32_t))(void (*)(void)) table[7];
	VFX_DAC_read = (void (*)(int32_t, uint8_t*))(void (*)(void)) table[8];
	VFX_DAC_write = (void (*)(int32_t, uint8_t*))(void (*)(void)) table[9];
	VFX_bank_reset = (void (*)(void)) table[10];
	VFX_pane_refresh = (void (*)(PANE*, int32_t, int32_t, int32_t, int32_t))(void (*)(void)) table[11];
	VFX_line_address = (void (*)(int32_t, int32_t, uint8_t**, uint32_t*))(void (*)(void)) table[12];
}

// --- Pixels ---

// Writes a pixel, and returns the one it replaced; -3 when the point lies outside the view.
int32_t VFX_pixel_write(PANE* p_pane, int32_t p_x, int32_t p_y, uint32_t p_color)
{
	ClippedView clip;
	int32_t result = ClipView(&clip, p_pane);
	int32_t x;
	int32_t y;
	uint8_t* pixel;
	uint8_t old;

	if (result) {
		return result;
	}

	x = Add(p_x, clip.m_originX);
	y = Add(p_y, clip.m_originY);
	if (x < clip.m_left || x > clip.m_right || y < clip.m_top || y > clip.m_bottom) {
		return -3;
	}

	pixel = clip.m_pixels + PixelOffset(&clip, x, y);
	old = *pixel;
	*pixel = (uint8_t) p_color;
	return old;
}

int32_t VFX_pixel_read(PANE* p_pane, int32_t p_x, int32_t p_y)
{
	ClippedView clip;
	int32_t result = ClipView(&clip, p_pane);
	int32_t x;
	int32_t y;

	if (result) {
		return result;
	}

	x = Add(p_x, clip.m_originX);
	y = Add(p_y, clip.m_originY);
	if (x < clip.m_left || x > clip.m_right || y < clip.m_top || y > clip.m_bottom) {
		return -3;
	}

	return clip.m_pixels[PixelOffset(&clip, x, y)];
}

// --- VFX_line_draw ---

// VFX_line_draw's modes: 0 (or less) plots p_color; 1 maps each pixel through the 256-byte table
// p_color points to; more calls p_color, a function, once per point (the assembly with the
// point in registers, which a C function can't read). The game only plots; the other two take
// a pointer in a 32-bit word, so they only exist where pointers are 32 bits.
enum LineMode {
	c_linePlot,
	c_lineRemap,
	c_lineCall
};

typedef struct LineDraw {
	uint8_t* m_pixels;
	int32_t m_mode;
	int32_t m_color;
} LineDraw;

static void DrawLinePoint(const LineDraw* p_draw, uint32_t p_offset)
{
	uint8_t* pixel = p_draw->m_pixels + p_offset;

	if (p_draw->m_mode < c_lineRemap) {
		*pixel = (uint8_t) p_draw->m_color;
	}
	else if (p_draw->m_mode == c_lineRemap) {
		*pixel = ((const uint8_t*) (size_t) (uint32_t) p_draw->m_color)[*pixel];
	}
	else {
		((void (*)(void))(size_t) (uint32_t) p_draw->m_color)();
	}
}

// p_count points from p_offset, each p_step bytes after the previous one.
static void DrawLineRun(const LineDraw* p_draw, uint32_t p_offset, uint32_t p_step, uint32_t p_count)
{
	do {
		DrawLinePoint(p_draw, p_offset);
		p_offset += p_step;
	} while (--p_count);
}

// The minor axis's distance for p_distance along the major one: the 0.32 slope's product,
// rounded.
static uint32_t MajorToMinor(uint32_t p_distance, uint32_t p_slope)
{
	return (uint32_t) (((uint64_t) p_distance * p_slope + 0x80000000u) >> 32);
}

// The major axis's distance for p_distance along the minor one, by dividing by the slope (with
// half a pixel added): rounded up for the line's first point (from p_distance - 1), and down
// for its last (less one when the division is exact).
static uint32_t MinorToMajor(uint32_t p_distance, uint32_t p_slope, int32_t p_first)
{
	uint64_t dividend = ((uint64_t) (p_first ? p_distance - 1 : p_distance) << 32) | 0x80000000u;
	uint32_t quotient = PortableDiv(dividend, p_slope);
	uint32_t remainder = (uint32_t) (dividend - (uint64_t) quotient * p_slope);

	if (p_first) {
		return quotient + (remainder != 0);
	}

	return quotient - (remainder == 0);
}

// Negates p_value when p_mask is all ones.
static uint32_t ApplySign(uint32_t p_value, uint32_t p_mask)
{
	return (p_value ^ p_mask) - p_mask;
}

// Draws a line from (p_x1, p_y1) to (p_x2, p_y2), clipped to the view. Returns 0 when it didn't
// need clipping, 1 when it did, and 2 when nothing of it is in the view. For horizontal and
// vertical lines, the assembly returns the clipping flag without having set it; here, 0.
int32_t VFX_line_draw(
	PANE* p_pane,
	int32_t p_x0,
	int32_t p_y0,
	int32_t p_x1,
	int32_t p_y1,
	int32_t p_mode,
	int32_t p_parm
)
{
	ClippedView clip;
	LineDraw draw;
	int32_t result = ClipView(&clip, p_pane);
	int32_t dx;
	int32_t dy;
	uint32_t signX;
	uint32_t signY;
	uint32_t sign;
	int32_t absX;
	int32_t absY;
	uint32_t slope;
	uint32_t clipped;
	int32_t x0;
	int32_t y0;
	int32_t x1;
	int32_t y1;
	uint32_t offset;
	uint32_t rowStep;
	uint32_t fraction;
	uint32_t count;

	if (result) {
		return result;
	}

	PORTABLE_ASSERT(p_mode <= c_lineRemap || sizeof(void (*)(void)) == 4);
	PORTABLE_ASSERT(p_mode != c_lineRemap || sizeof(void*) == 4);
	draw.m_pixels = clip.m_pixels;
	draw.m_mode = p_mode;
	draw.m_color = p_parm;
	p_x0 = Add(p_x0, clip.m_originX);
	p_x1 = Add(p_x1, clip.m_originX);
	p_y0 = Add(p_y0, clip.m_originY);
	p_y1 = Add(p_y1, clip.m_originY);
	dx = Sub(p_x1, p_x0);
	signX = dx < 0 ? 0xffffffff : 0;
	absX = Abs(dx);
	dy = Sub(p_y1, p_y0);
	signY = dy < 0 ? 0xffffffff : 0;
	absY = Abs(dy);
	x0 = p_x0;
	x1 = p_x1;
	y0 = p_y0;
	y1 = p_y1;
	rowStep = (clip.m_pitch ^ signY) - signY;

	if (dx == 0) {
		int32_t high = p_y0 > p_y1 ? p_y0 : p_y1;
		int32_t low = p_y0 < p_y1 ? p_y0 : p_y1;

		if (p_x0 < clip.m_left || p_x0 > clip.m_right || high < clip.m_top || low > clip.m_bottom) {
			return 2;
		}

		y0 = p_y0 > clip.m_top ? p_y0 : clip.m_top;
		y0 = y0 < clip.m_bottom ? y0 : clip.m_bottom;
		y1 = p_y1 > clip.m_top ? p_y1 : clip.m_top;
		y1 = y1 < clip.m_bottom ? y1 : clip.m_bottom;
		count = (uint32_t) Abs(Sub(y1, y0)) + 1;
		DrawLineRun(&draw, PixelOffset(&clip, x0, y0), rowStep, count);
		return 0;
	}

	if (dy == 0) {
		int32_t high = p_x0 > p_x1 ? p_x0 : p_x1;
		int32_t low = p_x0 < p_x1 ? p_x0 : p_x1;

		if (p_y0 < clip.m_top || p_y0 > clip.m_bottom || high < clip.m_left || low > clip.m_right) {
			return 2;
		}

		x0 = p_x0 > clip.m_left ? p_x0 : clip.m_left;
		x0 = x0 < clip.m_right ? x0 : clip.m_right;
		x1 = p_x1 > clip.m_left ? p_x1 : clip.m_left;
		x1 = x1 < clip.m_right ? x1 : clip.m_right;
		count = (uint32_t) Abs(Sub(x1, x0)) + 1;
		DrawLineRun(&draw, PixelOffset(&clip, x0, y0), signX ? 0xffffffff : 1, count);
		return 0;
	}

	// The slope: the minor axis's distance over the major one's, 0.32 (all ones when equal)
	sign = signX ^ signY;
	if (absX == absY) {
		slope = 0xffffffff;
	}
	else if (absX < absY) {
		slope = PortableDiv((uint64_t) (uint32_t) absX << 32, (uint32_t) absY);
	}
	else {
		slope = PortableDiv((uint64_t) (uint32_t) absY << 32, (uint32_t) absX);
	}

	// Clip both ends to the view, recomputing each moved point from the line's start
	clipped = 0;
	for (;;) {
		uint32_t first = OutCode(&clip, x0, y0);
		uint32_t last = OutCode(&clip, x1, y1);
		uint32_t left = (uint32_t) clip.m_left;
		uint32_t top = (uint32_t) clip.m_top;
		uint32_t right = (uint32_t) clip.m_right;
		uint32_t bottom = (uint32_t) clip.m_bottom;
		uint32_t startX = (uint32_t) p_x0;
		uint32_t startY = (uint32_t) p_y0;

		clipped |= first | last << 8;
		if (!(first | last)) {
			break;
		}

		if (first & last) {
			return 2;
		}

		if (absX >= absY) {
			if (first & 8) {
				x0 = clip.m_left;
				y0 = PortableS32(startY + ApplySign(MajorToMinor(left - startX, slope), sign));
			}
			else if (first & 4) {
				x0 = clip.m_right;
				y0 = PortableS32(startY + ApplySign(MajorToMinor(startX - right, slope), ~sign));
			}
			else if (first & 2) {
				y0 = clip.m_top;
				x0 = PortableS32(startX + ApplySign(MinorToMajor(top - startY, slope, 1), sign));
			}
			else if (first & 1) {
				y0 = clip.m_bottom;
				x0 = PortableS32(startX + ApplySign(MinorToMajor(startY - bottom, slope, 1), ~sign));
			}
			else if (last & 8) {
				x1 = clip.m_left;
				y1 = PortableS32(startY + ApplySign(MajorToMinor(startX - left, slope), ~sign));
			}
			else if (last & 4) {
				x1 = clip.m_right;
				y1 = PortableS32(startY + ApplySign(MajorToMinor(right - startX, slope), sign));
			}
			else if (last & 2) {
				y1 = clip.m_top;
				x1 = PortableS32(startX + ApplySign(MinorToMajor(startY - top, slope, 0), ~sign));
			}
			else {
				y1 = clip.m_bottom;
				x1 = PortableS32(startX + ApplySign(MinorToMajor(bottom - startY, slope, 0), sign));
			}
		}
		else {
			if (first & 8) {
				x0 = clip.m_left;
				y0 = PortableS32(startY + ApplySign(MinorToMajor(left - startX, slope, 1), sign));
			}
			else if (first & 4) {
				x0 = clip.m_right;
				y0 = PortableS32(startY + ApplySign(MinorToMajor(startX - right, slope, 1), ~sign));
			}
			else if (first & 2) {
				y0 = clip.m_top;
				x0 = PortableS32(startX + ApplySign(MajorToMinor(top - startY, slope), sign));
			}
			else if (first & 1) {
				y0 = clip.m_bottom;
				x0 = PortableS32(startX + ApplySign(MajorToMinor(startY - bottom, slope), ~sign));
			}
			else if (last & 8) {
				x1 = clip.m_left;
				y1 = PortableS32(startY + ApplySign(MinorToMajor(startX - left, slope, 0), ~sign));
			}
			else if (last & 4) {
				x1 = clip.m_right;
				y1 = PortableS32(startY + ApplySign(MinorToMajor(right - startX, slope, 0), sign));
			}
			else if (last & 2) {
				y1 = clip.m_top;
				x1 = PortableS32(startX + ApplySign(MajorToMinor(startY - top, slope), ~sign));
			}
			else {
				y1 = clip.m_bottom;
				x1 = PortableS32(startX + ApplySign(MajorToMinor(bottom - startY, slope), sign));
			}
		}
	}

	offset = PixelOffset(&clip, x0, y0);
	if (absX == absY) {
		count = (uint32_t) Abs(Sub(x1, x0)) + 1;
		DrawLineRun(&draw, offset, rowStep + (signX ? 0xffffffff : 1), count);
	}
	else if (absX > absY) {
		// Along x, a row whenever the fraction carries
		count = (uint32_t) Abs(Sub(x1, x0)) + 1;
		fraction = (uint32_t) Abs(Sub(x0, p_x0)) * slope + 0x80000000u;
		do {
			DrawLinePoint(&draw, offset);
			offset += signX ? 0xffffffff : 1;
			fraction += slope;
			if (fraction < slope) {
				offset += rowStep;
			}
		} while (--count);
	}
	else {
		// Along y, a column whenever the fraction carries
		count = (uint32_t) Abs(Sub(y1, y0)) + 1;
		fraction = (uint32_t) Abs(Sub(y0, p_y0)) * slope + 0x80000000u;
		do {
			DrawLinePoint(&draw, offset);
			offset += rowStep;
			fraction += slope;
			if (fraction < slope) {
				offset += signX ? 0xffffffff : 1;
			}
		} while (--count);
	}

	return clipped != 0;
}

// Fills the rectangle from (p_left, p_top) to (p_right, p_bottom), clipped to the view, with
// every other pixel, in a checkerboard that counts from the rectangle's bottom right. A
// rectangle one pixel wide is drawn one pixel to the right on alternate rows.
void VFX_rectangle_hash(PANE* p_pane, int32_t p_x0, int32_t p_y0, int32_t p_x1, int32_t p_y1, uint8_t p_color)
{
	ClippedView clip;
	uint32_t row;
	int32_t width;
	int32_t rows;

	if (ClipView(&clip, p_pane)) {
		return;
	}

	p_x0 = Add(p_x0, clip.m_originX);
	p_x1 = Add(p_x1, clip.m_originX);
	p_y0 = Add(p_y0, clip.m_originY);
	p_y1 = Add(p_y1, clip.m_originY);
	p_x0 = p_x0 > clip.m_left ? p_x0 : clip.m_left;
	p_y0 = p_y0 > clip.m_top ? p_y0 : clip.m_top;
	p_x1 = p_x1 < clip.m_right ? p_x1 : clip.m_right;
	p_y1 = p_y1 < clip.m_bottom ? p_y1 : clip.m_bottom;
	if (p_x1 < p_x0 || p_y1 < p_y0) {
		return;
	}

	width = Add(Sub(p_x1, p_x0), 1);
	rows = Sub(p_y1, p_y0);
	row = PixelOffset(&clip, p_x0, p_y0);
	for (; rows >= 0; rows--) {
		uint32_t offset = row;
		int32_t count = width;

		if (rows & 1) {
			offset++;
			count--;
		}

		do {
			clip.m_pixels[offset] = p_color;
			offset += 2;
			count -= 2;
		} while (count > 0);

		row += clip.m_pitch;
	}
}

// --- SHP frames ---

// An SHP animation: a 8-byte header (the frame count at +4), then 8-byte entries, each a frame's
// offset from the start and its palette's (0 for none). A frame has a 0x18-byte header (its
// size, at +8 to +0x14 the left, top, right and bottom of its pixels from the origin), then
// run-length codes, a row at a time: 0 ends the row, 1 skips the next byte's count of pixels,
// 2n draws n pixels of the next byte's color, and 2n + 1 copies the next n bytes.
#define SHP_FRAME_HEADER 0x18

static const uint8_t* ShpEntry(const void* p_shape, int32_t p_frame)
{
	return (const uint8_t*) p_shape + 8 + (uint32_t) p_frame * 8;
}

static uint8_t* ShpFrame(void* p_shape, int32_t p_frame)
{
	return (uint8_t*) p_shape + ReadU32(ShpEntry(p_shape, p_frame));
}

// The bytes after a run's or a literal's code that its data takes.
static uint32_t ShpCodeData(uint32_t p_code)
{
	return p_code & 1 ? p_code >> 1 : 1;
}

// The frame's rows: bottom + 1 - top, 0 when that isn't positive.
static uint32_t ShpFrameRows(const uint8_t* p_frame)
{
	int32_t end = Add(ReadS32(p_frame + 0x14), 1);
	int32_t top = ReadS32(p_frame + 0xc);

	return end > top ? (uint32_t) end - (uint32_t) top : 0;
}

static const uint8_t* SkipShpRow(const uint8_t* p_codes)
{
	for (;;) {
		uint32_t code = *p_codes++;

		if (code == 0) {
			return p_codes;
		}

		p_codes += code == 1 ? 1 : ShpCodeData(code);
	}
}

// How a row's codes draw: everything, only the part right of the left edge (then on as
// c_rowNormal or c_rowRight), only the part left of the right edge, or nothing more.
enum ShpRowState {
	c_rowNormal,
	c_rowLeft,
	c_rowRight,
	c_rowPast
};

typedef struct ShpRow {
	uint8_t* m_pixels;
	int32_t m_leftEdge; // the offsets of the clipped view's first and last pixel on the row
	int32_t m_rightEdge;
	int32_t m_clipRight; // whether to clip at the right edge, after the left one
	const uint8_t* m_remap;
} ShpRow;

// Draws a run (p_code even) or copies a literal, of p_count pixels at p_at.
static void DrawShpRun(const ShpRow* p_row, int32_t p_at, uint32_t p_code, const uint8_t* p_data, uint32_t p_count)
{
	uint8_t* pixels = p_row->m_pixels + (uint32_t) p_at;
	uint32_t i;

	if (!(p_code & 1)) {
		uint8_t color = p_row->m_remap ? p_row->m_remap[*p_data] : *p_data;

		memset(pixels, color, p_count);
	}
	else if (p_row->m_remap) {
		for (i = 0; i < p_count; i++) {
			pixels[i] = p_row->m_remap[p_data[i]];
		}
	}
	else {
		memcpy(pixels, p_data, p_count);
	}
}

// Draws a row of codes from p_at, and returns where the next row's start.
static const uint8_t* DrawShpRow(const ShpRow* p_row, int32_t p_at, const uint8_t* p_codes, int32_t p_state)
{
	for (;;) {
		uint32_t code = *p_codes++;
		uint32_t count = code >> 1;

		if (code == 0) {
			return p_codes;
		}

		if (code == 1) {
			p_at = Add(p_at, *p_codes++);
			continue;
		}

		if (p_state == c_rowLeft) {
			int32_t before = Sub(p_row->m_leftEdge, p_at);

			if (before >= (int32_t) count) {
				p_at = Add(p_at, (int32_t) count);
				p_codes += code & 1 ? count : 1;
				continue;
			}

			if (before >= 0) {
				p_at = Add(p_at, before);
				count -= (uint32_t) before;
				if (code & 1) {
					p_codes += before;
				}
			}

			p_state = p_row->m_clipRight ? c_rowRight : c_rowNormal;
		}

		if (p_state == c_rowRight && p_at > p_row->m_rightEdge) {
			p_state = c_rowPast;
		}

		if (p_state == c_rowPast) {
			p_at = Add(p_at, (int32_t) count);
			p_codes += code & 1 ? count : 1;
			continue;
		}

		if (p_state == c_rowRight) {
			int32_t over = Sub(Sub(Add(p_at, (int32_t) count), 1), p_row->m_rightEdge);

			if (over < 0) {
				over = 0;
			}

			DrawShpRun(p_row, p_at, code, p_codes, count - (uint32_t) over);
			p_at = Add(p_at, (int32_t) count);
			p_codes += code & 1 ? count : 1;
			continue;
		}

		DrawShpRun(p_row, p_at, code, p_codes, count);
		p_at = Add(p_at, (int32_t) count);
		p_codes += code & 1 ? count : 1;
	}
}

// Draws a frame (its header) at (p_x, p_y) from the view's origin, without clipping.
static void DrawShapeUnclippedWith(
	PANE* p_view,
	const uint8_t* p_frame,
	int32_t p_x,
	int32_t p_y,
	const uint8_t* p_remap
)
{
	const WINDOW* buffer = p_view->m_window;
	const uint8_t* codes = p_frame + SHP_FRAME_HEADER;
	uint32_t pitch;
	uint32_t row;
	uint32_t rows;
	ShpRow draw;

	p_x = Add(p_x, p_view->m_x0);
	p_y = Add(p_y, p_view->m_y0);
	if (buffer->m_xMax < 0) {
		return;
	}

	pitch = (uint32_t) buffer->m_xMax + 1;
	row = (uint32_t) Add(ReadS32(p_frame + 8), p_x) + (uint32_t) Add(ReadS32(p_frame + 0xc), p_y) * pitch;
	rows = ShpFrameRows(p_frame);
	draw.m_pixels = buffer->m_buffer;
	draw.m_leftEdge = 0;
	draw.m_rightEdge = 0;
	draw.m_clipRight = 0;
	draw.m_remap = p_remap;
	for (; rows; rows--) {
		codes = DrawShpRow(&draw, PortableS32(row), codes, c_rowNormal);
		row += pitch;
	}
}

void DrawShapeUnclipped(PANE* p_pane, void* p_shape, int32_t p_hotX, int32_t p_hotY, uint32_t p_cpW)
{
	DrawShapeUnclippedWith(p_pane, (const uint8_t*) p_shape, p_hotX, p_hotY, NULL);
}

int32_t XlatShapeUnclipped(PANE* p_pane, void* p_shape, int32_t p_hotX, int32_t p_hotY, uint32_t p_cpW)
{
	DrawShapeUnclippedWith(p_pane, (const uint8_t*) p_shape, p_hotX, p_hotY, lookaside);
	return 0;
}

// Draws frame p_frame of an SHP animation at (p_x, p_y), clipped to the view. Returns -4 for a
// frame without pixels, -3 when it lies beyond the view, else 0.
static int32_t ShapeDrawWith(
	PANE* p_view,
	void* p_shape,
	int32_t p_frame,
	int32_t p_x,
	int32_t p_y,
	const uint8_t* p_remap
)
{
	ClippedView clip;
	int32_t result = ClipView(&clip, p_view);
	const uint8_t* frame;
	const uint8_t* codes;
	uint32_t first;
	uint32_t last;
	int32_t x0;
	int32_t y0;
	int32_t x1;
	int32_t y1;
	int32_t y;
	uint32_t row;
	ShpRow draw;

	if (result) {
		return result;
	}

	p_x = Add(p_x, clip.m_originX);
	p_y = Add(p_y, clip.m_originY);
	frame = ShpFrame(p_shape, p_frame);
	codes = frame + SHP_FRAME_HEADER;
	x0 = Add(ReadS32(frame + 8), p_x);
	y0 = Add(ReadS32(frame + 0xc), p_y);
	x1 = Add(ReadS32(frame + 0x10), p_x);
	y1 = Add(ReadS32(frame + 0x14), p_y);
	if (x1 < x0 || y1 < y0) {
		return -4;
	}

	first = OutCode(&clip, x0, y0);
	last = OutCode(&clip, x1, y1);
	if (first & last) {
		return -3;
	}

	if (!(first | last)) {
		DrawShapeUnclippedWith(p_view, frame, Sub(p_x, clip.m_originX), Sub(p_y, clip.m_originY), p_remap);
		return 0;
	}

	// The rows above the view
	row = PixelOffset(&clip, x0, y0);
	for (y = y0; y < clip.m_top; y++) {
		codes = SkipShpRow(codes);
		row += clip.m_pitch;
	}

	draw.m_pixels = clip.m_pixels;
	draw.m_leftEdge = PortableS32(row - (uint32_t) x0 + (uint32_t) clip.m_left);
	draw.m_rightEdge = PortableS32(row - (uint32_t) x0 + (uint32_t) clip.m_right);
	draw.m_clipRight = (last & 4) != 0;
	draw.m_remap = p_remap;
	for (; y <= y1; y++) {
		if (y > clip.m_bottom) {
			break;
		}

		codes = DrawShpRow(&draw, PortableS32(row), codes, first & 8 ? c_rowLeft : last & 4 ? c_rowRight : c_rowNormal);
		row += clip.m_pitch;
		draw.m_leftEdge = Add(draw.m_leftEdge, (int32_t) clip.m_pitch);
		draw.m_rightEdge = Add(draw.m_rightEdge, (int32_t) clip.m_pitch);
	}

	return 0;
}

void VFX_shape_draw(PANE* p_pane, void* p_shapeTable, int32_t p_shapeNumber, int32_t p_hotX, int32_t p_hotY)
{
	ShapeDrawWith(p_pane, p_shapeTable, p_shapeNumber, p_hotX, p_hotY, NULL);
}

void VFX_shape_lookaside(uint8_t* p_table)
{
	memcpy(lookaside, p_table, sizeof(lookaside));
}

// VFX_shape_draw through lookaside.
int32_t VFX_shape_translate_draw(
	PANE* p_pane,
	void* p_shapeTable,
	int32_t p_shapeNumber,
	int32_t p_hotX,
	int32_t p_hotY
)
{
	return ShapeDrawWith(p_pane, p_shapeTable, p_shapeNumber, p_hotX, p_hotY, lookaside);
}

// Maps frame p_frame's pixels through lookaside, in place.
int32_t VFX_shape_remap_colors(void* p_shapeTable, int32_t p_shapeNumber)
{
	uint8_t* frame = ShpFrame(p_shapeTable, p_shapeNumber);
	uint8_t* codes = frame + SHP_FRAME_HEADER;
	uint32_t rows = ShpFrameRows(frame);

	for (; rows; rows--) {
		for (;;) {
			uint32_t code = *codes++;
			uint32_t i;

			if (code == 0) {
				break;
			}

			if (code == 1) {
				codes++;
			}
			else if (code & 1) {
				for (i = 0; i < code >> 1; i++) {
					codes[i] = lookaside[codes[i]];
				}

				codes += code >> 1;
			}
			else {
				*codes = lookaside[*codes];
				codes++;
			}
		}
	}

	return 0;
}

// The bounds of frame p_frame's pixels drawn at (p_x, p_y): the least and the greatest x (one
// past the last pixel) and y of its runs, mirrored about p_x when p_flags has bit 0 and about
// p_y when it has bit 1, written to p_bounds as left, top, right, bottom.
int32_t VFX_shape_visible_rectangle(
	void* p_shapeTable,
	int32_t p_shapeNumber,
	int32_t p_hotX,
	int32_t p_hotY,
	uint32_t p_mirror,
	int32_t* p_rectangle
)
{
	const uint8_t* frame = ShpFrame(p_shapeTable, p_shapeNumber);
	const uint8_t* codes = frame + SHP_FRAME_HEADER;
	int32_t left = 0;
	int32_t top = 0;
	int32_t right = 0;
	int32_t bottom = 0;
	uint32_t rows = ShpFrameRows(frame);
	int32_t y = Add(ReadS32(frame + 0xc), p_hotY);
	int32_t swap;

	if (rows) {
		left = 0x7fffffff;
		top = 0x7fffffff;
		right = -0x7fffffff - 1;
		bottom = -0x7fffffff - 1;
		for (; rows; rows--) {
			int32_t x = Add(ReadS32(frame + 8), p_hotX);

			for (;;) {
				uint32_t code = *codes++;

				if (code == 0) {
					break;
				}

				if (code == 1) {
					x = Add(x, *codes++);
					continue;
				}

				codes += ShpCodeData(code);
				if (left >= x) {
					left = x;
				}

				x = Add(x, (int32_t) (code >> 1));
				if (right <= x) {
					right = x;
				}

				if (top >= y) {
					top = y;
				}

				if (bottom <= y) {
					bottom = y;
				}
			}

			y = Add(y, 1);
		}
	}

	if (p_mirror & 1) {
		swap = left;
		left = Sub(Add(p_hotX, p_hotX), right);
		right = Sub(Add(p_hotX, p_hotX), swap);
	}

	if (p_mirror & 2) {
		swap = top;
		top = Sub(Add(p_hotY, p_hotY), bottom);
		bottom = Sub(Add(p_hotY, p_hotY), swap);
	}

	p_rectangle[0] = left;
	p_rectangle[1] = top;
	p_rectangle[2] = right;
	p_rectangle[3] = bottom;
	return 0;
}

// --- Frame queries ---

// The frame's size, (width - 1) << 16 | (height - 1): its first dword.
int32_t VFX_shape_bounds(void* p_shapeTable, int32_t p_shapeNum)
{
	return ReadS32(ShpFrame(p_shapeTable, p_shapeNum));
}

int32_t VFX_shape_origin(void* p_shapeTable, int32_t p_shapeNum)
{
	return ReadS32(ShpFrame(p_shapeTable, p_shapeNum) + 4);
}

// The frame's width << 16 | height, from its bounds.
int32_t VFX_shape_resolution(void* p_shapeTable, int32_t p_shapeNum)
{
	const uint8_t* frame = ShpFrame(p_shapeTable, p_shapeNum);
	uint32_t width = (uint32_t) ReadS32(frame + 0x10) - (uint32_t) ReadS32(frame + 8) + 1;
	uint32_t height = (uint32_t) ReadS32(frame + 0x14) - (uint32_t) ReadS32(frame + 0xc) + 1;

	return PortableS32(width << 16 | (height & 0xffff));
}

// The frame's left << 16 | top.
int32_t VFX_shape_minxy(void* p_shapeTable, int32_t p_shapeNum)
{
	const uint8_t* frame = ShpFrame(p_shapeTable, p_shapeNum);

	return PortableS32(ReadU32(frame + 8) << 16 | ReadU16(frame + 0xc));
}

// The frame's palette entries: a count (at least 1), then for each a color index and its three
// components.
static const uint8_t* ShpPalette(const void* p_shape, int32_t p_frame)
{
	uint32_t offset = ReadU32(ShpEntry(p_shape, p_frame) + 4);

	return offset ? (const uint8_t*) p_shape + offset : NULL;
}

void VFX_shape_palette(void* p_shapeTable, int32_t p_shapeNum, uint8_t* p_palette)
{
	const uint8_t* entries = ShpPalette(p_shapeTable, p_shapeNum);
	uint32_t count;

	if (!entries) {
		return;
	}

	count = ReadU32(entries);
	PORTABLE_ASSERT(count != 0);
	entries += 4;
	do {
		uint32_t color = entries[0] * 3;

		p_palette[color] = entries[1];
		p_palette[color + 1] = entries[2];
		p_palette[color + 2] = entries[3];
		entries += 4;
	} while (--count);
}

// Copies the frame's palette entries (dwords) to p_out when not NULL. Returns their count, 0 for
// none.
int32_t VFX_shape_colors(void* p_shapeTable, int32_t p_shapeNum, uint32_t* p_colors)
{
	const uint8_t* entries = ShpPalette(p_shapeTable, p_shapeNum);
	uint32_t count;
	uint32_t i;

	if (!entries) {
		return 0;
	}

	count = ReadU32(entries);
	if (p_colors) {
		PORTABLE_ASSERT(count != 0);
		for (i = 0; i < count; i++) {
			p_colors[i] = ReadU32(entries + 4 + i * 4);
		}
	}

	return PortableS32(count);
}

// Copies p_in, when not NULL, over the frame's palette entries. Returns their count, 0 for none.
int32_t VFX_shape_set_colors(void* p_shapeTable, int32_t p_shapeNumber, uint32_t* p_colors)
{
	uint8_t* entries = (uint8_t*) ShpPalette(p_shapeTable, p_shapeNumber);
	uint32_t count;
	uint32_t i;

	if (!entries) {
		return 0;
	}

	count = ReadU32(entries);
	if (p_colors) {
		PORTABLE_ASSERT(count != 0);
		for (i = 0; i < count; i++) {
			WriteU32(entries + 4 + i * 4, p_colors[i]);
		}
	}

	return PortableS32(count);
}

int32_t VFX_shape_count(void* p_shapeTable)
{
	return ReadS32((const uint8_t*) p_shapeTable + 4);
}

// Counts the distinct dwords at p_first, p_first + 8... (one per frame), and stores the index of
// each first occurrence in p_out when not NULL. The shape has at least one frame.
static int32_t CountDistinctEntries(const void* p_shape, const uint8_t* p_first, int32_t* p_out)
{
	uint32_t count = ReadU32((const uint8_t*) p_shape + 4);
	int32_t distinct = 1;
	uint32_t i;
	uint32_t j;

	PORTABLE_ASSERT(count != 0);
	if (p_out) {
		*p_out++ = 0;
	}

	for (i = 1; i < count; i++) {
		uint32_t value = ReadU32(p_first + i * 8);

		for (j = 0; j < i; j++) {
			if (ReadU32(p_first + j * 8) == value) {
				break;
			}
		}

		if (j == i) {
			if (p_out) {
				*p_out++ = (int32_t) i;
			}

			distinct++;
		}
	}

	return distinct;
}

// The frames with distinct offsets.
int32_t VFX_shape_list(void* p_shapeTable, int32_t* p_indexList)
{
	return CountDistinctEntries(p_shapeTable, (const uint8_t*) p_shapeTable + 8, p_indexList);
}

// The frames with distinct palettes.
int32_t VFX_shape_palette_list(void* p_shapeTable, int32_t* p_indexList)
{
	return CountDistinctEntries(p_shapeTable, (const uint8_t*) p_shapeTable + 0xc, p_indexList);
}

// --- Views ---

static int32_t PaneWipeWith(PANE* p_view, uint8_t p_color)
{
	ClippedView clip;
	int32_t result = ClipView(&clip, p_view);
	uint32_t row;
	uint32_t width;
	int32_t y;

	if (result) {
		return result;
	}

	row = PixelOffset(&clip, clip.m_left, clip.m_top);
	width = (uint32_t) clip.m_right + 1 - (uint32_t) clip.m_left;
	for (y = clip.m_top; y <= clip.m_bottom; y++) {
		memset(clip.m_pixels + row, p_color, width);
		row += clip.m_pitch;
	}

	return 0;
}

// Fills the view, clipped to its buffer.
void VFX_pane_wipe(PANE* p_pane, int32_t p_color)
{
	PaneWipeWith(p_pane, (uint8_t) p_color);
}

// Copies a row as `rep movsb` then `rep movsd` do: the odd bytes, then dwords (each read whole
// before it's written), forwards from its start, or backwards from its end. Where the row
// overlaps itself, the order decides what's copied.
static void CopyRow(uint8_t* p_to, const uint8_t* p_from, uint32_t p_width, int32_t p_backwards)
{
	uint32_t odd = p_width & 3;
	uint32_t i;

	for (i = 0; i < odd; i++) {
		uint32_t at = p_backwards ? p_width - 1 - i : i;

		p_to[at] = p_from[at];
	}

	for (i = 0; i < p_width >> 2; i++) {
		uint32_t at = p_backwards ? p_width - odd - 4 - i * 4 : odd + i * 4;
		uint8_t dword[4];

		memcpy(dword, p_from + at, 4);
		memcpy(p_to + at, dword, 4);
	}
}

// Copies the rectangle at (p_sourceX, p_sourceY) of p_source to (p_destX, p_destY) of p_dest,
// clipped to both views; a p_fillColor from 0 to 0xff fills it instead. The views may share a
// buffer: the rows go down when the source's rectangle starts lower in its view than the
// destination's in its own, else up, and each row forwards when the source's starts further
// right, else backwards. That copies an overlapping rectangle whole when the views share their
// origin, not always otherwise. Returns -3 when nothing is left of the rectangle.
int32_t VFX_pane_copy(
	PANE* p_source,
	int32_t p_sx,
	int32_t p_sy,
	PANE* p_target,
	int32_t p_tx,
	int32_t p_ty,
	int32_t p_fill
)
{
	ClippedView source;
	ClippedView dest;
	int32_t result;
	int32_t sourceLeft;
	int32_t sourceTop;
	int32_t sourceRight;
	int32_t sourceBottom;
	int32_t destLeft;
	int32_t destTop;
	int32_t destRight;
	int32_t destBottom;
	int32_t offsetX;
	int32_t offsetY;
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
	int32_t toLeft;
	int32_t toTop;
	int32_t toBottom;
	uint32_t width;
	uint32_t height;
	uint32_t from;
	uint32_t to;
	uint32_t fromStep;
	uint32_t toStep;
	int32_t backwards;

	result = ClipView(&source, p_source);
	if (result) {
		return result;
	}

	result = ClipView(&dest, p_target);
	if (result) {
		return result;
	}

	// Both clipped rectangles from their views' origins, and their intersection in the source's
	sourceLeft = Sub(source.m_left, source.m_originX);
	sourceTop = Sub(source.m_top, source.m_originY);
	sourceRight = Sub(source.m_right, source.m_originX);
	sourceBottom = Sub(source.m_bottom, source.m_originY);
	destLeft = Sub(dest.m_left, dest.m_originX);
	destTop = Sub(dest.m_top, dest.m_originY);
	destRight = Sub(dest.m_right, dest.m_originX);
	destBottom = Sub(dest.m_bottom, dest.m_originY);
	offsetX = Sub(p_sx, p_tx);
	offsetY = Sub(p_sy, p_ty);
	left = sourceLeft > Add(destLeft, offsetX) ? sourceLeft : Add(destLeft, offsetX);
	top = sourceTop > Add(destTop, offsetY) ? sourceTop : Add(destTop, offsetY);
	right = sourceRight < Add(destRight, offsetX) ? sourceRight : Add(destRight, offsetX);
	bottom = sourceBottom < Add(destBottom, offsetY) ? sourceBottom : Add(destBottom, offsetY);
	if (right < left || bottom < top) {
		return -3;
	}

	toLeft = destLeft > Sub(sourceLeft, offsetX) ? destLeft : Sub(sourceLeft, offsetX);
	toTop = destTop > Sub(sourceTop, offsetY) ? destTop : Sub(sourceTop, offsetY);
	toBottom = destBottom < Sub(sourceBottom, offsetY) ? destBottom : Sub(sourceBottom, offsetY);
	width = (uint32_t) right + 1 - (uint32_t) left;
	height = (uint32_t) bottom + 1 - (uint32_t) top;
	from = PixelOffset(&source, source.m_originX, source.m_originY) + (uint32_t) left;
	to = PixelOffset(&dest, dest.m_originX, dest.m_originY) + (uint32_t) toLeft;
	if (top > toTop) {
		from += (uint32_t) top * source.m_pitch;
		to += (uint32_t) toTop * dest.m_pitch;
		fromStep = source.m_pitch;
		toStep = dest.m_pitch;
	}
	else {
		from += (uint32_t) bottom * source.m_pitch;
		to += (uint32_t) toBottom * dest.m_pitch;
		fromStep = 0 - source.m_pitch;
		toStep = 0 - dest.m_pitch;
	}

	backwards = !(left > toLeft);
	do {
		if (p_fill & 0xffffff00) {
			CopyRow(dest.m_pixels + to, source.m_pixels + from, width, backwards);
		}
		else {
			memset(dest.m_pixels + to, (uint8_t) p_fill, width);
		}

		from += fromStep;
		to += toStep;
	} while (--height);

	return 0;
}

// Scrolls the view by (p_dx, p_dy). Mode 1 wraps it around, through p_color, a scratch buffer of
// the view's size (where pointers are 32 bits; with p_color 0, it returns the size instead);
// another mode fills what the scroll uncovers with p_color, and the whole view when the scroll
// is the view's size or more. Returns -2 for an empty view.
int32_t VFX_pane_scroll(PANE* p_pane, int32_t p_dx, int32_t p_dy, int32_t p_mode, int32_t p_parm)
{
	WINDOW scratch;
	PANE scratchView;
	PANE* source;
	int32_t width;
	int32_t height;
	int32_t fill;

	if (Add(p_pane->m_x1, 1) <= p_pane->m_x0 || Add(p_pane->m_y1, 1) <= p_pane->m_y0) {
		return -2;
	}

	width = Sub(Add(p_pane->m_x1, 1), p_pane->m_x0);
	height = Sub(Add(p_pane->m_y1, 1), p_pane->m_y0);
	if (p_mode != 1) {
		if (Abs(p_dx) >= width || Abs(p_dy) >= height) {
			return PaneWipeWith(p_pane, (uint8_t) p_parm);
		}

		source = p_pane;
		fill = p_parm;
	}
	else {
		if (p_parm == 0) {
			return PortableS32((uint32_t) width * (uint32_t) height);
		}

		PORTABLE_ASSERT(sizeof(void*) == 4);
		scratch.m_buffer = (uint8_t*) (size_t) (uint32_t) p_parm;
		scratch.m_xMax = width - 1;
		scratch.m_yMax = height - 1;
		scratch.m_stencil = NULL;
		scratch.m_shadow = 0;
		scratchView.m_window = &scratch;
		scratchView.m_x0 = 0;
		scratchView.m_y0 = 0;
		scratchView.m_x1 = width - 1;
		scratchView.m_y1 = height - 1;
		VFX_pane_copy(p_pane, 0, 0, &scratchView, 0, 0, -1);
		p_dx %= width;
		p_dy %= height;
		source = &scratchView;
		fill = -1;
	}

	if (!(p_dx | p_dy)) {
		return 0;
	}

	VFX_pane_copy(source, 0, 0, p_pane, p_dx, p_dy, -1);
	VFX_pane_copy(source, width, height, p_pane, p_dx, p_dy, fill);
	VFX_pane_copy(source, width, 0, p_pane, p_dx, p_dy, fill);
	VFX_pane_copy(source, width, -height, p_pane, p_dx, p_dy, fill);
	VFX_pane_copy(source, 0, height, p_pane, p_dx, p_dy, fill);
	VFX_pane_copy(source, 0, -height, p_pane, p_dx, p_dy, fill);
	VFX_pane_copy(source, -width, height, p_pane, p_dx, p_dy, fill);
	VFX_pane_copy(source, -width, 0, p_pane, p_dx, p_dy, fill);
	VFX_pane_copy(source, -width, -height, p_pane, p_dx, p_dy, fill);
	return 0;
}

// --- Ellipses ---

// The midpoint ellipse of VFX_ellipse_draw and VFX_ellipse_fill, in 32-bit sums that wrap.
typedef struct Ellipse {
	ClippedView m_clip;
	int32_t m_centerX;
	int32_t m_centerY;
	uint8_t m_color;
	int32_t m_x; // the point on the first quadrant's arc, from the center
	int32_t m_y;
} Ellipse;

typedef void (*EllipsePlotFn)(const Ellipse* p_ellipse);

static void PutEllipsePixel(const Ellipse* p_ellipse, int32_t p_x, int32_t p_y)
{
	const ClippedView* clip = &p_ellipse->m_clip;

	if (p_x >= clip->m_left && p_x <= clip->m_right && p_y >= clip->m_top && p_y <= clip->m_bottom) {
		clip->m_pixels[PixelOffset(clip, p_x, p_y)] = p_ellipse->m_color;
	}
}

// The four points symmetric to the arc's.
static void PlotEllipsePoints(const Ellipse* p_ellipse)
{
	int32_t right = Add(p_ellipse->m_centerX, p_ellipse->m_x);
	int32_t left = Sub(p_ellipse->m_centerX, p_ellipse->m_x);
	int32_t below = Add(p_ellipse->m_centerY, p_ellipse->m_y);
	int32_t above = Sub(p_ellipse->m_centerY, p_ellipse->m_y);

	PutEllipsePixel(p_ellipse, right, below);
	PutEllipsePixel(p_ellipse, right, above);
	PutEllipsePixel(p_ellipse, left, below);
	PutEllipsePixel(p_ellipse, left, above);
}

// The two rows between the arc's symmetric points, clipped. The test of the lower row's top
// skips both.
static void FillEllipseRows(const Ellipse* p_ellipse)
{
	const ClippedView* clip = &p_ellipse->m_clip;
	int32_t right = Add(p_ellipse->m_centerX, p_ellipse->m_x);
	int32_t left = Sub(p_ellipse->m_centerX, p_ellipse->m_x);
	int32_t y = Add(p_ellipse->m_centerY, p_ellipse->m_y);
	uint32_t count;

	if (right < clip->m_left || left > clip->m_right || y < clip->m_top) {
		return;
	}

	right = right < clip->m_right ? right : clip->m_right;
	left = left > clip->m_left ? left : clip->m_left;
	count = (uint32_t) right - (uint32_t) left + 1;
	if (y <= clip->m_bottom) {
		memset(clip->m_pixels + PixelOffset(clip, left, y), p_ellipse->m_color, count);
	}

	y = Sub(p_ellipse->m_centerY, p_ellipse->m_y);
	if (y >= clip->m_top && y <= clip->m_bottom) {
		memset(clip->m_pixels + PixelOffset(clip, left, y), p_ellipse->m_color, count);
	}
}

static void DrawEllipseWith(
	PANE* p_view,
	int32_t p_centerX,
	int32_t p_centerY,
	int32_t p_radiusX,
	int32_t p_radiusY,
	int32_t p_color,
	EllipsePlotFn p_plot
)
{
	Ellipse ellipse;
	uint32_t squareY;
	uint32_t squareX;
	uint32_t stepX;
	uint32_t stepY;
	uint32_t slopeX;
	uint32_t slopeY;
	uint32_t decision;
	uint32_t rows;

	if (p_radiusX == 0 || p_radiusY == 0) {
		VFX_line_draw(
			p_view,
			Sub(p_centerX, p_radiusX),
			Sub(p_centerY, p_radiusY),
			Add(p_centerX, p_radiusX),
			Add(p_centerY, p_radiusY),
			0,
			p_color
		);
		return;
	}

	if (ClipView(&ellipse.m_clip, p_view)) {
		return;
	}

	ellipse.m_color = (uint8_t) p_color;
	ellipse.m_centerX = Add(p_centerX, ellipse.m_clip.m_originX);
	ellipse.m_centerY = Add(p_centerY, ellipse.m_clip.m_originY);
	ellipse.m_x = 0;
	ellipse.m_y = p_radiusY;
	squareY = (uint32_t) p_radiusY * (uint32_t) p_radiusY;
	stepY = squareY << 1;
	squareX = (uint32_t) p_radiusX * (uint32_t) p_radiusX;
	stepX = squareX << 1;
	slopeX = 0;
	slopeY = stepX * (uint32_t) p_radiusY;
	decision = (squareX >> 2) + squareY - squareX * (uint32_t) p_radiusY;
	rows = (uint32_t) p_radiusY;

	// Where the arc is flatter than 45 degrees: a column per point
	while (PortableS32(slopeX - slopeY) < 0) {
		p_plot(&ellipse);
		if (PortableS32(decision) >= 0) {
			ellipse.m_y = Sub(ellipse.m_y, 1);
			rows--;
			slopeY -= stepX;
			decision -= slopeY;
		}

		ellipse.m_x = Add(ellipse.m_x, 1);
		slopeX += stepY;
		decision += slopeX + squareY;
	}

	// Where it's steeper: a row per point
	decision += (uint32_t) PortableSar32(
		PortableS32(
			(uint32_t) PortableSar32(PortableS32(squareX - squareY), 1) + (squareX - squareY) - slopeX - slopeY
		),
		1
	);
	do {
		p_plot(&ellipse);
		if (PortableS32(decision) < 0) {
			ellipse.m_x = Add(ellipse.m_x, 1);
			slopeX += stepY;
			decision += slopeX;
		}

		ellipse.m_y = Sub(ellipse.m_y, 1);
		slopeY -= stepX;
		decision -= slopeY - squareX;
	} while (PortableS32(--rows) >= 0);
}

// Draws the outline of an ellipse centered on (p_centerX, p_centerY), clipped to the view; a
// radius of 0 draws a line.
void VFX_ellipse_draw(PANE* p_pane, int32_t p_xc, int32_t p_yc, int32_t p_width, int32_t p_height, int32_t p_color)
{
	DrawEllipseWith(p_pane, p_xc, p_yc, p_width, p_height, p_color, PlotEllipsePoints);
}

void VFX_ellipse_fill(PANE* p_pane, int32_t p_xc, int32_t p_yc, int32_t p_width, int32_t p_height, int32_t p_color)
{
	DrawEllipseWith(p_pane, p_xc, p_yc, p_width, p_height, p_color, FillEllipseRows);
}

// --- Rotation ---

// The 16.16 cosine and sine of p_angle, in tenths of a degree.
void VFX_Cos_Sin(int32_t p_angle, int32_t* p_cos, int32_t* p_sin)
{
	int32_t angle = p_angle;
	int32_t base;

	while (angle < 0) {
		angle += 3600;
	}

	while (angle > 3600) {
		angle -= 3600;
	}

	if (angle <= 1800) {
		if (angle <= 900) {
			*p_cos = g_cosTable[angle];
			*p_sin = g_cosTable[900 - angle];
		}
		else {
			base = 1800 - angle;
			*p_cos = -g_cosTable[base];
			*p_sin = g_cosTable[900 - base];
		}
	}
	else {
		base = 3600 - angle;
		if (base <= 900) {
			*p_cos = g_cosTable[base];
			*p_sin = -g_cosTable[900 - base];
		}
		else {
			base = 1800 - base;
			*p_cos = -g_cosTable[base];
			*p_sin = -g_cosTable[900 - base];
		}
	}
}

// The 64-bit product plus half of 1.0, as 16.16: its middle 32 bits.
static int32_t MulRound16(int32_t p_a, int32_t p_b)
{
	return PortableS32((uint32_t) ((uint64_t) ((int64_t) p_a * p_b + 0x8000) >> 16));
}

// Its top 32 bits.
static int32_t MulRound32(int32_t p_a, int32_t p_b)
{
	return PortableS32((uint32_t) ((uint64_t) ((int64_t) p_a * p_b + 0x8000) >> 32));
}

void VFX_fixed_mul(int32_t p_m1, int32_t p_m2, int32_t* p_result)
{
	*p_result = MulRound16(p_m1, p_m2);
}

// Rotates p_point about p_origin by p_angle (tenths of a degree), its offsets scaled by p_scaleX
// and p_scaleY (16.16), into p_result. Points are x, y.
void VFX_point_transform(
	int32_t* p_in,
	int32_t* p_out,
	int32_t* p_origin,
	int32_t p_rot,
	int32_t p_xScale,
	int32_t p_yScale
)
{
	int32_t cosine;
	int32_t sine;
	int32_t x;
	int32_t y;

	VFX_Cos_Sin(p_rot, &cosine, &sine);
	x = MulRound32(PortableS32(((uint32_t) p_in[0] - (uint32_t) p_origin[0]) << 16), p_xScale);
	y = MulRound32(PortableS32(((uint32_t) p_in[1] - (uint32_t) p_origin[1]) << 16), p_yScale);
	p_out[0] = Add(Sub(MulRound16(x, cosine), MulRound16(y, sine)), p_origin[0]);
	p_out[1] = Add(Add(MulRound16(y, cosine), MulRound16(x, sine)), p_origin[1]);
}

// --- Fonts ---

// A font: its height at +8, then at +0x10 the offsets of its characters, each a width and the
// width by height pixels.
int32_t VFX_font_height(void* p_font)
{
	return ReadS32((const uint8_t*) p_font + 8);
}

static const uint8_t* FontChar(const void* p_font, int32_t p_char)
{
	const uint8_t* font = (const uint8_t*) p_font;

	return font + ReadU32(font + 0x10 + (uint32_t) p_char * 4);
}

int32_t VFX_character_width(void* p_font, int32_t p_character)
{
	return ReadS32(FontChar(p_font, p_character));
}

// Whether a + b, in the assembly's flags, is no more than 0 (`add; jle`): it compares the exact
// sum.
static int32_t SumNotPositive(int32_t p_a, int32_t p_b)
{
	return (int64_t) p_a + p_b <= 0;
}

// Draws a character, clipped to the view: through p_palette when not NULL, which maps a pixel
// to 0xff to leave it out, else copied. Returns its width.
int32_t VFX_character_draw(
	PANE* p_pane,
	int32_t p_x,
	int32_t p_y,
	void* p_font,
	int32_t p_character,
	void* p_colorTranslate
)
{
	ClippedView clip;
	int32_t result = ClipView(&clip, p_pane);
	const uint8_t* glyph;
	const uint8_t* palette = (const uint8_t*) p_colorTranslate;
	int32_t width;
	int32_t height;
	int32_t columns;
	int32_t cut;
	uint32_t to;
	int32_t i;

	if (result) {
		return result;
	}

	p_x = Add(p_x, clip.m_originX);
	p_y = Add(p_y, clip.m_originY);
	height = ReadS32((const uint8_t*) p_font + 8);
	glyph = FontChar(p_font, p_character);
	width = ReadS32(glyph);
	if (width == 0) {
		return 0;
	}

	glyph += 4;
	columns = width;
	cut = Sub(Sub(Add(clip.m_right, 1), columns), p_x);
	if (cut < 0) {
		if (SumNotPositive(columns, cut)) {
			return width;
		}

		columns = Add(columns, cut);
	}

	cut = Sub(p_x, clip.m_left);
	if (cut < 0) {
		if (SumNotPositive(columns, cut)) {
			return width;
		}

		columns = Add(columns, cut);
		glyph -= cut;
		p_x = Sub(p_x, cut);
	}

	cut = Sub(Sub(Add(clip.m_bottom, 1), height), p_y);
	if (cut < 0) {
		if (SumNotPositive(height, cut)) {
			return width;
		}

		height = Add(height, cut);
	}

	cut = Sub(p_y, clip.m_top);
	if (cut < 0) {
		if (SumNotPositive(height, cut)) {
			return width;
		}

		height = Add(height, cut);
		p_y = Sub(p_y, cut);
		glyph -= PortableS32((uint32_t) cut * (uint32_t) width);
	}

	to = PixelOffset(&clip, p_x, p_y);
	do {
		if (!palette) {
			memcpy(clip.m_pixels + to, glyph, (uint32_t) columns);
		}
		else {
			for (i = 0; i < columns; i++) {
				uint8_t pixel = palette[glyph[i]];

				if (pixel != 0xff) {
					clip.m_pixels[to + (uint32_t) i] = pixel;
				}
			}
		}

		glyph += width;
		to += clip.m_pitch;
	} while (--height);

	return width;
}

// Draws p_text, a character at a time, through VFX_character_draw. It draws the first character even when
// it ends the string.
void VFX_string_draw(PANE* p_pane, int32_t p_x, int32_t p_y, void* p_font, char* p_string, void* p_colorTranslate)
{
	const uint8_t* text = (const uint8_t*) p_string;

	do {
		p_x = Add(p_x, VFX_character_draw(p_pane, p_x, p_y, p_font, *text, p_colorTranslate));
		text++;
	} while (*text);
}

// --- Pictures ---

// Copies p_width pixels to row p_row of the view, clipped. Returns -1 or -2 for an empty buffer
// or view, else 0 (the assembly leaves what it last computed).
int32_t VFX_line_to_pane(PANE* p_target, int32_t p_y, uint8_t* p_lineBuffer, int32_t p_lineLength)
{
	ClippedView clip;
	int32_t result = ClipView(&clip, p_target);
	int32_t x;
	int32_t cut;

	if (result) {
		return result;
	}

	x = clip.m_originX;
	p_y = Add(p_y, clip.m_originY);
	cut = Sub(Add(Sub(clip.m_right, x), 1), p_lineLength);
	if (cut < 0) {
		if (SumNotPositive(p_lineLength, cut)) {
			return 0;
		}

		p_lineLength = Add(p_lineLength, cut);
	}

	cut = Sub(x, clip.m_left);
	if (cut < 0) {
		if (SumNotPositive(p_lineLength, cut)) {
			return 0;
		}

		p_lineLength = Add(p_lineLength, cut);
		p_lineBuffer -= cut;
		x = Sub(x, cut);
	}

	if (Sub(clip.m_bottom, p_y) < 0 || Sub(p_y, clip.m_top) < 0) {
		return 0;
	}

	PORTABLE_ASSERT(p_lineLength >= 0);
	memcpy(clip.m_pixels + PixelOffset(&clip, x, p_y), p_lineBuffer, (uint32_t) p_lineLength);
	return 0;
}

// The data of the IFF chunk with the tag p_tag: its chunks start at +0xc, and may be padded with
// zero bytes. The chunk must be there.
uint8_t* find_ILBM_property(char* p_tag, uint8_t* p_iff)
{
	uint8_t* chunk = p_iff + 0xc;

	for (;;) {
		while (*chunk == 0) {
			chunk++;
		}

		if (!memcmp(chunk, p_tag, 4)) {
			return chunk + 8;
		}

		// The length's low word
		chunk += 8 + ReadBigU16(chunk + 6);
	}
}

#define LINE_BUFFER 0x300
#define RUN_MAX 0x80

// Decodes an IFF (ILBM, planar, or PBM) image into the view, through VFX_line_to_pane, its rows
// optionally ByteRun1-compressed. Returns its transparent color's low byte. The images are at
// most 640 pixels wide, and ILBM ones a multiple of 16 (the assembly reads eight planes, which
// are wider for other widths than the row it decodes). With a masking of 1 it draws nothing,
// and returns a local it never set (here, 0).
int32_t VFX_ILBM_draw(PANE* p_pane, uint8_t* p_ilbmBuffer)
{
	uint8_t decoded[LINE_BUFFER];
	uint8_t line[LINE_BUFFER];
	int32_t viewWidth = Add(Sub(p_pane->m_x1, p_pane->m_x0), 1);
	int32_t viewHeight = Add(Sub(p_pane->m_y1, p_pane->m_y0), 1);
	int32_t planar = ReadU32(p_ilbmBuffer + 8) == 0x4d424c49;
	const uint8_t* header = find_ILBM_property(g_bmhdProp, p_ilbmBuffer);
	const uint8_t* body;
	int32_t width = (int32_t) ReadBigU16(header);
	int32_t rows = (int32_t) ReadBigU16(header + 2);
	int32_t compression;
	int32_t transparent;
	uint32_t planeBytes;
	uint32_t rowBytes;
	int32_t columns;
	int32_t row = 0;

	rows = rows < viewHeight ? rows : viewHeight;
	if (header[9] == 1) {
		return 0;
	}

	compression = header[10];
	transparent = header[13];
	planeBytes = ((uint32_t) width >> 3) + ((width & 7) != 0);
	planeBytes += planeBytes & 1;
	rowBytes = (uint32_t) width + (width & 1);
	columns = width < viewWidth ? width : viewWidth;
	body = find_ILBM_property(g_bodyProp, p_ilbmBuffer);
	PORTABLE_ASSERT(rows > 0);
	PORTABLE_ASSERT(rowBytes + RUN_MAX <= LINE_BUFFER);
	do {
		const uint8_t* data;

		if (compression == 1) {
			uint32_t count = 0;

			while (count < rowBytes) {
				uint32_t code = *body++;

				if (code < 0x80) {
					memcpy(decoded + count, body, code + 1);
					body += code + 1;
					count += code + 1;
				}
				else if (code > 0x80) {
					uint32_t repeat = (0x101 - code) & 0xff;

					memset(decoded + count, *body++, repeat);
					count += repeat;
				}
			}

			data = decoded;
		}
		else {
			data = body;
			body += rowBytes;
		}

		if (planar) {
			// Eight planes, a bit of each per pixel, from the high bit of each byte
			int32_t pixels = columns;
			uint32_t bytes = planeBytes;
			uint32_t out = 0;

			for (;;) {
				uint32_t mask = 0x80;

				for (;;) {
					uint32_t value = 0;
					uint32_t plane;

					for (plane = 0; plane < 8; plane++) {
						if (data[plane * planeBytes] & mask) {
							value |= 1 << plane;
						}
					}

					line[out++] = (uint8_t) value;
					if (--pixels == 0) {
						break;
					}

					mask >>= 1;
					if (!mask) {
						break;
					}
				}

				if (pixels == 0) {
					break;
				}

				data++;
				if (--bytes == 0) {
					break;
				}
			}

			VFX_line_to_pane(p_pane, row, line, columns);
		}
		else {
			VFX_line_to_pane(p_pane, row, (uint8_t*) data, columns);
		}

		row++;
	} while (--rows);

	return transparent;
}

void VFX_ILBM_palette(uint8_t* p_ilbmBuffer, uint8_t* p_palette)
{
	const uint8_t* colors = find_ILBM_property(g_cmapProp, p_ilbmBuffer);
	int32_t i;

	for (i = 0; i < 0x300; i++) {
		p_palette[i] = colors[i] >> 2;
	}
}

// The image's width << 16 | height.
int32_t VFX_ILBM_resolution(uint8_t* p_ilbmBuffer)
{
	const uint8_t* header = find_ILBM_property(g_bmhdProp, p_ilbmBuffer);

	return PortableS32(ReadBigU16(header) << 16 | ReadBigU16(header + 2));
}

// Decodes a PCX image (one plane, run-length coded) into the view, through VFX_line_to_pane. A run
// may end past the row, which the next row then overwrites.
int32_t VFX_PCX_draw(PANE* p_pane, uint8_t* p_pcxBuffer)
{
	uint8_t line[LINE_BUFFER + 0x40];
	const uint8_t* data = p_pcxBuffer + 0x80;
	int32_t rows = (int32_t) ((ReadU16(p_pcxBuffer + 0xa) - ReadU16(p_pcxBuffer + 6)) & 0xffff);
	int32_t bytes = (int32_t) ReadU16(p_pcxBuffer + 0x42);
	int32_t row = 0;

	PORTABLE_ASSERT(bytes <= LINE_BUFFER);
	do {
		int32_t count = 0;

		do {
			uint32_t code = *data++;

			if ((code & 0xc0) == 0xc0) {
				memset(line + count, *data++, code & 0x3f);
				count += (int32_t) (code & 0x3f);
			}
			else {
				line[count++] = (uint8_t) code;
			}
		} while (count < bytes);

		VFX_line_to_pane(p_pane, row, line, bytes);
	} while (++row <= rows);

	return 0;
}

// The PCX file's palette, its last 0x300 bytes, with 6-bit components.
void VFX_PCX_palette(uint8_t* p_pcxBuffer, int32_t p_pcxFileSize, void* p_palette)
{
	const uint8_t* colors = p_pcxBuffer + p_pcxFileSize - 0x300;
	uint8_t* palette = (uint8_t*) p_palette;
	int32_t i;

	for (i = 0; i < 0x300; i++) {
		palette[i] = colors[i] >> 2;
	}
}

// --- GIF ---

// VFX_GIF_draw's state, in the caller's p_state block: its fields, then the decoded string's stack and
// the code table (each code's first and last character, and its prefix).
enum GifField {
	c_gifNext = 0x00,   // the next free code
	c_gifLimit = 0x04,  // the first code the code size can't hold
	c_gifColumn = 0x08, // in the row being decoded
	c_gifRow = 0x0c,
	c_gifBlock = 0x10, // bytes left in the data sub-block
	c_gifBits = 0x14,  // the code stream's buffered bits
	c_gifBitCount = 0x18,
	c_gifCodeSize = 0x1c,
	c_gifLeft = 0x20, // pixels left in the row
	c_gifWidth = 0x24,
	c_gifHeight = 0x28,
	c_gifInterlaced = 0x2c, // a byte
	c_gifPass = 0x2d,       // a byte
	c_gifStack = 0x2e,
	c_gifFirst = 0x102e,
	c_gifLast = 0x202e,
	c_gifPrefix = 0x302e // words
};

#define GIF_CODES 0x1000
#define GIF_NO_PREFIX 0xffff
#define GIF_UNUSED 0xfffe

typedef struct GifDecoder {
	uint8_t* m_state;
	const uint8_t* m_data;
	PANE* m_view;
	uint8_t m_line[LINE_BUFFER];
} GifDecoder;

static uint32_t GifField(const GifDecoder* p_gif, uint32_t p_field)
{
	return ReadU32(p_gif->m_state + p_field);
}

static void SetGifField(GifDecoder* p_gif, uint32_t p_field, uint32_t p_value)
{
	WriteU32(p_gif->m_state + p_field, p_value);
}

static uint32_t GifPrefix(const GifDecoder* p_gif, uint32_t p_code)
{
	return ReadU16(p_gif->m_state + c_gifPrefix + p_code * 2);
}

static void SetGifPrefix(GifDecoder* p_gif, uint32_t p_code, uint32_t p_prefix)
{
	uint8_t* entry = p_gif->m_state + c_gifPrefix + p_code * 2;

	entry[0] = (uint8_t) p_prefix;
	entry[1] = (uint8_t) (p_prefix >> 8);
}

// The next byte of the image data's sub-blocks. A sub-block's length of 0 isn't the end here:
// the count wraps.
static uint32_t GIF_getb(GifDecoder* p_gif)
{
	if (GifField(p_gif, c_gifBlock) == 0) {
		SetGifField(p_gif, c_gifBlock, *p_gif->m_data++);
	}

	SetGifField(p_gif, c_gifBlock, GifField(p_gif, c_gifBlock) - 1);
	return *p_gif->m_data++;
}

// The next p_bits (1 to 8) bits of the code stream.
static uint32_t GIF_getbcode(GifDecoder* p_gif, uint32_t p_bits)
{
	uint32_t bits;
	uint32_t count;

	if (GifField(p_gif, c_gifBitCount) == 0) {
		SetGifField(p_gif, c_gifBits, GIF_getb(p_gif));
		SetGifField(p_gif, c_gifBitCount, 8);
	}

	count = GifField(p_gif, c_gifBitCount);
	if (PortableS32(count) < (int32_t) p_bits) {
		SetGifField(p_gif, c_gifBits, GifField(p_gif, c_gifBits) | GIF_getb(p_gif) << (count & 31));
		SetGifField(p_gif, c_gifBitCount, count + 8);
	}

	bits = GifField(p_gif, c_gifBits);
	SetGifField(p_gif, c_gifBitCount, GifField(p_gif, c_gifBitCount) - p_bits);
	SetGifField(p_gif, c_gifBits, bits >> p_bits);
	return bits & g_gifCmask[p_bits];
}

static void GIF_init_codetable(GifDecoder* p_gif, uint32_t p_roots)
{
	uint32_t code;

	SetGifField(p_gif, c_gifNext, p_roots + 2);
	SetGifField(p_gif, c_gifLimit, p_roots * 2);
	for (code = 0; code < p_roots; code++) {
		p_gif->m_state[c_gifFirst + code] = (uint8_t) code;
		p_gif->m_state[c_gifLast + code] = (uint8_t) code;
		SetGifPrefix(p_gif, code, GIF_NO_PREFIX);
	}

	for (; code < GIF_CODES; code++) {
		SetGifPrefix(p_gif, code, GIF_UNUSED);
	}
}

// Adds the code p_prefix followed by p_code's first character, and grows the code size when the
// table reaches its limit (up to 12 bits).
static void GIF_insertcode(GifDecoder* p_gif, uint32_t p_code, uint32_t p_prefix)
{
	uint32_t next = GifField(p_gif, c_gifNext);

	PORTABLE_ASSERT(next < GIF_CODES && p_code < GIF_CODES && p_prefix < GIF_CODES);
	SetGifPrefix(p_gif, next, p_prefix);
	p_gif->m_state[c_gifLast + next] = p_gif->m_state[c_gifFirst + p_code];
	p_gif->m_state[c_gifFirst + next] = p_gif->m_state[c_gifFirst + p_prefix];
	next++;
	SetGifField(p_gif, c_gifNext, next);
	if (next == GifField(p_gif, c_gifLimit) && PortableS32(GifField(p_gif, c_gifCodeSize)) < 12) {
		SetGifField(p_gif, c_gifCodeSize, GifField(p_gif, c_gifCodeSize) + 1);
		SetGifField(p_gif, c_gifLimit, GifField(p_gif, c_gifLimit) << 1);
	}
}

// Adds a pixel to the row, and draws the row when it's complete.
static void GIF_dopixel(GifDecoder* p_gif, uint8_t p_pixel)
{
	uint32_t column = GifField(p_gif, c_gifColumn);
	uint32_t row;

	PORTABLE_ASSERT(column < LINE_BUFFER);
	p_gif->m_line[column] = p_pixel;
	SetGifField(p_gif, c_gifColumn, column + 1);
	SetGifField(p_gif, c_gifLeft, GifField(p_gif, c_gifLeft) - 1);
	if (GifField(p_gif, c_gifLeft) != 0) {
		return;
	}

	VFX_line_to_pane(
		p_gif->m_view,
		PortableS32(GifField(p_gif, c_gifRow)),
		p_gif->m_line,
		PortableS32(GifField(p_gif, c_gifWidth))
	);
	SetGifField(p_gif, c_gifColumn, 0);
	SetGifField(p_gif, c_gifLeft, GifField(p_gif, c_gifWidth));
	if (p_gif->m_state[c_gifInterlaced]) {
		// After the fourth pass, the rows repeat the first one
		row = GifField(p_gif, c_gifRow) + g_gifIncTable[p_gif->m_state[c_gifPass]];
		SetGifField(p_gif, c_gifRow, row);
		if (PortableS32(row) >= PortableS32(GifField(p_gif, c_gifHeight))) {
			p_gif->m_state[c_gifPass]++;
			PORTABLE_ASSERT(p_gif->m_state[c_gifPass] < sizeof(g_gifStartTable));
			SetGifField(p_gif, c_gifRow, g_gifStartTable[p_gif->m_state[c_gifPass]]);
		}
	}
	else {
		row = GifField(p_gif, c_gifRow) + 1;
		SetGifField(p_gif, c_gifRow, row);
		if (PortableS32(row) >= PortableS32(GifField(p_gif, c_gifHeight))) {
			SetGifField(p_gif, c_gifRow, 0);
		}
	}
}

// A GIF's palette sizes from a flags byte: 3 << ((flags & 7) + 1) bytes.
static uint32_t GifPaletteSize(uint32_t p_flags)
{
	return 3u << ((p_flags & 7) + 1);
}

// The image descriptor, after the header and the global palette.
static const uint8_t* GifImage(const uint8_t* p_gif)
{
	const uint8_t* image = p_gif + 0xd;

	if (p_gif[0xa] & 0x80) {
		image += GifPaletteSize(p_gif[0xa]);
	}

	return image;
}

// Decodes a GIF's first image into the view, with p_state (0x502e bytes) as the decoder's state,
// rows of at most 0x300 pixels. Returns the background color index.
int32_t VFX_GIF_draw(PANE* p_pane, uint8_t* p_gifBuffer, uint8_t* p_gifScratch)
{
	GifDecoder gif;
	const uint8_t* image = GifImage(p_gifBuffer);
	uint32_t minimum;
	uint32_t clear;
	uint32_t previous = GIF_NO_PREFIX;

	gif.m_state = p_gifScratch;
	gif.m_view = p_pane;
	memset(p_gifScratch, 0, c_gifStack);
	SetGifField(&gif, c_gifWidth, ReadU16(image + 5));
	SetGifField(&gif, c_gifHeight, ReadU16(image + 7));
	p_gifScratch[c_gifInterlaced] = image[9] & 0x40;
	gif.m_data = image + 0xa;
	if (image[9] & 0x80) {
		gif.m_data += GifPaletteSize(image[9]);
	}

	SetGifField(&gif, c_gifBlock, 0);
	minimum = *gif.m_data++;
	clear = (uint32_t) 1 << (minimum & 31);
	SetGifField(&gif, c_gifCodeSize, minimum + 1);
	GIF_init_codetable(&gif, clear);
	p_gifScratch[c_gifPass] = 0;
	SetGifField(&gif, c_gifLeft, GifField(&gif, c_gifWidth));
	SetGifField(&gif, c_gifColumn, 0);
	SetGifField(&gif, c_gifRow, 0);
	for (;;) {
		uint32_t size = GifField(&gif, c_gifCodeSize);
		uint32_t code;

		if (PortableS32(size) <= 8) {
			code = GIF_getbcode(&gif, size);
		}
		else {
			code = GIF_getbcode(&gif, 8);
			code |= GIF_getbcode(&gif, size - 8) << 8;
		}

		if (code == clear) {
			GIF_init_codetable(&gif, clear);
			SetGifField(&gif, c_gifCodeSize, minimum + 1);
			previous = GIF_NO_PREFIX;
		}
		else if (code == clear + 1) {
			// The end: skip the rest of the sub-blocks
			do {
				gif.m_data += GifField(&gif, c_gifBlock);
				SetGifField(&gif, c_gifBlock, *gif.m_data++);
			} while (GifField(&gif, c_gifBlock));
			break;
		}
		else {
			uint32_t count = 0;
			uint32_t chain = code;

			PORTABLE_ASSERT(code < GIF_CODES);
			if (GifPrefix(&gif, code) == GIF_UNUSED) {
				GIF_insertcode(&gif, previous, previous);
			}
			else if (previous != GIF_NO_PREFIX) {
				GIF_insertcode(&gif, code, previous);
			}

			do {
				p_gifScratch[c_gifStack + count++] = p_gifScratch[c_gifLast + chain];
				chain = GifPrefix(&gif, chain);
			} while (chain != GIF_NO_PREFIX);

			while (count) {
				GIF_dopixel(&gif, p_gifScratch[c_gifStack + --count]);
			}

			previous = code;
		}
	}

	return p_gifBuffer[0xb];
}

// The GIF's global palette, then its first image's local one, with 6-bit components.
void VFX_GIF_palette(uint8_t* p_gifBuffer, uint8_t* p_palette)
{
	const uint8_t* image = p_gifBuffer + 0xd;
	uint32_t size;
	uint32_t i;

	if (p_gifBuffer[0xa] & 0x80) {
		size = GifPaletteSize(p_gifBuffer[0xa]);
		for (i = 0; i < size; i++) {
			p_palette[i] = *image++ >> 2;
		}
	}

	if (image[9] & 0x80) {
		size = GifPaletteSize(image[9]);
		for (i = 0; i < size; i++) {
			p_palette[i] = image[0xa + i] >> 2;
		}
	}
}

// The first image's width << 16 | height.
int32_t VFX_GIF_resolution(void* p_gifBuffer)
{
	const uint8_t* image = GifImage((const uint8_t*) p_gifBuffer);

	return PortableS32(ReadU16(image + 5) << 16 | ReadU16(image + 7));
}

// --- VFX_shape_transform ---

// The assembly maps the rotated frame with the texture mapper of VFX3D.ASM's
// VFX_map_polygon, repeated: these helpers are vfx3d.c's.

// (p_delta << 16) / (p_count << 16): a 64-bit dividend, and a 32-bit divisor.
static uint32_t Slope(uint32_t p_delta, uint32_t p_count)
{
	return (uint32_t) PortableIdiv((int64_t) PortableS32(p_delta) * 0x10000, PortableS32(p_count << 16));
}

// p_count steps of p_step: `imul` by the count in 16.16, then `shrd 16`.
static uint32_t Advance(uint32_t p_step, uint32_t p_count)
{
	int64_t product = (int64_t) PortableS32(p_step) * PortableS32(p_count << 16);

	return (uint32_t) ((uint64_t) product >> 16);
}

// A texture step's whole texels, rounded towards zero (u's sign-extended, v's a word).
static uint32_t WholeTexels(uint32_t p_step, int32_t p_extend)
{
	uint32_t whole = (p_step >> 16) & 0xffff;

	if (PortableS32(p_step) < 0) {
		if (p_extend) {
			whole |= 0xffff0000;
		}

		if (p_step & 0xffff) {
			whole++;
		}
	}

	return whole;
}

// A coordinate's fraction and its step, in the top half of a word: the complement's, stepping
// up, for a negative step.
static void StartFraction(uint32_t p_value, uint32_t p_step, uint32_t* p_fraction, uint32_t* p_fractionStep)
{
	if (PortableS32(p_step) < 0) {
		p_step = 0 - p_step;
		p_value = ~p_value;
	}

	*p_fraction = p_value << 16;
	*p_fractionStep = p_step << 16;
}

static uint32_t AddCarry(uint32_t* p_value, uint32_t p_step)
{
	uint32_t value = *p_value;

	*p_value = value + p_step;
	return *p_value < value;
}

// A corner of the rotated frame: where it lands, and the texel it maps (whole texels).
typedef struct RotatedCorner {
	int32_t m_x;
	int32_t m_y;
	int32_t m_u;
	int32_t m_v;
} RotatedCorner;

typedef struct RotatedEdge {
	const RotatedCorner* m_start;
	const RotatedCorner* m_end;
	uint32_t m_count;
	uint32_t m_x;
	uint32_t m_xStep;
	uint32_t m_u;
	uint32_t m_uStep;
	uint32_t m_v;
	uint32_t m_vStep;
} RotatedEdge;

typedef struct RotatedQuad {
	RotatedCorner m_corners[4];
	ClippedView m_clip;
	const uint8_t* m_texels;
	uint32_t m_texturePitch;
	RotatedEdge m_left; // walks the corners backwards
	RotatedEdge m_right;
	uint32_t m_lines;
	uint32_t m_row;
} RotatedQuad;

static void AdvanceRotatedEdge(const RotatedQuad* p_quad, RotatedEdge* p_edge)
{
	int32_t index = (int32_t) (p_edge->m_end - p_quad->m_corners);

	p_edge->m_start = p_edge->m_end;
	p_edge->m_end = &p_quad->m_corners[(index + (p_edge == &p_quad->m_left ? 3 : 1)) % 4];
}

static void StartRotatedEdge(RotatedEdge* p_edge, uint32_t p_count)
{
	const RotatedCorner* start = p_edge->m_start;
	const RotatedCorner* end = p_edge->m_end;

	p_edge->m_count = p_count;
	p_edge->m_xStep = Slope(((uint32_t) end->m_x - (uint32_t) start->m_x) << 16, p_count);
	p_edge->m_uStep = Slope(((uint32_t) end->m_u - (uint32_t) start->m_u) << 16, p_count);
	p_edge->m_vStep = Slope(((uint32_t) end->m_v - (uint32_t) start->m_v) << 16, p_count);
	p_edge->m_x = ((uint32_t) start->m_x << 16) + 0x8000;
	p_edge->m_u = ((uint32_t) start->m_u << 16) + 0x8000;
	p_edge->m_v = ((uint32_t) start->m_v << 16) + 0x8000;
}

static void StepRotatedEdge(RotatedEdge* p_edge)
{
	p_edge->m_x += p_edge->m_xStep;
	p_edge->m_u += p_edge->m_uStep;
	p_edge->m_v += p_edge->m_vStep;
}

// Maps the scratch view onto the quadrilateral, clipped to the view, leaving out its 0xff texels.
static void FillRotatedQuad(RotatedQuad* p_quad)
{
	const ClippedView* clip = &p_quad->m_clip;
	const RotatedCorner* top = NULL;
	RotatedEdge* edges[2];
	int32_t minY = 0x7fff;
	int32_t maxY = -0x8000;
	uint32_t inside = 0xf;
	int32_t i;

	for (i = 0; i < 4; i++) {
		const RotatedCorner* corner = &p_quad->m_corners[i];

		inside &= OutCode(clip, corner->m_x, corner->m_y);
		if (corner->m_y <= minY) {
			minY = corner->m_y;
			top = corner;
		}

		if (corner->m_y >= maxY) {
			maxY = corner->m_y;
		}
	}

	if (inside) {
		return;
	}

	PORTABLE_ASSERT(top != NULL);
	if (maxY == minY) {
		return;
	}

	edges[0] = &p_quad->m_left;
	edges[1] = &p_quad->m_right;
	for (i = 0; i < 2; i++) {
		RotatedEdge* edge = edges[i];

		edge->m_end = top;
		do {
			AdvanceRotatedEdge(p_quad, edge);
		} while ((edge->m_start->m_y < clip->m_top && edge->m_end->m_y <= clip->m_top) ||
				 edge->m_end->m_y == edge->m_start->m_y);
		StartRotatedEdge(edge, (uint32_t) edge->m_end->m_y - (uint32_t) edge->m_start->m_y);
	}

	if (maxY > clip->m_bottom) {
		p_quad->m_lines = (uint32_t) clip->m_bottom - (uint32_t) minY;
	}
	else {
		p_quad->m_lines = (uint32_t) maxY - (uint32_t) minY;
	}

	if (clip->m_top > minY) {
		p_quad->m_lines -= (uint32_t) clip->m_top - (uint32_t) minY;
		minY = clip->m_top;
		for (i = 0; i < 2; i++) {
			RotatedEdge* edge = edges[i];
			uint32_t lines = (uint32_t) clip->m_top - (uint32_t) edge->m_start->m_y;

			edge->m_count -= lines;
			edge->m_x += Advance(edge->m_xStep, lines);
			edge->m_u += Advance(edge->m_uStep, lines);
			edge->m_v += Advance(edge->m_vStep, lines);
		}
	}

	p_quad->m_row = (uint32_t) minY * clip->m_pitch;
	for (;;) {
		const RotatedEdge* low = &p_quad->m_left;
		const RotatedEdge* high = &p_quad->m_right;
		int32_t left;
		int32_t right;

		if (!(PortableS32(high->m_x) > PortableS32(low->m_x))) {
			low = &p_quad->m_right;
			high = &p_quad->m_left;
		}

		left = PortableSar32(PortableS32(low->m_x), 16);
		right = PortableSar32(PortableS32(high->m_x), 16);
		if (left <= clip->m_right && right >= clip->m_left) {
			uint32_t u = low->m_u;
			uint32_t v = low->m_v;
			uint32_t uStep = 0;
			uint32_t vStep = 0;
			uint32_t offsets[4] = {0, 0, 0, 0};
			uint32_t uFraction;
			uint32_t uFractionStep;
			uint32_t vFraction;
			uint32_t vFractionStep;
			uint32_t texel;
			uint8_t* pixels;
			int32_t count;

			if (right != left) {
				uint32_t width = (uint32_t) right - (uint32_t) left;
				uint32_t uWhole;
				uint32_t vOffset;
				uint32_t vCarry;

				uStep = Slope(high->m_u - u, width);
				uWhole = WholeTexels(uStep, 1);
				vStep = Slope(high->m_v - v, width);
				vOffset = (uint32_t) PortableS16((uint16_t) (p_quad->m_texturePitch * WholeTexels(vStep, 0)));
				vCarry = PortableS32(vStep) < 0 ? 0 - p_quad->m_texturePitch : p_quad->m_texturePitch;
				offsets[0] = uWhole + vOffset;
				offsets[1] = offsets[0] + vCarry;
				offsets[2] = uWhole + (PortableS32(uStep) < 0 ? 0xffffffff : 1) + vOffset;
				offsets[3] = offsets[2] + vCarry;
				if (clip->m_left > left) {
					uint32_t skipped = (uint32_t) clip->m_left - (uint32_t) left;

					left = clip->m_left;
					u += Advance(uStep, skipped);
					v += Advance(vStep, skipped);
				}

				if (right > clip->m_right) {
					right = clip->m_right;
				}
			}

			texel = (v >> 16) * p_quad->m_texturePitch + (u >> 16);
			StartFraction(u, uStep, &uFraction, &uFractionStep);
			StartFraction(v, vStep, &vFraction, &vFractionStep);
			pixels = clip->m_pixels + (uint32_t) (p_quad->m_row + (uint32_t) left);
			count = right - left + 1;
			for (i = 0; i < count; i++) {
				uint8_t value = p_quad->m_texels[texel];

				if (value != 0xff) {
					pixels[i] = value;
				}

				if (i + 1 < count) {
					uint32_t carry = AddCarry(&uFraction, uFractionStep) << 1;

					carry |= AddCarry(&vFraction, vFractionStep);
					texel += offsets[carry];
				}
			}
		}

		p_quad->m_row += clip->m_pitch;
		p_quad->m_lines--;
		if (PortableS32(p_quad->m_lines) < 0) {
			return;
		}

		if (p_quad->m_lines == 0) {
			StepRotatedEdge(&p_quad->m_left);
			StepRotatedEdge(&p_quad->m_right);
			continue;
		}

		for (i = 0; i < 2; i++) {
			RotatedEdge* edge = edges[i];

			if (--edge->m_count == 0) {
				uint32_t lines;

				AdvanceRotatedEdge(p_quad, edge);
				lines = (uint32_t) edge->m_end->m_y - (uint32_t) edge->m_start->m_y;
				StartRotatedEdge(edge, lines ? lines : 1);
			}
			else {
				StepRotatedEdge(edge);
			}
		}
	}
}

// Draws frame p_frame at (p_x, p_y), rotated by p_angle (tenths of a degree) and scaled by
// p_scaleX and p_scaleY (16.16) about its origin. The frame is first drawn into p_scratch, a
// buffer of its size (cleared to 0xff, and through lookaside when p_flags has bit 0; bit 1
// keeps what's there instead), then mapped onto the view, leaving out 0xff. Without rotation or
// scaling it's VFX_shape_draw (or VFX_shape_translate_draw), whose result it returns; the mapping
// returns -1 or -2 for an empty buffer or view, else 0 (the assembly leaves what it last
// computed).
int32_t VFX_shape_transform(
	PANE* p_pane,
	void* p_shapeTable,
	int32_t p_shapeNumber,
	int32_t p_hotX,
	int32_t p_hotY,
	uint8_t* p_buffer,
	int32_t p_rot,
	int32_t p_xScale,
	int32_t p_yScale,
	uint32_t p_flags
)
{
	const uint8_t* remap = p_flags & 1 ? lookaside : NULL;
	WINDOW scratch;
	PANE scratchView;
	RotatedQuad quad;
	uint32_t extent;
	uint32_t origin;
	int32_t right;
	int32_t bottom;
	int32_t pivot[2];
	int32_t offsetX;
	int32_t offsetY;
	int32_t result;
	int32_t i;

	if (p_xScale == 0x10000 && p_yScale == 0x10000 && p_rot == 0) {
		return ShapeDrawWith(p_pane, p_shapeTable, p_shapeNumber, p_hotX, p_hotY, remap);
	}

	extent = (uint32_t) VFX_shape_resolution(p_shapeTable, p_shapeNumber);
	right = (int32_t) (extent >> 16) - 1;
	bottom = (int32_t) (extent & 0xffff) - 1;
	scratch.m_buffer = p_buffer;
	scratch.m_xMax = right;
	scratch.m_yMax = bottom;
	scratch.m_stencil = NULL;
	scratch.m_shadow = 0;
	scratchView.m_window = &scratch;
	scratchView.m_x0 = 0;
	scratchView.m_y0 = 0;
	scratchView.m_x1 = right;
	scratchView.m_y1 = bottom;

	// The frame drawn with its top left at the scratch view's
	origin = (uint32_t) VFX_shape_minxy(p_shapeTable, p_shapeNumber);
	pivot[0] = PortableS32(0 - (uint32_t) PortableSar32(PortableS32(origin), 16));
	pivot[1] = PortableS32(0 - (uint32_t) (int32_t) PortableS16((uint16_t) origin));
	if (!(p_flags & 2)) {
		PaneWipeWith(&scratchView, 0xff);
		ShapeDrawWith(&scratchView, p_shapeTable, p_shapeNumber, pivot[0], pivot[1], remap);
	}

	result = ClipView(&quad.m_clip, p_pane);
	if (result) {
		return result;
	}

	offsetX = Sub(p_hotX, pivot[0]);
	offsetY = Sub(p_hotY, pivot[1]);
	for (i = 0; i < 4; i++) {
		int32_t point[2];
		int32_t rotated[2];

		point[0] = i == 1 || i == 2 ? right : 0;
		point[1] = i >= 2 ? bottom : 0;
		VFX_point_transform(point, rotated, pivot, p_rot, p_xScale, p_yScale);
		quad.m_corners[i].m_x = Add(Add(rotated[0], offsetX), quad.m_clip.m_originX);
		quad.m_corners[i].m_y = Add(Add(rotated[1], offsetY), quad.m_clip.m_originY);
		quad.m_corners[i].m_u = point[0];
		quad.m_corners[i].m_v = point[1];
	}

	quad.m_texels = p_buffer;
	quad.m_texturePitch = (uint32_t) right + 1;
	FillRotatedQuad(&quad);
	return 0;
}

// --- Run-length encoding ---

// VFX_shape_scan's encoder: the assembly keeps it in globals (SHP_building...), for its two
// helpers, ScanLine and FlushPacket.
typedef struct RleEncoder {
	uint8_t* m_output; // NULL only measures
	uint32_t m_cursor; // the bytes written so far
	const uint8_t* m_row;
	const uint8_t* m_run;      // past the end of the run to emit
	const uint8_t* m_runStart; // the pixels emitted so far end there
	uint32_t m_skip;           // the transparent pixels before the next run
	int32_t m_left;            // the opaque pixels' bounds
	int32_t m_top;
	int32_t m_right;
	int32_t m_bottom;
} RleEncoder;

// FlushPacket's operations.
enum RleOp {
	c_rleStartRow,
	c_rleLiteral,
	c_rleRepeat,
	c_rleSkip,
	c_rleEndRow,
	c_rleNone
};

static void PutRleByte(RleEncoder* p_encoder, uint32_t p_byte)
{
	if (p_encoder->m_output) {
		p_encoder->m_output[p_encoder->m_cursor] = (uint8_t) p_byte;
	}

	p_encoder->m_cursor++;
}

// Emits the run from m_runStart to p_back pixels before m_run, as p_op. A run's bounds update the
// encoder's; a run that reaches past the right one, the assembly only emits when it isn't
// empty.
static void FlushPacket(RleEncoder* p_encoder, int32_t p_op, uint32_t p_back, int32_t p_left)
{
	const uint8_t* start = p_encoder->m_runStart;
	uint32_t length;
	uint32_t chunk;
	int32_t x;

	switch (p_op) {
	case c_rleStartRow:
		p_encoder->m_skip = 0;
		start = p_encoder->m_run;
		break;
	case c_rleLiteral:
	case c_rleRepeat:
		if (p_encoder->m_skip) {
			uint32_t skip = p_encoder->m_skip;

			do {
				chunk = PortableS32(skip) < 0xff ? skip : 0xff;
				skip -= chunk;
				PutRleByte(p_encoder, 1);
				PutRleByte(p_encoder, chunk);
				start += chunk;
			} while (skip);

			p_encoder->m_skip = 0;
		}

		length = (uint32_t) (p_encoder->m_run - start) - p_back;
		x = PortableS32((uint32_t) p_left + (uint32_t) (start - p_encoder->m_row));
		if (x < p_encoder->m_left) {
			p_encoder->m_left = x;
		}

		x = PortableS32((uint32_t) x + length - 1);
		if (x > p_encoder->m_right) {
			p_encoder->m_right = x;
			if (!length) {
				break;
			}
		}

		do {
			chunk = PortableS32(length) < 0x7f ? length : 0x7f;
			if (p_op == c_rleRepeat) {
				PutRleByte(p_encoder, chunk * 2);
				PutRleByte(p_encoder, *start);
			}
			else {
				PutRleByte(p_encoder, chunk * 2 + 1);
				if (p_encoder->m_output) {
					memcpy(p_encoder->m_output + p_encoder->m_cursor, start, chunk);
				}

				p_encoder->m_cursor += chunk;
			}

			start += chunk;
			length -= chunk;
		} while (length);
		break;
	case c_rleSkip:
		p_encoder->m_skip = (uint32_t) (p_encoder->m_run - start) - p_back;
		break;
	case c_rleEndRow:
		PutRleByte(p_encoder, 0);
		break;
	}

	p_encoder->m_runStart = start;
}

// Encodes a row of p_count pixels: transparent pixels as skips, then runs of at least two equal
// pixels (three in a literal), and literals.
static void ScanLine(RleEncoder* p_encoder, uint32_t p_count, uint8_t p_transparent, int32_t p_left)
{
	const uint8_t* pixel = p_encoder->m_row;
	int32_t last = c_rleNone;
	uint8_t previous;
	uint8_t current;
	int32_t equal;

	p_encoder->m_run = pixel;
	FlushPacket(p_encoder, c_rleStartRow, 0, p_left);
	if (!p_count) {
		goto end;
	}

	previous = *pixel++;
	p_count--;
	if (previous == p_transparent) {
		goto skip;
	}

literal:
	last = c_rleLiteral;
	if (!p_count) {
		goto end;
	}

	current = *pixel++;
	p_count--;
	if (current == previous) {
		goto repeat;
	}

	previous = current;
	if (current == p_transparent) {
		p_encoder->m_run = pixel;
		FlushPacket(p_encoder, c_rleLiteral, 1, p_left);
		goto skip;
	}

	// A literal ends at a transparent pixel, or before three equal ones
	for (;;) {
		if (!p_count) {
			goto end;
		}

		current = *pixel++;
		p_count--;
		equal = current == previous;
		previous = current;
		if (current == p_transparent) {
			p_encoder->m_run = pixel;
			FlushPacket(p_encoder, c_rleLiteral, 1, p_left);
			goto skip;
		}

		if (!equal) {
			continue;
		}

		if (!p_count) {
			goto end;
		}

		current = *pixel++;
		p_count--;
		equal = current == previous;
		previous = current;
		if (current == p_transparent) {
			p_encoder->m_run = pixel;
			FlushPacket(p_encoder, c_rleLiteral, 1, p_left);
			goto skip;
		}

		if (equal) {
			p_encoder->m_run = pixel;
			FlushPacket(p_encoder, c_rleLiteral, 3, p_left);
			goto repeat;
		}
	}

repeat:
	last = c_rleRepeat;
	do {
		if (!p_count) {
			goto end;
		}

		current = *pixel++;
		p_count--;
	} while (current == previous);

	previous = current;
	p_encoder->m_run = pixel;
	FlushPacket(p_encoder, c_rleRepeat, 1, p_left);
	if (current != p_transparent) {
		goto literal;
	}

skip:
	last = c_rleSkip;
	do {
		if (!p_count) {
			goto end;
		}

		current = *pixel++;
		p_count--;
	} while (current == previous);

	previous = current;
	p_encoder->m_run = pixel;
	FlushPacket(p_encoder, c_rleSkip, 1, p_left);
	goto literal;

end:
	p_encoder->m_run = pixel;
	FlushPacket(p_encoder, last, 0, p_left);
	FlushPacket(p_encoder, c_rleEndRow, 0, p_left);
}

// Encodes the view's pixels other than p_transparent as an SHP frame at (p_x, p_y): a 0x18-byte
// header, then its rows' codes, bounded by the opaque pixels. With p_out NULL, it only measures.
// Returns the frame's size.
int32_t VFX_shape_scan(PANE* p_pane, uint8_t p_transparentColor, int32_t p_hotX, int32_t p_hotY, uint8_t* p_buffer)
{
	ClippedView clip;
	RleEncoder encoder;
	int32_t result = ClipView(&clip, p_pane);
	uint32_t width;
	uint32_t row;
	int32_t y;

	if (result) {
		return result;
	}

	encoder.m_output = p_buffer;
	if (p_buffer) {
		WriteU32(
			p_buffer,
			((uint32_t) Sub(p_pane->m_x1, p_pane->m_x0) << 16) |
				(((uint32_t) p_pane->m_y1 - (uint32_t) p_pane->m_y0) & 0xffff)
		);
		WriteU32(p_buffer + 4, ((uint32_t) p_hotX << 16) | ((uint32_t) p_hotY & 0xffff));
		WriteU32(p_buffer + 8, 0);
		WriteU32(p_buffer + 0xc, 0);
		WriteU32(p_buffer + 0x10, 0xffffffff);
		WriteU32(p_buffer + 0x14, 0xffffffff);
	}

	p_hotX = Add(p_hotX, clip.m_originX);
	p_hotY = Add(p_hotY, clip.m_originY);
	width = (uint32_t) clip.m_right + 1 - (uint32_t) clip.m_left;
	encoder.m_left = 0x7fffffff;
	encoder.m_top = 0x7fffffff;
	encoder.m_right = -0x7fffffff;
	encoder.m_bottom = -0x7fffffff;

	// The opaque pixels' bounds
	row = PixelOffset(&clip, clip.m_left, clip.m_top);
	for (y = clip.m_top; y <= clip.m_bottom; y++) {
		const uint8_t* pixels = clip.m_pixels + row;
		uint32_t first;
		uint32_t last;

		for (first = 0; first < width && pixels[first] == p_transparentColor; first++) {
		}

		if (first < width) {
			for (last = width - 1; pixels[last] == p_transparentColor; last--) {
			}

			encoder.m_top = encoder.m_top < y ? encoder.m_top : y;
			encoder.m_bottom = encoder.m_bottom > y ? encoder.m_bottom : y;
			encoder.m_left =
				encoder.m_left < Add(clip.m_left, (int32_t) first) ? encoder.m_left : Add(clip.m_left, (int32_t) first);
			encoder.m_right =
				encoder.m_right > Add(clip.m_left, (int32_t) last) ? encoder.m_right : Add(clip.m_left, (int32_t) last);
		}

		row += clip.m_pitch;
	}

	if (p_buffer) {
		WriteU32(p_buffer + 8, (uint32_t) Sub(encoder.m_left, p_hotX));
		WriteU32(p_buffer + 0xc, (uint32_t) Sub(encoder.m_top, p_hotY));
		WriteU32(p_buffer + 0x10, (uint32_t) Sub(encoder.m_right, p_hotX));
		WriteU32(p_buffer + 0x14, (uint32_t) Sub(encoder.m_bottom, p_hotY));
	}

	encoder.m_cursor = 0x18;
	if (encoder.m_top <= encoder.m_bottom) {
		uint32_t bounds = (uint32_t) encoder.m_right + 1 - (uint32_t) encoder.m_left;
		int32_t bottom = encoder.m_bottom;

		encoder.m_row = clip.m_pixels + PixelOffset(&clip, encoder.m_left, encoder.m_top);
		for (y = encoder.m_top; y <= bottom; y++) {
			ScanLine(&encoder, bounds, p_transparentColor, clip.m_left);
			encoder.m_row += clip.m_pitch;
		}
	}

	return PortableS32(encoder.m_cursor);
}

// --- Dissolve ---

// Copies up to p_count pixels from p_src to p_dest, the next ones in the order of a maximal LFSR
// over the pixel indices (x in the state's high bits, y in its low ones), from p_state (0 starts
// a dissolve, else a state it returned). Pixels outside either view count but aren't copied;
// when the LFSR comes back to 1, the next pixel copied is the last. Returns the state to continue
// from. The views are at most 768 rows high (576 for p_dest: the assembly's row tables run on
// into other globals), and must share a pixel: the assembly looks for one forever otherwise, as
// it does from a state wider than the LFSR, which can fall to 0.
int32_t VFX_pixel_fade(PANE* p_source, PANE* p_destination, int32_t p_intervals, int32_t p_rnd)
{
	ClippedView source;
	ClippedView dest;
	int32_t result;
	int32_t width;
	int32_t height;
	uint32_t yMask = 0;
	uint32_t yBits = 0;
	uint32_t bits;
	uint32_t taps;
	uint32_t state;
	uint32_t x;
	uint32_t y;
	uint32_t value;

	result = ClipView(&source, p_source);
	if (result) {
		return result;
	}

	PORTABLE_ASSERT(Add(Sub(p_source->m_y1, p_source->m_y0), 1) > 0);
	PORTABLE_ASSERT(Add(Sub(p_source->m_y1, p_source->m_y0), 1) <= 0x300);
	result = ClipView(&dest, p_destination);
	if (result) {
		return result;
	}

	PORTABLE_ASSERT(Add(Sub(p_destination->m_y1, p_destination->m_y0), 1) > 0);
	PORTABLE_ASSERT(Add(Sub(p_destination->m_y1, p_destination->m_y0), 1) <= 0x240);
	width = Add(Sub(p_source->m_x1, p_source->m_x0), 1);
	height = Add(Sub(p_source->m_y1, p_source->m_y0), 1);
	if (Add(Sub(p_destination->m_y1, p_destination->m_y0), 1) < height) {
		height = Add(Sub(p_destination->m_y1, p_destination->m_y0), 1);
	}

	if (Add(Sub(p_destination->m_x1, p_destination->m_x0), 1) <= width) {
		width = Add(Sub(p_destination->m_x1, p_destination->m_x0), 1);
	}

	// The LFSR's width: y's bits, then x's
	value = (uint32_t) height;
	do {
		yBits++;
		yMask = yMask << 1 | 1;
		value >>= 1;
	} while (value);

	bits = yBits;
	value = (uint32_t) width;
	do {
		bits++;
		value >>= 1;
	} while (value);

	PORTABLE_ASSERT(bits >= 2 && bits <= 32 && yBits < 32);
	taps = g_pfConstants[bits - 2];
	state = (uint32_t) p_rnd;
	if (state == 0) {
		state = 1;
		x = 0;
		y = 0;
		goto visit;
	}

	for (;;) {
		state = state & 1 ? (state >> 1) ^ taps : state >> 1;
		if (state == 1) {
			p_intervals = 0;
		}

		x = state >> yBits;
		if (x >= (uint32_t) width) {
			continue;
		}

		y = state & yMask;
		if (y >= (uint32_t) height) {
			continue;
		}

	visit:
		p_intervals--;
		if (Add((int32_t) x, dest.m_originX) < dest.m_left || Add((int32_t) x, dest.m_originX) > dest.m_right ||
			Add((int32_t) x, source.m_originX) < source.m_left || Add((int32_t) x, source.m_originX) > source.m_right ||
			Add((int32_t) y, dest.m_originY) < dest.m_top || Add((int32_t) y, dest.m_originY) > dest.m_bottom ||
			Add((int32_t) y, source.m_originY) < source.m_top || Add((int32_t) y, source.m_originY) > source.m_bottom) {
			continue;
		}

		dest.m_pixels[PixelOffset(&dest, dest.m_originX, dest.m_originY) + y * dest.m_pitch + x] =
			source.m_pixels[PixelOffset(&source, source.m_originX, source.m_originY) + y * source.m_pitch + x];
		if (p_intervals < 0) {
			return PortableS32(state);
		}
	}
}

// --- Colors ---

// A byte compared as signed.
static int32_t SignedByte(uint32_t p_value)
{
	p_value &= 0xff;
	return p_value < 0x80 ? (int32_t) p_value : (int32_t) p_value - 0x100;
}

// Fades the colors used in p_buffer's pixels to p_palette's in steps, through the display
// driver: it reads each color's components, steps them towards the palette's by Bresenham
// errors (color_error), sets them at each step, and waits between steps p_steps times over all.
void VFX_window_fade(WINDOW* p_buffer, uint8_t* p_palette, int32_t p_intervals)
{
	uint8_t used[0x100];
	uint8_t colors[0x100];
	uint8_t current[0x300];
	uint8_t distances[0x300];
	uint8_t directions[0x300];
	const uint8_t* pixels = p_buffer->m_buffer;
	uint32_t count = ((uint32_t) p_buffer->m_xMax + 1) * ((uint32_t) p_buffer->m_yMax + 1);
	int32_t last = -1;
	uint8_t farthest = 0;
	uint32_t steps;
	uint32_t stepFraction;
	uint32_t wait;
	int32_t i;
	int32_t j;

	PORTABLE_ASSERT(count != 0);
	memset(used, 0, sizeof(used));
	do {
		uint8_t color = *pixels++;

		if (!used[color]) {
			used[color] = 1;
			colors[++last] = color;
			VFX_DAC_read(color, &current[color * 3]);
		}
	} while (--count);

	for (i = last; i >= 0; i--) {
		uint32_t base = colors[i] * 3u;

		for (j = 0; j < 3; j++) {
			uint32_t index = base + (uint32_t) j;
			uint8_t distance = (uint8_t) (current[index] - p_palette[index]);

			if (SignedByte(current[index]) >= SignedByte(p_palette[index])) {
				directions[index] = 0xff;
			}
			else {
				directions[index] = 1;
				distance = (uint8_t) (0 - distance);
			}

			distances[index] = distance;
			if (SignedByte(distance) > SignedByte(farthest)) {
				farthest = distance;
			}
		}
	}

	memset(color_error, farthest >> 1, (uint32_t) last);
	steps = farthest;
	if (!steps) {
		return;
	}

	stepFraction = PortableDiv((uint64_t) (uint32_t) p_intervals << 16, steps);
	wait = 0x8000;
	do {
		for (i = last; i >= 0; i--) {
			uint32_t base = colors[i] * 3u;

			for (j = 2; j >= 0; j--) {
				uint32_t index = base + (uint32_t) j;
				uint8_t error = (uint8_t) (color_error[index] + distances[index]);

				if (SignedByte(error) >= SignedByte(farthest)) {
					error = (uint8_t) (error - farthest);
					current[index] = (uint8_t) (current[index] + directions[index]);
				}

				color_error[index] = error;
			}

			VFX_DAC_write(colors[i], &current[base]);
		}

		wait += stepFraction;
		if (PortableS16((uint16_t) (wait >> 16)) >= 1) {
			uint32_t waits = (wait >> 16) & 0xffff;

			do {
				VFX_wait_vblank_leading();
			} while (--waits);

			wait &= 0xffff;
		}
	} while (--steps);
}

// Counts the distinct colors in the view (its rectangle, not clipped), storing each in p_out
// when not NULL, in the order it finds them: rows down, each from its right.
int32_t VFX_color_scan(PANE* p_pane, uint32_t* p_colors)
{
	uint8_t used[0x100];
	uint32_t pitch = (uint32_t) p_pane->m_window->m_xMax + 1;
	const uint8_t* row = p_pane->m_window->m_buffer + ((uint32_t) p_pane->m_y0 * pitch + (uint32_t) p_pane->m_x0);
	int32_t right = Sub(p_pane->m_x1, p_pane->m_x0);
	int32_t rows = Sub(p_pane->m_y1, p_pane->m_y0);
	int32_t count = 0;
	int32_t x;

	memset(used, 0, sizeof(used));
	do {
		x = right;
		do {
			uint8_t color = row[x];

			if (!used[color]) {
				used[color] = 1;
				count++;
				if (p_colors) {
					*p_colors++ = color;
				}
			}
		} while (--x >= 0);

		row += pitch;
	} while (--rows >= 0);

	return count;
}
