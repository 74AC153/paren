#if ! defined(DBGTRACE_H)
#define DBGTRACE_H

#include <stdbool.h>
#include "stream.h"
#include "traceclass.h"

void dbgtrace_setstream(stream_t *s);
stream_t *dbgtrace_getstream(void);
void dbgtrace_enable(traceclass_t c);
void dbgtrace_disable(traceclass_t c);
bool dbgtrace_enabled(traceclass_t c);
/* each arg is char *, end with NULL */
void dbgtrace(traceclass_t c, ...); 

#if defined(DBGTRACE_ENABLED)
#define DBGTRACELN(x, ...) dbgtrace(x, __VA_ARGS__, "\n", NULL);
#define DBGTRACE(x, ...) dbgtrace(x, __VA_ARGS__, NULL);
#define DBGRUN(x, BLOCK) do { if(dbgtrace_enabled(x)) BLOCK } while (0)
#define DBGSTMT(x) x
#else
#define DBGTRACELN(x, ...)
#define DBGTRACE(x, ...)
#define DBGRUN(x, BLOCK)
#define DBGSTMT(x)
#endif

#endif
