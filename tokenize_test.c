#include <stdio.h>
#include "token.h"

int main(int argc, char *argv[])
{
	char *start;
	token_t *tok, *tok_list;
	if(argc != 2) {
		printf("usage: %s 'string'\n", argv[0]);
		return -1;
	}
	for(start = argv[1], tok = first_tok(&start, &tok_list);
	    tok;
	    tok = next_tok(&start, &tok_list)) {
		print_tok(tok);
	}
	return 0;
}
