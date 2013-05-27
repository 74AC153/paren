#include <stdio.h>
#include "token.h"

int main(int argc, char *argv[])
{
	char *start;
	tok_state_t state;
	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}
	tok_state_init(&state);
	for(start = argv[1], token_chomp(&start, &state);
	    token_type(&state) != TOK_NONE;
	    token_chomp(&start, &state)) {
		token_print(&state);
	}
	return 0;
}
