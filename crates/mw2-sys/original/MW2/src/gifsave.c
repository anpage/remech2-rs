#include "files.h"
#include "gifsave.h"

#include "types.h"

#include <stdio.h>
#include <stdlib.h>

// The screenshot GIF writer: Sverre H. Huseby's public-domain GIFSAVE, built with 32-bit
// words. GifCreate writes the header and the screen descriptor, GifSetColor fills the
// global color table, GifCompressImage writes an image LZW-compressed from a pixel
// callback, and GifClose writes the trailer.

// Reserved codes past the color codes: the clear code and the end-of-information code.
#define GIF_RESERVED_CODES 2

#define GIF_HASH_FREE 0xffff
#define GIF_NEXT_FIRST 0xffff

#define GIF_MAX_BITS 12
#define GIF_MAX_STRINGS (1 << GIF_MAX_BITS)

#define GIF_HASH_SIZE 9973
#define GIF_HASH_STEP 2039

#define GIF_HASH(p_index, p_lastByte) ((((p_lastByte) << 8) ^ (p_index)) % GIF_HASH_SIZE)

#pragma pack(push, 1)

// SIZE 0xb
typedef struct GifScreenDescriptor {
	MechU32 m_width;                   // 0x00
	MechU32 m_height;                  // 0x04
	MechU8 m_globalColorTableSize : 3; // 0x08
	MechU8 m_sortFlag : 1;             // 0x08
	MechU8 m_colorResolution : 3;      // 0x08
	MechU8 m_globalColorTableFlag : 1; // 0x08
	MechU8 m_backgroundColorIndex;     // 0x09
	MechU8 m_pixelAspectRatio;         // 0x0a
} GifScreenDescriptor;

// SIZE 0x12
typedef struct GifImageDescriptor {
	MechU8 m_separator;               // 0x00
	MechU32 m_left;                   // 0x01
	MechU32 m_top;                    // 0x05
	MechU32 m_width;                  // 0x09
	MechU32 m_height;                 // 0x0d
	MechU8 m_localColorTableSize : 3; // 0x11
	MechU8 m_reserved : 2;            // 0x11
	MechU8 m_sortFlag : 1;            // 0x11
	MechU8 m_interlaceFlag : 1;       // 0x11
	MechU8 m_localColorTableFlag : 1; // 0x11
} GifImageDescriptor;

#pragma pack(pop)

static MechS32 GifOpenFile(const MechChar* p_filename);
static MechS32 GifWrite(const void* p_buffer, MechU32 p_length);
static MechS32 BitsNeeded(MechU32 p_n);
static MechS32 WriteScreenDescriptor(GifScreenDescriptor* p_descriptor);
static MechS32 GifWriteByte(MechU8 p_byte);
static MechS32 GifWriteWord(MechU32 p_word);
static MechS32 LzwCompress(MechS32 p_codeSize, MechS32 (*p_inputByte)(void));
static void InitBitFile(void);
static MechS32 ResetOutBitFile(void);
static MechS32 WriteBits(MechS32 p_bits, MechS32 p_numBits);
static void FreeStringTable(void);
static MechS32 AllocStringTable(void);
static MechU32 AddCharString(MechU32 p_index, MechU8 p_byte);
static MechU32 FindCharString(MechU32 p_index, MechU8 p_byte);
static void ClearStringTable(MechS32 p_codeSize);
static MechS32 InputByte(void);
static MechS32 WriteImageDescriptor(GifImageDescriptor* p_descriptor);
static void GifCloseFile(void);

// The bit file: the current data sub-block and the bits left in its current byte.

// GLOBAL: MW2 0x100bf1b4
static FILE* g_outFile;

// GLOBAL: MW2 0x100bf0a0
static MechU8 g_gifBuffer[256];

// GLOBAL: MW2 0x100bf1b0
static MechS32 g_gifIndex;

// GLOBAL: MW2 0x100bf1b8
static MechS32 g_bitsLeft;

// The LZW string table: each string's last byte, its prefix string, and the hash table.

// GLOBAL: MW2 0x100b141c
static MechU8* g_strChr = NULL;

// GLOBAL: MW2 0x100b1420
static MechU32* g_strNxt = NULL;

// GLOBAL: MW2 0x100b1424
static MechU32* g_strHsh = NULL;

// GLOBAL: MW2 0x100bf1c0
static MechU32 g_numStrings;

