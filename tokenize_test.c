#include <stdio.h>
#include <string.h>
#include "token.h"

int main(int argc, char *argv[])
{
	char *start;
	bufstream_t bs;
	stream_t stream;
	tok_state_t state;
	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}
	bufstream_init(&bs, (unsigned char *) argv[1], strlen(argv[1]));
	stream_init(&stream, bufstream_readch, &bs);
	tok_state_init(&state, &stream);
	token_chomp(&state);
	while(token_type(&state) != TOK_END) {
		token_print(&state);
		token_chomp(&state);
	}
	return 0;
}
