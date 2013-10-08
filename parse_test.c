#include <stdio.h>
#include <string.h>
#include "bufstream.h"
#include "fdstream.h"
#include "parse.h"
#include "memory.h"
#include "node.h"
#include "dbgtrace.h"
#include "malloc_wrapper.h"

#if defined(NO_GC_FREELIST)
#include "freemem_cache.h"
#endif

int main(int argc, char *argv[])
{
	node_t *result_hdl;
	parse_err_t status;

	memory_state_t ms;
	tok_state_t ts;
	stream_t *stream;
	bufstream_t bs;
#if defined(NO_GC_FREELIST)
	fmcache_state_t fmc_state;
#endif

	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}

	dbgtrace_setstream(g_stream_stdout);
	dbgtrace_enable( 0
	               //| TC_MEM_ALLOC
	               //| TC_GC_TRACING
	               //| TC_MEM_RC
	               //| TC_GC_VERBOSE
	               //| TC_NODE_GC
	               //| TC_NODE_INIT
	               //| TC_EVAL
	               | TC_FMC_ALLOC
	               );


#if defined(NO_GC_FREELIST)
	fmcache_state_init(&fmc_state, libc_malloc_wrap, libc_free_wrap, NULL);
	node_memstate_init(&ms, fmcache_request, fmcache_return, &fmc_state);
#else
	node_memstate_init(&ms, libc_malloc_wrap, libc_free_wrap, NULL);
#endif

	stream = bufstream_init(&bs, argv[1], strlen(argv[1]));
	tok_state_init(&ts, stream);

	result_hdl = node_handle_new(&ms, NULL);

	while((status = parse(&ms, &ts, result_hdl)) != PARSE_END) {
		if(status != PARSE_OK) {
			printf("parse error line %llu char %llu (offset %llu)\n",
			       (unsigned long long) tok_state_line(&ts),
			       (unsigned long long) tok_state_linechr(&ts),
			       (unsigned long long) tok_state_offset(&ts));
			printf("-- %s\n", parse_err_str(status));
			return -1;
		}
		printf("parse result: ");
		node_print_pretty_stream(g_stream_stdout,
		                         node_handle(result_hdl), false);
		printf("\n");
		node_print_recursive_stream(g_stream_stdout, (node_handle(result_hdl)));

		node_handle_update(result_hdl, NULL);
	}


	printf("*** cleanup ***\n");
	node_droproot(result_hdl);
	memory_gc_cycle(&ms);
	memory_gc_cycle(&ms);

	printf("*** ending state ***\n");
	memory_gc_print_state(&ms, g_stream_stdout);

	memory_state_reset(&ms);
#if defined(NO_GC_FREELIST)
	fmcache_state_reset(&fmc_state);
#endif
	return 0;
}
