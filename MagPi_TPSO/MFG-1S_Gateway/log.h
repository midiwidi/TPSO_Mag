#ifndef LOG_H_
#define LOG_H_

#include <syslog.h>

void log_write(int facility_priority, char *format, ...);
void log_start(int loglevel);

#endif /* LOG_H_ */
