#include <stdio.h>
#include "parse.h"

int main(int argc, char *argv[])
{
	node_t *result_hdl;
	char *remain;
	parse_err_t status;
	//size_t count;
	int count = 0;

	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}

	nodes_initialize();
	result_hdl = node_handle_new(NULL);

	remain = argv[1];
	while(remain && remain[0] && count++ < 10) {
		status = parse(remain, &remain, result_hdl, NULL);
		if(status != PARSE_OK) {
			printf("parse error before: %s\n", (*remain ? remain : "end of input"));
			printf("-- %s\n", parse_err_str(status));
			return -1;
		}
		printf("parse result: ");
		node_print_pretty(node_handle(result_hdl), false);
		printf("\n");
		node_print_recursive(node_handle(result_hdl));

		node_handle_update(result_hdl, NULL);
	}


	printf("*** cleanup ***\n");
	node_gc();
	node_gc();

	node_gc_print_state();

	return 0;
}
