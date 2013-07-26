#include <stdio.h>
#include <limits.h>
#include "node.h"
#include "foreign_common.h"

static eval_err_t do_sio_getchar(node_t **args, node_t **result, void *p)
{
	*result = node_value_new(getchar());
	return EVAL_OK;
}
eval_err_t sio_getchar(node_t *args, node_t *env_handle, node_t **result)
{
	return extract_args(0, do_sio_getchar, args, result, NULL);
}

static eval_err_t do_sio_putchar(node_t **args, node_t **result, void *p)
{
	value_t v = node_value(args[0]);
	if(v >= (0x1 << CHAR_BIT)) {
		return EVAL_ERR_VALUE_BOUNDS;
	}
	*result = node_value_new(putchar(v));
	return EVAL_OK;
}
eval_err_t sio_putchar(node_t *args, node_t *env_handle, node_t **result)
{
	return extract_args(1, do_sio_putchar, args, result, NULL);
}

static eval_err_t do_sio_eputchar(node_t **args, node_t **result, void *p)
{
	value_t v = node_value(args[0]);
	if(v >= (0x1 << CHAR_BIT)) {
		return EVAL_ERR_VALUE_BOUNDS;
	}
	*result = node_value_new(fputc(v, stderr));
	return EVAL_OK;
}
eval_err_t sio_eputchar(node_t *args, node_t *env_handle, node_t **result)
{
	return extract_args(1, do_sio_eputchar, args, result, NULL);
}

struct { char *name, *nmemonic; } sio_data_names[] = {
	{ "sio_getchar",  "sio:getchar" },
	{ "sio_putchar",  "sio:putchar" },
	{ "sio_eputchar", "sio:eputchar" },
};

size_t sio_data_count = sizeof(sio_data_names) / sizeof(sio_data_names[0]);