// GLOBAL: MW2 0x100bf09c
static MechS32 g_bitsPrPrimColor;

// GLOBAL: MW2 0x100bf1bc
static MechS32 g_numColors;

// GLOBAL: MW2 0x100b1428
static MechU8* g_colorTable = NULL;

// GLOBAL: MW2 0x100bf08c
static MechU32 g_gifScreenHeight;

// GLOBAL: MW2 0x100bf088
static MechU32 g_gifScreenWidth;

// GLOBAL: MW2 0x100bf1a0
static MechU32 g_imageHeight;

// GLOBAL: MW2 0x100bf1ac
static MechU32 g_imageWidth;

// GLOBAL: MW2 0x100bf1a8
static MechU32 g_imageLeft;

// GLOBAL: MW2 0x100bf090
static MechU32 g_imageTop;

// GLOBAL: MW2 0x100bf098
static MechU32 g_relPixX;

// GLOBAL: MW2 0x100bf094
static MechU32 g_relPixY;

// GLOBAL: MW2 0x100bf1a4
static MechS32 (*g_getPixel)(MechS32 p_x, MechS32 p_y);

// Operand order: the loop test (q < tabSize) compares with tabSize in eax in the original, and the
// color table size's bitfield store loads the byte before the value.
// FUNCTION: MW2 0x10074920
MechS32 GifCreate(
	const MechChar* p_filename,
	MechU32 p_width,
	MechU32 p_height,
	MechU32 p_numColors,
	MechS32 p_colorRes
)
{
	MechS32 q;
	MechS32 tabSize;
	MechU8* bp;
	GifScreenDescriptor sd;

	g_numColors = p_numColors ? (1 << BitsNeeded(p_numColors)) : 0;
	g_bitsPrPrimColor = p_colorRes;
	g_gifScreenHeight = p_height;
	g_gifScreenWidth = p_width;

	if (GifOpenFile(p_filename) != c_gifOk) {
		return c_gifErrCreate;
	}

	if (GifWrite("GIF87a", 6) != c_gifOk) {
		return c_gifErrWrite;
	}

	sd.m_width = p_width;
	sd.m_height = p_height;
	if (g_numColors) {
		sd.m_globalColorTableSize = BitsNeeded(g_numColors) - 1;
		sd.m_globalColorTableFlag = 1;
	}
	else {
		sd.m_globalColorTableSize = 0;
		sd.m_globalColorTableFlag = 0;
	}
	sd.m_sortFlag = 0;
	sd.m_colorResolution = p_colorRes - 1;
	sd.m_backgroundColorIndex = 0;
	sd.m_pixelAspectRatio = 0;
	if (WriteScreenDescriptor(&sd) != c_gifOk) {
		return c_gifErrWrite;
	}

	if (g_colorTable) {
		free(g_colorTable);
		g_colorTable = NULL;
	}
	if (g_numColors) {
		tabSize = g_numColors * 3;
		if ((g_colorTable = (MechU8*) malloc(tabSize * sizeof(MechU8))) == NULL) {
			return c_gifOutOfMemory;
		}
		else {
			bp = g_colorTable;
			for (q = 0; q < tabSize; q++) {
				*bp++ = 0;
			}
		}
	}

	return c_gifOk;
}

// FUNCTION: MW2 0x10074ae9
static MechS32 GifOpenFile(const MechChar* p_filename)
{
	if ((g_outFile = MechFopen(p_filename, "wb")) == NULL) {
		return c_gifErrCreate;
	}

	return c_gifOk;
}

// FUNCTION: MW2 0x10074b28
static MechS32 GifWrite(const void* p_buffer, MechU32 p_length)
{
	if (fwrite(p_buffer, sizeof(MechU8), p_length, g_outFile) < p_length) {
		return c_gifErrWrite;
	}

	return c_gifOk;
}

// FUNCTION: MW2 0x10074b65
static MechS32 BitsNeeded(MechU32 p_n)
{
	MechS32 ret = 1;

	if (!p_n--) {
		return 0;
	}
	while (p_n >>= 1) {
		++ret;
	}

	return ret;
}

