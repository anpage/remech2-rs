#include "inifile.h"

#include "decomp.h"
#include "files.h"
#include "types.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

// Where FindIniSection found the section's entries in mw2.ini, or -1.
// GLOBAL: MW2 0x100ad46c
MechS32 g_iniSectionOffset = -1;

// GLOBAL: MW2 0x100bee88
MechChar g_iniLine[0x85];

// Finds the section p_section in mw2.ini for GetIniValue. Returns 0, 0x36 when the file can't
// be opened or 0x3f when the section isn't there. A line starting with '[' ends the search.
// Stack-slot permutation: line, end and buffer.
// FUNCTION: MW2 0x1006c6c0
MechS32 FindIniSection(MechChar* p_section)
{
	FILE* file;
	MechChar* line;
	MechChar* end;
	MechChar buffer[0x85];

	g_iniSectionOffset = -1;
	file = MechFopen("mw2.ini", "rt");
	if (!file) {
		return 0x36;
	}

	p_section = TrimWhitespace(p_section);
	for (;;) {
		if (!fgets(buffer, 0x85, file)) {
			fclose(file);
			return 0x3f;
		}

		line = buffer + strspn(buffer, " \t");
		if (*line == '[') {
			break;
		}

		end = strchr(line, ']');
		if (!end) {
			continue;
		}

		*end = '\0';
		if (!_strcmpi(TrimWhitespace(line), p_section)) {
			break;
		}
	}

	g_iniSectionOffset = ftell(file);
	fclose(file);
	return 0;
}

// Returns the value of p_key in the section FindIniSection found, or an empty string. The
// value's last character (the newline) is cut off.
// Stack-slot permutation: value, length and line.
// FUNCTION: MW2 0x1006c817
MechChar* GetIniValue(MechChar* p_key)
{
	FILE* file;
	MechChar* value;
	size_t length;
	MechChar* line;

	if (g_iniSectionOffset == -1 || !(file = MechFopen("mw2.ini", "rt"))) {
		return "";
	}

	fseek(file, g_iniSectionOffset, SEEK_SET);
	p_key = TrimWhitespace(p_key);
	for (;;) {
		value = "";
		if (!fgets(g_iniLine, 0x85, file)) {
			break;
		}

		line = g_iniLine + strspn(g_iniLine, " \t");
		if (*line == '[') {
			break;
		}

		value = strchr(line, '=');
		if (!value) {
			continue;
		}

		*value = '\0';
		value++;
		if (!_strcmpi(TrimWhitespace(line), p_key)) {
			break;
		}
	}

	fclose(file);
	length = strlen(value);
	if (length > 0) {
		value[length - 1] = '\0';
	}

	return value;
}

// Cuts the whitespace off both ends of p_string, in place.
// FUNCTION: MW2 0x1006c968
MechChar* TrimWhitespace(MechChar* p_string)
{
	MechS32 length;

	if (p_string) {
		while (isspace(*p_string)) {
			p_string++;
		}

		length = strlen(p_string);
		while (length-- > 0 && isspace(p_string[length])) {
		}

		p_string[length + 1] = '\0';
	}

	return p_string;
}
