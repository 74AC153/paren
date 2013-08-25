#include <assert.h>
#include <alloca.h>
#include <stdlib.h>

#include "builtin_load.h"
#include "node.h"
#include "environ.h"
#include "load_wrapper.h"
#include "foreign_common.h"
#include "parse.h"

static eval_err_t loadfn(node_t **n, node_t **result, void *p)
{
	char *path = NULL;
	int load_status;
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
		name = node_symbol_new(ld_funs_cursor->name);
		val = node_foreign_new(ld_funs_cursor->fun);
		environ_add(env_handle, name, val);
	}

cleanup:
	free(ld_funs);
	return status;
}


eval_err_t foreign_loadlib(node_t *args, node_t *env_handle, node_t **result)
{
	return extract_args(1, loadfn, args, result, env_handle);
}

#if 0
static eval_err_t parsefilefn(node_t **n, node_t **result, void *p)
{
	char *path = NULL;
	size_t len;
	eval_err_t status = EVAL_OK;
	node_t *cursor, *val;
	node_t *env_handle = (node_t *) p;

	char *filebuf = NULL;
	filemap_info_t info;
	int status;

	/* count path length */
	for(cursor = n[0], len = 0;
	    cursor;
	    cursor = node_cons_cdr(cursor), len++) {
		if(node_type(cursor) != NODE_CONS) {
			status = EVAL_ERR_EXPECTED_CONS;
			goto cleanup;
		}
	}

	/* make path string */
	path = alloca(len + 1);
	if(!path) {
		status = EVAL_ERR_OUT_OF_MEM;
		goto cleanup;
	}
	
	for(cursor = n[0], len = 0; cursor; cursor = node_cons_cdr(cursor), len++) {
		if(node_type(cursor) != NODE_CONS) {
			status = EVAL_ERR_EXPECTED_CONS;
			goto cleanup;
		}
		val = node_cons_car(cursor);
		if(node_type(val) != NODE_VALUE) {
			status = EVAL_ERR_EXPECTED_VALUE;
			goto cleanup;
		}
		if(node_value(val) > 255 || node_value(val) < 0) {
			status = EVAL_ERR_VALUE_BOUNDS;
			goto cleanup;
		}
		path[len] = node_value(val);
	}
	path[len] = 0;

	status = map_file(&info, path);
	
	
}

eval_err_t foreign_parsefile(node_t *args, node_t *env_handle, node_t **result)
{
	return extract_args(1, parsefilefn, args, result, env_handle);
}
#endif

#if 0
typedef struct {
	int fd;
	void *buf;
	size_t len;
	int err;
} filemap_info_t;

static int map_file(filemap_info_t *info, char *path)
{
	struct stat sb;
	int status = 0;

	info->fd = open(path, O_RDONLY);
	if(info->fd < 0) {
		status = -1;
		info->err = errno;
		goto finish;
	}

	status = fstat(info->fd, &sb);
	if(status < 0) {
		status = -2;
		info->err = errno;
		goto finish;
	}
	info->len = sb.st_size;

	info->buf = mmap(NULL, info->len, PROT_READ, MAP_PRIVATE, info->fd, 0);
	if(info->buf == MAP_FAILED) {
		status = -3;
		info->err = errno;
		goto finish;
	}

finish:
	return status;
}

static void unmap_file(filemap_info_t *info)
{
	if(info->buf != NULL && info->buf != MAP_FAILED) {
		munmap(info->buf, info->len);
	}
	if(info->fd >= 0) {
		close(info->fd);
	}
}


static eval_err_t mmapfilefn(node_t **n, node_t **result, void *p)
{
	char *path = NULL;
	size_t len;
	eval_err_t status = EVAL_OK;
	node_t *cursor, *val;

	filemap_info_t *info;

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

	if(!(info = malloc(sizeof(*info)))) {
		status = eval_err(EVAL_ERR_OUT_OF_MEM);
		goto cleanup;
	}
	info->fd = -1;
	info->buf = MAP_FAILED;
	info->len = 0;
	info->err = 0;

	switch(map_file(info, path)) {
	default: break;
	case -1:
		fprintf(stderr, "error: cannot open() file %s: %s (errno=%d)\n",
		        argv[1], strerror(info.err), info.err);		
		goto map_err;
	case -2:
		fprintf(stderr, "error: canno stat() file %s: %s (errno=%d)\n",
		        argv[1], strerror(info.err), info.err);		
		goto map_err;
	case -3:
		fprintf(stderr, "error: cannot mmap() file %s: %s (errno=%d)\n",
		        argv[1], strerror(info.err), info.err);		
		goto map_err;
	}
	
	*result = node_blob_new(info, (blob_fin_t) unmap_file, BLOB_SIG_MMAPFILE);

cleanup:
	return status;

map_err:
	unmap_file(&info);
	return status;
}

eval_err_t foreign_mmapfile(node_t *args, node_t *env_handle, node_t **result)
{
	return extract_args(1, mmapfilefn, args, result, NULL);
}
#endif
