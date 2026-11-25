#ifndef TOKENSTREAM_H
#define TOKENSTREAM_H

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
	typedef struct TokenStream {
		int32_t m_capacity; // 0x00
		int32_t m_count;    // 0x04
		int32_t m_index;    // 0x08 — next token to hand out
		FILE* m_file;       // 0x0c
		char** m_tokens;    // 0x10
	} TokenStream;

	int32_t ReadTokens(TokenStream* p_stream);
	TokenStream* CreateTokenStream(FILE* p_file);
	void SkipBlanks(FILE* p_file);
	int32_t ReadFileToken(FILE* p_file, char* p_token);
	int32_t RefillTokens(TokenStream* p_stream);
	int32_t TakeToken(TokenStream* p_stream, char* p_key, char* p_value);
	void AddToken(TokenStream* p_stream, char* p_token);
	void SkipLine(TokenStream* p_stream);
	void ReadLine(TokenStream* p_stream, char* p_line);

#ifdef __cplusplus
}
#endif

#endif // TOKENSTREAM_H
