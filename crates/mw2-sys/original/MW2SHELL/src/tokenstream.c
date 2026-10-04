#include "tokenstream.h"

#include "decomp.h"
#include "stringutil.h"
#include "types.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DECOMP_SIZE_ASSERT(TokenStream, 0x14)

// Reads up to 100 more tokens from the file. Returns c_tokenEndOfFile when the file is already
// at its end.
// Stack-slot permutation: token and count.
// FUNCTION: MW2SHELL 0x10039280
MechS32 ReadTokens(TokenStream* p_stream)
{
	MechChar token[500];
	MechS32 count;
	MechS32 result;

	if (feof(p_stream->m_file)) {
		return c_tokenEndOfFile;
	}

	count = 0;
	result = c_tokenDone;
	while (!feof(p_stream->m_file) && count < 100) {
		result = ReadFileToken(p_stream->m_file, token);
		if (result == c_tokenOk && strlen(token)) {
			AddToken(p_stream, token);
			count++;
		}
	}

	return c_tokenOk;
}

// FUNCTION: MW2SHELL 0x10039334
TokenStream* CreateTokenStream(FILE* p_file)
{
	TokenStream* stream;
	MechChar** tokens;

	stream = (TokenStream*) MechHeapAlloc(g_primaryHeap, sizeof(TokenStream));
	if (stream == NULL) {
		fprintf(stderr, "Could not allocate token stream\n");
		fflush(stderr);
		exit(1);
	}

	stream->m_count = 0;
	stream->m_index = 0;
	stream->m_capacity = 100;
	stream->m_file = p_file;

	tokens = (MechChar**) MechHeapAlloc(g_primaryHeap, stream->m_capacity * sizeof(MechChar*));
	if (tokens == NULL) {
		fprintf(stderr, "Could not allocate Token entry array\n");
		fflush(stderr);
		exit(1);
	}

	stream->m_tokens = tokens;
	ReadTokens(stream);

	return stream;
}

// FUNCTION: MW2SHELL 0x10039428
void SkipBlanks(FILE* p_file)
{
	MechChar c;

	c = fgetc(p_file);
	while (!feof(p_file) && (c == ' ' || c == '\t')) {
		c = fgetc(p_file);
	}

	if (!feof(p_file)) {
		ungetc(c, p_file);
	}
}

// Stack-slot permutation: buffer, length, last, c and trimmed.
// FUNCTION: MW2SHELL 0x1003949e
MechS32 ReadFileToken(FILE* p_file, MechChar* p_token)
{
	MechS32 last;
	MechChar buffer[500];
	MechChar c;
	MechU8 trimmed;
	MechS32 length;

	strcpy(p_token, "");
	strcpy(buffer, "");
	length = 0;
	if (feof(p_file)) {
		return c_tokenEndOfFile;
	}

	c = fgetc(p_file);
	while (!feof(p_file) && c != ' ' && c != '\n' && c != '\t' && c != '=') {
		buffer[length] = c;
		length++;
		c = fgetc(p_file);
	}

	if (c == ' ' || (c == '\n' && length > 0) || (c == '=' && length > 0) || c == '\t') {
		buffer[length] = '\0';
		if (c == '\n' || c == '=') {
			ungetc(c, p_file);
		}
	}

	if ((c == '\n' || c == '=') && length == 0) {
		buffer[0] = c;
		buffer[1] = '\0';
	}

	last = strlen(buffer) - 1;
	trimmed = FALSE;
	while (buffer[last] == ',') {
		last--;
		trimmed = TRUE;
	}

	if (trimmed == TRUE) {
		last++;
		buffer[last] = '\0';
	}

	SkipBlanks(p_file);
	strcpy(p_token, buffer);
	if (feof(p_file)) {
		return c_tokenEndOfFile;
	}

	return c_tokenOk;
}

