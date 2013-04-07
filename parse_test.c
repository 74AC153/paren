#include <stdio.h>
#include "parse.h"

int print_callback(node_t *n, void *p)
{
	p = p;
	node_print(n, false);
	return 0;
}

int main(int argc, char *argv[])
{
	node_t *result;
	char *remain;
	parse_err_t status;
	size_t count;

	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}

	status = parse(argv[1], &remain, &result);
	if(status != PARSE_OK) {
		printf("parse error before: %s\n", (*remain ? remain : "end of input"));
		printf("%s\n", parse_err_str(status));
		return -1;
	}

	node_print_pretty(result);
	printf("\n");
	node_print(result, true);

	node_release(result);

	printf("tokens remaining: %s\n", remain);

	printf("*** cleanup ***\n");
	count = node_gc();
	printf("%llu nodes freed\n", (unsigned long long) count);

	printf("leaked nodes:\n");
	node_find_live(print_callback, NULL);

	return 0;
}