// Operand order: the original evaluates the flag bits' `|` operands in a different order.
// FUNCTION: MW2 0x10074bb1
static MechS32 WriteScreenDescriptor(GifScreenDescriptor* p_descriptor)
{
	MechU8 tmp;

	if (GifWriteWord(p_descriptor->m_width) != c_gifOk) {
		return c_gifErrWrite;
	}
	if (GifWriteWord(p_descriptor->m_height) != c_gifOk) {
		return c_gifErrWrite;
	}
	tmp = (p_descriptor->m_globalColorTableFlag << 7) | (p_descriptor->m_colorResolution << 4) |
		  (p_descriptor->m_sortFlag << 3) | p_descriptor->m_globalColorTableSize;
	if (GifWriteByte(tmp) != c_gifOk) {
		return c_gifErrWrite;
	}
	if (GifWriteByte(p_descriptor->m_backgroundColorIndex) != c_gifOk) {
		return c_gifErrWrite;
	}
	if (GifWriteByte(p_descriptor->m_pixelAspectRatio) != c_gifOk) {
		return c_gifErrWrite;
	}

	return c_gifOk;
}

// FUNCTION: MW2 0x10074cba
static MechS32 GifWriteByte(MechU8 p_byte)
{
	if (putc(p_byte, g_outFile) == EOF) {
		return c_gifErrWrite;
	}

	return c_gifOk;
}

// FUNCTION: MW2 0x10074cf3
static MechS32 GifWriteWord(MechU32 p_word)
{
	if (putc(p_word & 0xff, g_outFile) == EOF) {
		return c_gifErrWrite;
	}
	if (putc(p_word >> 8, g_outFile) == EOF) {
		return c_gifErrWrite;
	}

	return c_gifOk;
}

// FUNCTION: MW2 0x10074d57
void GifSetColor(MechS32 p_colorNum, MechS32 p_red, MechS32 p_green, MechS32 p_blue)
{
	MechS32 maxColor;
	MechU8* p;

	maxColor = (1L << g_bitsPrPrimColor) - 1L;
	p = g_colorTable + p_colorNum * 3;
	*p++ = (MechU8) ((p_red * 255L) / maxColor);
	*p++ = (MechU8) ((p_green * 255L) / maxColor);
	*p++ = (MechU8) ((p_blue * 255L) / maxColor);
}

// FUNCTION: MW2 0x10074dc7
MechS32 GifCompressImage(
	MechS32 p_left,
	MechS32 p_top,
	MechS32 p_width,
	MechS32 p_height,
	MechS32 (*p_getPixel)(MechS32 p_x, MechS32 p_y)
)
{
	MechS32 codeSize;
	MechS32 errCode;
	GifImageDescriptor id;

	if (p_width < 0) {
		p_width = g_gifScreenWidth;
		p_left = 0;
	}
	if (p_height < 0) {
		p_height = g_gifScreenHeight;
		p_top = 0;
	}
	if (p_left < 0) {
		p_left = 0;
	}
	if (p_top < 0) {
		p_top = 0;
	}

	if (g_numColors) {
		if (GifWrite(g_colorTable, g_numColors * 3) != c_gifOk) {
			return c_gifErrWrite;
		}
	}

	id.m_separator = ',';
	id.m_left = g_imageLeft = p_left;
	id.m_top = g_imageTop = p_top;
	id.m_width = g_imageWidth = p_width;
	id.m_height = g_imageHeight = p_height;
	id.m_localColorTableSize = 0;
	id.m_reserved = 0;
	id.m_sortFlag = 0;
	id.m_interlaceFlag = 0;
	id.m_localColorTableFlag = 0;
	if (WriteImageDescriptor(&id) != c_gifOk) {
		return c_gifErrWrite;
	}

	codeSize = BitsNeeded(g_numColors);
	if (codeSize == 1) {
		++codeSize;
	}
	if (GifWriteByte(codeSize) != c_gifOk) {
		return c_gifErrWrite;
	}

	g_relPixX = g_relPixY = 0;
	g_getPixel = p_getPixel;

	if ((errCode = LzwCompress(codeSize, InputByte)) != c_gifOk) {
		return errCode;
	}

	if (GifWriteByte(0) != c_gifOk) {
		return c_gifErrWrite;
	}

	return c_gifOk;
}