// Refills the stream once every token has been handed out.
// FUNCTION: MW2SHELL 0x100396c2
MechS32 RefillTokens(TokenStream* p_stream)
{
	MechS32 result;

	if (p_stream->m_count <= p_stream->m_index) {
		if (feof(p_stream->m_file)) {
			return c_tokenEndOfFile;
		}
		else {
			p_stream->m_index = 0;
			p_stream->m_count = 0;
			result = ReadTokens(p_stream);
		}
	}

	return result;
}

// Takes the next token if it starts with p_key (any token for an empty key), copying it to
// p_value. Line ends are skipped unless p_key is one. A mismatch leaves the token in place, still
// copied, and returns c_tokenMismatch.
// FUNCTION: MW2SHELL 0x1003972c
MechS32 TakeToken(TokenStream* p_stream, MechChar* p_key, MechChar* p_value)
{
	if (RefillTokens(p_stream) == c_tokenEndOfFile) {
		return c_tokenEndOfFile;
	}

	if (strlen(p_key) == 0) {
		strcpy(p_value, p_stream->m_tokens[p_stream->m_index]);
		free(p_stream->m_tokens[p_stream->m_index]);
		p_stream->m_tokens[p_stream->m_index] = NULL;
		p_stream->m_index++;
		return c_tokenOk;
	}

	if (memcmp(p_key, "\n", 2)) {
		while (!memcmp("\n", p_stream->m_tokens[p_stream->m_index], 2)) {
			free(p_stream->m_tokens[p_stream->m_index]);
			p_stream->m_tokens[p_stream->m_index] = NULL;
			p_stream->m_index++;
			if (RefillTokens(p_stream) == c_tokenEndOfFile) {
				return c_tokenEndOfFile;
			}
		}
	}

	if (!strncmp(p_key, p_stream->m_tokens[p_stream->m_index], strlen(p_key))) {
		if (p_value) {
			strcpy(p_value, p_stream->m_tokens[p_stream->m_index]);
		}

		free(p_stream->m_tokens[p_stream->m_index]);
		p_stream->m_tokens[p_stream->m_index] = NULL;
		p_stream->m_index++;
		return c_tokenOk;
	}

	if (p_value) {
		strcpy(p_value, p_stream->m_tokens[p_stream->m_index]);
	}

	return c_tokenMismatch;
}

// FUNCTION: MW2SHELL 0x1003994b
void AddToken(TokenStream* p_stream, MechChar* p_token)
{
	MechChar** tokens;
	MechChar* copy;

	if (p_stream->m_capacity - 1 < p_stream->m_count) {
		p_stream->m_capacity += 100;
		tokens = (MechChar**) realloc(p_stream->m_tokens, p_stream->m_capacity * sizeof(MechChar*));
		if (tokens == NULL) {
			fprintf(stderr, "Failed to reallocate token array\n");
			fflush(stderr);
			exit(1);
		}

		p_stream->m_tokens = tokens;
	}

	tokens = p_stream->m_tokens;
	copy = AllocateString(p_token);
	tokens[p_stream->m_count] = copy;
	p_stream->m_count++;
}

// Skips the tokens up to the next line end.
// FUNCTION: MW2SHELL 0x100399fd
void SkipLine(TokenStream* p_stream)
{
	MechChar value[512];
	MechS32 result;

	result = c_tokenOk;
	while (result == c_tokenOk) {
		result = TakeToken(p_stream, "\n", NULL);
		if (result == c_tokenOk) {
			result = c_tokenDone;
		}
		else {
			result = TakeToken(p_stream, "", value);
		}
	}
}

// Joins the tokens up to the next line end into p_line, each followed by a space.
// FUNCTION: MW2SHELL 0x10039a6b
void ReadLine(TokenStream* p_stream, MechChar* p_line)
{
	MechChar value[512];
	MechS32 result;

	strcpy(p_line, "");
	result = c_tokenOk;
	while (result == c_tokenOk) {
		result = TakeToken(p_stream, "\n", NULL);
		if (result == c_tokenOk || result == c_tokenEndOfFile) {
			result = c_tokenDone;
		}
		else if (p_line) {
			result = TakeToken(p_stream, "", value);
			strcat(p_line, value);
			strcat(p_line, " ");
		}
	}
}
