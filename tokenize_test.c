#include <stdio.h>
#include <string.h>
#include "token.h"
#include "bufstream.h"
#include "fdstream.h"

int main(int argc, char *argv[])
{
	char *start;
	bufstream_t bs;
	fdstream_t fs;
	stream_t *b_stream;
	stream_t *f_stream;
	tok_state_t state;
	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}
	b_stream = bufstream_init(&bs, (unsigned char *) argv[1], strlen(argv[1]));
	f_stream = fdstream_init(&fs, STDOUT_FILENO);

	tok_state_init(&state, b_stream);
	token_chomp(&state);
	while(token_type(&state) != TOK_END) {
		token_print(f_stream, &state);
		token_chomp(&state);
	}
	return 0;
}