// Stack-slot permutation: clearCode, endOfInfo and limit.
// FUNCTION: MW2 0x10074f76
static MechS32 LzwCompress(MechS32 p_codeSize, MechS32 (*p_inputByte)(void))
{
	MechS32 c;
	MechU32 index;
	MechS32 clearCode;
	MechS32 endOfInfo;
	MechS32 numBits;
	MechS32 limit;
	MechS32 errCode;
	MechU32 prefix = 0xffff;

	InitBitFile();

	clearCode = 1 << p_codeSize;
	endOfInfo = clearCode + 1;

	numBits = p_codeSize + 1;
	limit = (1 << numBits) - 1;

	if ((errCode = AllocStringTable()) != c_gifOk) {
		return errCode;
	}
	ClearStringTable(p_codeSize);

	WriteBits(clearCode, numBits);

	while ((c = p_inputByte()) != -1) {
		if ((index = FindCharString(prefix, c)) != 0xffff) {
			prefix = index;
		}
		else {
			WriteBits(prefix, numBits);
			if (AddCharString(prefix, c) > limit) {
				if (++numBits > GIF_MAX_BITS) {
					WriteBits(clearCode, numBits - 1);
					ClearStringTable(p_codeSize);
					numBits = p_codeSize + 1;
				}
				limit = (1 << numBits) - 1;
			}
			prefix = c;
		}
	}

	if (prefix != 0xffff) {
		WriteBits(prefix, numBits);
	}

	WriteBits(endOfInfo, numBits);

	ResetOutBitFile();

	FreeStringTable();

	return c_gifOk;
}

// FUNCTION: MW2 0x100750db
static void InitBitFile(void)
{
	g_gifBuffer[g_gifIndex = 0] = 0;
	g_bitsLeft = 8;
}

// FUNCTION: MW2 0x10075106
static MechS32 ResetOutBitFile(void)
{
	MechU8 numBytes;

	numBytes = g_gifIndex + (g_bitsLeft == 8 ? 0 : 1);
	if (numBytes) {
		if (GifWriteByte(numBytes) != c_gifOk) {
			return -1;
		}
		if (GifWrite(g_gifBuffer, numBytes) != c_gifOk) {
			return -1;
		}
		g_gifBuffer[g_gifIndex = 0] = 0;
		g_bitsLeft = 8;
	}

	return 0;
}

// Stack-slot permutation: bitsWritten and numBytes. Operand order: the original compares
// p_numBits <= g_bitsLeft with g_bitsLeft in eax.
// FUNCTION: MW2 0x100751a0
static MechS32 WriteBits(MechS32 p_bits, MechS32 p_numBits)
{
	MechS32 bitsWritten = 0;
	MechU8 numBytes = 255;

	do {
		if ((g_gifIndex == 254 && !g_bitsLeft) || g_gifIndex > 254) {
			if (GifWriteByte(numBytes) != c_gifOk) {
				return -1;
			}
			if (GifWrite(g_gifBuffer, numBytes) != c_gifOk) {
				return -1;
			}
			g_gifBuffer[g_gifIndex = 0] = 0;
			g_bitsLeft = 8;
		}

		if (p_numBits <= g_bitsLeft) {
			g_gifBuffer[g_gifIndex] |= (p_bits & ((1 << p_numBits) - 1)) << (8 - g_bitsLeft);
			bitsWritten += p_numBits;
			g_bitsLeft -= p_numBits;
			p_numBits = 0;
		}
		else {
			g_gifBuffer[g_gifIndex] |= (p_bits & ((1 << g_bitsLeft) - 1)) << (8 - g_bitsLeft);
			bitsWritten += g_bitsLeft;
			p_bits >>= g_bitsLeft;
			p_numBits -= g_bitsLeft;
			g_gifBuffer[++g_gifIndex] = 0;
			g_bitsLeft = 8;
		}
	} while (p_numBits);

	return bitsWritten;
}

// FUNCTION: MW2 0x1007532b
static void FreeStringTable(void)
{
	if (g_strHsh) {
		free(g_strHsh);
		g_strHsh = NULL;
	}
	if (g_strNxt) {
		free(g_strNxt);
		g_strNxt = NULL;
	}
	if (g_strChr) {
		free(g_strChr);
		g_strChr = NULL;
	}
}

// FUNCTION: MW2 0x100753a5
static MechS32 AllocStringTable(void)
{
	FreeStringTable();

	if ((g_strChr = (MechU8*) malloc(GIF_MAX_STRINGS * sizeof(MechU8))) == NULL) {
		FreeStringTable();
		return c_gifOutOfMemory;
	}
	if ((g_strNxt = (MechU32*) malloc(GIF_MAX_STRINGS * sizeof(MechU32))) == NULL) {
		FreeStringTable();
		return c_gifOutOfMemory;
	}
	if ((g_strHsh = (MechU32*) malloc(GIF_HASH_SIZE * sizeof(MechU32))) == NULL) {
		FreeStringTable();
		return c_gifOutOfMemory;
	}

	return c_gifOk;
}

