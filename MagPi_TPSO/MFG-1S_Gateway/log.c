#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include "log.h"
#include "globals.h"

// messages passed should be only single line, call log_write multiple times for multiple lines
void log_write(int facility_priority, char *format, ...)
{
	va_list args;

	va_start(args, format);

	if (facility_priority <= g_config.loglevel && CONSOLE_OUT)
	{
		if (facility_priority <= LOG_ERR)
			vfprintf(stderr, format, args);
		else
			vfprintf(stdout, format, args);
		printf("\n");
	}
	vsyslog(facility_priority, format, args);
	va_end(args);
}

void log_start(int loglevel)
{
	setlogmask(LOG_UPTO (loglevel));
	openlog("mfg1s", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}
