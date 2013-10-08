#include <stdint.h>
#include <stdarg.h>
#include "stream.h"
#include "dbgtrace.h"

static stream_t *g_dbgstrm = NULL;

static tracemask_t g_tmask;

void dbgtrace_setstream(stream_t *s)
{
	g_dbgstrm = s;
}

stream_t *dbgtrace_getstream(void)
{
	return g_dbgstrm;
}

void dbgtrace_enable(traceclass_t c)
{
	tracemask_set(&g_tmask, c);
}

void dbgtrace_disable(traceclass_t c)
{
	tracemask_reset(&g_tmask, c);
}

bool dbgtrace_enabled(traceclass_t c)
{
	return tracemask_test(&g_tmask, c);
}

void dbgtrace(traceclass_t c, ...)
{
	if(g_dbgstrm && tracemask_test(&g_tmask, c)){
		va_list ap;
		va_start(ap, c);
		stream_putv(g_dbgstrm, ap);
		va_end(ap);
	}
}
