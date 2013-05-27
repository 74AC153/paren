#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SYM_LEN 32

#define TOK_TYPES \
X(TOK_NONE) \
X(TOK_LPAREN) \
X(TOK_RPAREN) \
X(TOK_DOT) \
X(TOK_QUOTE) \
X(TOK_SYM) \
X(TOK_LIT)

typedef enum {
#define X(name) name,
TOK_TYPES
#undef X
} toktype_t;

struct token {
	toktype_t type;
	bool in_string;
	union {
		char sym[MAX_SYM_LEN+1]; // include space for tailing null
		int64_t lit;
	} u;
};
typedef struct token tok_state_t;

tok_state_t *tok_state_init(void *p);
void token_chomp(char **input, tok_state_t *state);
toktype_t token_type(tok_state_t *state);
char *token_sym(tok_state_t *state);
int64_t token_lit(tok_state_t *state);
char *token_type_str(tok_state_t *state);
void token_print(tok_state_t *state);

#endif
