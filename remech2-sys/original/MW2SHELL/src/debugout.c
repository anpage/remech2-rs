#include "debugout.h"

#include "log.h"
#include "types.h"

#include <stdarg.h>
#include <stdio.h>

void DebugPrintInternal(char* p_message, ...)
{
	char message[0x100];
	va_list args;

	va_start(args, p_message);
	vsnprintf(message, sizeof(message), p_message, args);
	va_end(args);

	MechLogDebug(message);
}