// FUNCTION: MW2 0x10075446
static MechU32 AddCharString(MechU32 p_index, MechU8 p_byte)
{
	MechU32 hshIdx;

	if (g_numStrings >= GIF_MAX_STRINGS) {
		return 0xffff;
	}

	hshIdx = GIF_HASH(p_index, p_byte);
	while (g_strHsh[hshIdx] != 0xffff) {
		hshIdx = (hshIdx + GIF_HASH_STEP) % GIF_HASH_SIZE;
	}

	g_strHsh[hshIdx] = g_numStrings;
	g_strChr[g_numStrings] = p_byte;
	g_strNxt[g_numStrings] = (p_index != 0xffff) ? p_index : GIF_NEXT_FIRST;

	return g_numStrings++;
}

// FUNCTION: MW2 0x10075523
static MechU32 FindCharString(MechU32 p_index, MechU8 p_byte)
{
	MechU32 hshIdx;
	MechU32 nxtIdx;

	if (p_index == 0xffff) {
		return p_byte;
	}

	hshIdx = GIF_HASH(p_index, p_byte);
	while ((nxtIdx = g_strHsh[hshIdx]) != 0xffff) {
		if (g_strNxt[nxtIdx] == p_index && g_strChr[nxtIdx] == p_byte) {
			return nxtIdx;
		}
		hshIdx = (hshIdx + GIF_HASH_STEP) % GIF_HASH_SIZE;
	}

	return 0xffff;
}

// FUNCTION: MW2 0x100755d6
static void ClearStringTable(MechS32 p_codeSize)
{
	MechS32 q;
	MechS32 w;
	MechU32* wp;

	g_numStrings = 0;

	wp = g_strHsh;
	for (q = 0; q < GIF_HASH_SIZE; q++) {
		*wp++ = GIF_HASH_FREE;
	}

	w = (1 << p_codeSize) + GIF_RESERVED_CODES;
	for (q = 0; q < w; q++) {
		AddCharString(0xffff, q);
	}
}

// FUNCTION: MW2 0x10075665
static MechS32 InputByte(void)
{
	MechS32 ret;

	if (g_relPixY >= g_imageHeight) {
		return -1;
	}

	ret = g_getPixel(g_imageLeft + g_relPixX, g_imageTop + g_relPixY);
	if (++g_relPixX >= g_imageWidth) {
		g_relPixX = 0;
		++g_relPixY;
	}

	return ret;
}

// Operand order: the original evaluates the flag bits' `|` operands in a different order.
// FUNCTION: MW2 0x100756e1
static MechS32 WriteImageDescriptor(GifImageDescriptor* p_descriptor)
{
	MechU8 tmp;

	if (GifWriteByte(p_descriptor->m_separator) != c_gifOk) {
		return c_gifErrWrite;
	}
	if (GifWriteWord(p_descriptor->m_left) != c_gifOk) {
		return c_gifErrWrite;
	}
	if (GifWriteWord(p_descriptor->m_top) != c_gifOk) {
		return c_gifErrWrite;
	}
	if (GifWriteWord(p_descriptor->m_width) != c_gifOk) {
		return c_gifErrWrite;
	}
	if (GifWriteWord(p_descriptor->m_height) != c_gifOk) {
		return c_gifErrWrite;
	}
	tmp = (p_descriptor->m_localColorTableFlag << 7) | (p_descriptor->m_interlaceFlag << 6) |
		  (p_descriptor->m_sortFlag << 5) | (p_descriptor->m_reserved << 3) | p_descriptor->m_localColorTableSize;
	if (GifWriteByte(tmp) != c_gifOk) {
		return c_gifErrWrite;
	}

	return c_gifOk;
}

// FUNCTION: MW2 0x10075823
MechS32 GifClose(void)
{
	GifImageDescriptor id;

	id.m_separator = ';';
	if (WriteImageDescriptor(&id) != c_gifOk) {
		return c_gifErrWrite;
	}

	GifCloseFile();

	if (g_colorTable) {
		free(g_colorTable);
		g_colorTable = NULL;
	}

	return c_gifOk;
}

// FUNCTION: MW2 0x10075884
static void GifCloseFile(void)
{
	fclose(g_outFile);
}
