#include <assert.h>
#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "eval.h"
#include "builtin_load.h"
#include "node.h"
#include "environ.h"
#include "load_wrapper.h"
#include "foreign_common.h"
#include "parse.h"
#include "map_file.h"
#include "bufstream.h"
#include "fdstream.h"

static eval_err_t loadlibfn(memory_state_t *ms, node_t **n, node_t **result, void *p)
{
	char *path = NULL;
	size_t len;
	ldwrap_ent_t *ld_funs = NULL, *ld_funs_cursor;
	eval_err_t status = EVAL_OK;
	node_t *cursor, *val;
	node_t *env_handle = (node_t *) p;

	/* count path length */
	if(count_list_len(n[0], &len) != 0) {
		status = eval_err(EVAL_ERR_EXPECTED_CONS);
		goto cleanup;
	}

	/* make path string */
	if(!(path = alloca(len + 1))) {
		status = eval_err(EVAL_ERR_OUT_OF_MEM);
		goto cleanup;
	}
	if(list_to_cstr(n[0], path, len + 1, NULL)) {
		status = eval_err(EVAL_ERR_FOREIGN_FAILURE);
		goto cleanup;
	}

	/* load libs */
	if(load_wrapper(path, &ld_funs)) {
		status = eval_err(EVAL_ERR_FOREIGN_FAILURE);
		goto cleanup;
	}
	
	/* update environment */
	assert(node_type(env_handle) == NODE_HANDLE);
	if(! ld_funs) {
		status = eval_err(EVAL_ERR_FOREIGN_FAILURE);
		goto cleanup;
	}
	for(ld_funs_cursor = ld_funs; ld_funs_cursor->name; ld_funs_cursor++) {
		node_t *name, *val;
		name = node_symbol_new(ms, ld_funs_cursor->name);
		val = node_foreign_new(ms, ld_funs_cursor->fun);
		environ_add(env_handle, name, val);
	}

cleanup:
	free(ld_funs);
	return status;
}


eval_err_t foreign_loadlib(memory_state_t *ms, node_t *args, node_t *env_handle, node_t **result)
{
	return extract_args(ms, 1, loadlibfn, args, result, env_handle);
}

static eval_err_t readevalfn(memory_state_t *ms, node_t **n, node_t **result, void *p)
{
	char *path = NULL;
	int map_status;
	size_t len;
	node_t *cursor, *val;
	node_t *env_handle = (node_t *) p;

	parse_err_t parse_stat;
	filemap_info_t info;
	stream_t *stream;
	tok_state_t ts;
	bufstream_t bs;
	eval_err_t eval_stat = EVAL_OK;
	node_t *eval_in_hdl = NULL, *eval_out_hdl = NULL;

	/* count path length */
	if(count_list_len(n[0], &len) != 0) {
		eval_stat  = eval_err(EVAL_ERR_EXPECTED_CONS);
		goto cleanup;
	}

	/* make path string */
	if(!(path = alloca(len + 1))) {
		eval_stat  = eval_err(EVAL_ERR_OUT_OF_MEM);
		goto cleanup;
	}
	if(list_to_cstr(n[0], path, len + 1, NULL)) {
		eval_stat  = eval_err(EVAL_ERR_FOREIGN_FAILURE);
		goto cleanup;
	}


	map_status = map_file(&info, path);
	switch(map_status) {
	default: break;
	case 1:
		fprintf(stderr, "error: cannot open() file %s: %s (errno=%d)\n",
		        path, strerror(info.err), info.err);		
		eval_stat = eval_err(EVAL_ERR_FOREIGN_FAILURE);
		goto cleanup;
	case 2:
		fprintf(stderr, "error: canno stat() file %s: %s (errno=%d)\n",
		        path, strerror(info.err), info.err);		
		eval_stat = eval_err(EVAL_ERR_FOREIGN_FAILURE);
		goto cleanup;
	case 3:
		fprintf(stderr, "error: cannot mmap() file %s: %s (errno=%d)\n",
		        path, strerror(info.err), info.err);		
		eval_stat = eval_err(EVAL_ERR_FOREIGN_FAILURE);
		goto cleanup;
	}

	stream = bufstream_init(&bs, info.buf, info.len);
	tok_state_init(&ts, stream);

	eval_in_hdl = node_lockroot(node_handle_new(ms, NULL));
	eval_out_hdl = node_lockroot(node_handle_new(ms, NULL));

	while((parse_stat = parse(ms, &ts, eval_in_hdl)) != PARSE_END) {
		if(parse_stat != PARSE_OK) {
			printf("parse error line %llu char %llu (offset %llu)\n",
			       (unsigned long long) tok_state_line(&ts),
			       (unsigned long long) tok_state_linechr(&ts),
			       (unsigned long long) tok_state_offset(&ts));
			printf("-- %s\n", parse_err_str(parse_stat));
			eval_stat = eval_err(EVAL_ERR_FOREIGN_FAILURE);
			goto cleanup;
		}
		eval_stat = eval(ms, env_handle, eval_in_hdl, eval_out_hdl);
		if(eval_stat) {
			printf("eval error for: \n");
			node_print_pretty_stream(g_stream_stdout,
			                         node_handle(eval_out_hdl), false);
			printf("\n");
			printf("-- %s\n", eval_err_str(eval_stat));
			goto cleanup;
		}
		node_handle_update(eval_in_hdl, NULL);
		node_handle_update(eval_out_hdl, NULL);
	}

cleanup:

	node_droproot(eval_in_hdl);
	node_droproot(eval_out_hdl);
	node_droproot(env_handle);
	unmap_file(&info);

	return eval_stat;
}

eval_err_t foreign_read_eval(memory_state_t *ms, node_t *args, node_t *env_handle, node_t **result)
{
	return extract_args(ms, 1, readevalfn, args, result, env_handle);
}
