#include "logwindow.h"

#include "log.h"
#include "types.h"

#include <stdarg.h>
#include <stdio.h>

void DebugPrintInternal(const char* p_format, ...)
{
	char message[0x100];
	va_list args;

	va_start(args, p_format);
	vsnprintf(message, sizeof(message), p_format, args);
	va_end(args);

	MechLogDebug(message);
}
