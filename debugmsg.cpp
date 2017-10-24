#include "common.h"

void DebugMsg(const char *fmt,...) {
	va_list val;
	va_start(val, fmt);
	char buf[1024];
	wvsprintf(buf, fmt, val);
	va_end(val);
	OutputDebugString(buf);
}
