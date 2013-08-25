#include <stdio.h>
#include <string.h>
#include "token.h"
#include "parse.h"

int main(int argc, char *argv[])
{
	node_t *result_hdl;
	parse_err_t status;

	parse_state_t state;
	stream_t stream;
	bufstream_t bs;


	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}

	bufstream_init(&bs, (unsigned char *) argv[1], strlen(argv[1]));
	stream_init(&stream, bufstream_readch, &bs);
	parse_state_init(&state, &stream);

	nodes_initialize();
	result_hdl = node_handle_new(NULL);

	while((status = parse(&state, result_hdl)) != PARSE_END) {
		if(status != PARSE_OK) {
			off_t off, line, lchr;
			parse_location(&state, &off, &line, &lchr);
			
			printf("parse error line %llu char %llu (offset %llu)\n",
			       (unsigned long long) line,
			       (unsigned long long) lchr,
			       (unsigned long long) off);
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

	nodes_reset();

	return 0;
}
