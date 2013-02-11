#include <stdio.h>
#include "parse.h"

int main(int argc, char *argv[])
{
	node_t *result;
	char *remain;

	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}

	result = parse(argv[1], &remain);
	node_retain(result);

	node_print_pretty(result);
	printf("\n");
	node_print(result, true);

	node_release(result);

	printf("remaining: %s\n", remain);

	return 0;
}
