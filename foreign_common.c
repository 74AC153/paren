#include <alloca.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "foreign_common.h"
#include "eval_err.h"

eval_err_t extract_args(
	unsigned int n,
	callback_t cb,
	node_t *args,
	node_t **result,
	void *p)
{
	unsigned int i;
	node_t *cursor = args;
	node_t **arglist = alloca(n * sizeof(node_t *));
	for(i = 0; i < n; i++) {
		if(node_type(cursor) != NODE_CONS) {
			*result = args;
			return eval_err(EVAL_ERR_EXPECTED_CONS);
		}
		arglist[i] = node_cons_car(cursor);
		cursor = node_cons_cdr(cursor);
	}
	if(cursor) {
		*result = args;
		return eval_err(EVAL_ERR_TOO_MANY_ARGS);
	}
	return cb(arglist, result, p);
}

int count_list_len(node_t *cons, size_t *len)
{
	node_t *cursor;
	int status = 0;

	*len = 0;
	for(cursor = cons;
	    cursor;
	    cursor = node_cons_cdr(cursor)) {
		if(node_type(cursor) != NODE_CONS) {
			status = -1;
			break;
		}
		(*len)++;
	}

	return status;
}

int list_to_cstr(
	node_t *clist,
	char *strbuf,
	size_t buflen,
	size_t *written)
{
	node_t *cursor;
	node_t *val;
	size_t off;
	int status = 0;
	value_t cval;

	off = 0;
	for(cursor = clist; cursor; cursor = node_cons_cdr(cursor)) {
		if(node_type(cursor) != NODE_CONS) {
			status = -1;
			goto finish;
		}
		val = node_cons_car(cursor);
		if(node_type(val) != NODE_VALUE) {
			status = -2;
			goto finish;
		}
		cval = node_value(val);
		if(cval > UCHAR_MAX || cval < 0) {
			status = -3;
			goto finish;
		}
		if(off == (buflen)) {
			status = -4;
			goto finish;
		}
		strbuf[off++] = (unsigned char) cval;
	}
	if(off == (buflen)) {
		status = -4;
		goto finish;
	}
	strbuf[off++] = 0;
finish:
	if(written) *written = off;
	return status;
}

node_t *generate_argv(int argc, char *argv[])
{
	unsigned int len;
	int i, c;
	node_t *cnode, *lnode, *argv_head;

	argv_head = NULL;
	for(i = argc - 1; i >= 0; i--) {
		len = strlen(argv[i]);
		lnode = NULL;
		for(c = len - 1; c >= 0; c--) {
			cnode = node_value_new(argv[i][c]);
			lnode = node_cons_new(cnode, lnode);
		}
		argv_head = node_cons_new(lnode, argv_head);
	}
	return argv_head;
}

