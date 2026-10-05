#ifndef TOKENSTREAM_H
#define TOKENSTREAM_H

#include "decomp.h"
#include "types.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

	// Status codes of the token stream functions.
	enum {
		c_tokenOk = 0,
		c_tokenMismatch = 1,
		c_tokenEndOfFile = 2,
		c_tokenDone = 3
	};

	// Whitespace-separated tokens read ahead from a text file, up to 100 at a time. Line ends and
	// '=' are tokens of their own, and trailing commas are dropped.
	// SIZE 0x14
	typedef struct TokenStream {
		MechS32 m_capacity;  // 0x00
		MechS32 m_count;     // 0x04
		MechS32 m_index;     // 0x08 — next token to hand out
		FILE* m_file;        // 0x0c
		MechChar** m_tokens; // 0x10
	} TokenStream;

	MechS32 ReadTokens(TokenStream* p_stream);
	TokenStream* CreateTokenStream(FILE* p_file);
	void SkipBlanks(FILE* p_file);
	MechS32 ReadFileToken(FILE* p_file, MechChar* p_token);
	MechS32 RefillTokens(TokenStream* p_stream);
	MechS32 TakeToken(TokenStream* p_stream, MechChar* p_key, MechChar* p_value);
	void AddToken(TokenStream* p_stream, MechChar* p_token);
	void SkipLine(TokenStream* p_stream);
	void ReadLine(TokenStream* p_stream, MechChar* p_line);

#ifdef __cplusplus
}
#endif

#endif // TOKENSTREAM_H
