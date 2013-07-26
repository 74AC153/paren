#include <stdio.h>
#include "parse.h"

int main(int argc, char *argv[])
{
	node_t *result;
	char *remain;
	parse_err_t status;
	//size_t count;
	int count = 0;

	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}

	nodes_initialize();

	remain = argv[1];
	while(remain && remain[0] && count++ < 10) {
		status = parse(remain, &remain, &result, NULL);
		if(status != PARSE_OK) {
			printf("parse error before: %s\n", (*remain ? remain : "end of input"));
			printf("-- %s\n", parse_err_str(status));
			return -1;
		}
		printf("parse result: ");
		node_print_pretty(result, false);
		printf("\n");
		node_print_recursive(result);

		node_droproot(result);
	}


	printf("*** cleanup ***\n");
	node_gc();
	node_gc();

	node_gc_print_state();

	return 0;
}
