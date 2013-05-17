#include <stdio.h>
#include "parse.h"

int main(int argc, char *argv[])
{
	node_t *result;
	char *remain;
	parse_err_t status;
	//size_t count;

	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}

	nodes_initialize();

	status = parse(argv[1], &remain, &result);
	if(status != PARSE_OK) {
		printf("parse error before: %s\n", (*remain ? remain : "end of input"));
		printf("%s\n", parse_err_str(status));
		return -1;
	}

	node_print_pretty(result);
	printf("\n");
	node_print_recursive(result);

	node_forget(result);

	printf("*** cleanup ***\n");
	node_gc();
	node_gc();

	node_gc_state();

	return 0;
}
