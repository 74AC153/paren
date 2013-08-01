#include <stdlib.h>
#include <stdio.h>

#include "node.h"
#include "foreign_common.h"

static eval_err_t do_node_print(node_t **args, node_t **result, void *p)
{
	node_print(args[0]);
	return EVAL_OK;
}
eval_err_t testutil_node_print(
	node_t *args,
	node_t *env_handle,
	node_t **result)
{
	return extract_args(1, do_node_print, args, result, NULL);
}

static eval_err_t do_node_print_pretty(node_t **args, node_t **result, void *p)
{
	node_print_pretty(args[0], 0);
	printf("\n");
	return EVAL_OK;
}
eval_err_t testutil_node_print_pretty(
	node_t *args,
	node_t *env_handle,
	node_t **result)
{
	return extract_args(1, do_node_print_pretty, args, result, NULL);
}

struct { char *name, *nmemonic; } testutil_data_names[] = {
	{ "testutil_node_print",        "testutil:nodeprint" },
	{ "testutil_node_print_pretty", "testutil:nodeprintpretty" },
};

size_t testutil_data_count =
	sizeof(testutil_data_names) / sizeof(testutil_data_names[0]);
