#ifndef TOKEN_H
#define TOKEN_H

#define MAX_SYM_LEN 32

#define TOK_TYPES \
X(TOK_NONE) \
X(TOK_LPAREN) \
X(TOK_RPAREN) \
X(TOK_DOT) \
X(TOK_ATOM)

/*
TOK_SYM
TOK_LIT
*/

typedef enum {
#define X(name) name,
TOK_TYPES
#undef X
} toktype_t;

struct token {
	struct token *next;
	toktype_t type;
	char sym[MAX_SYM_LEN];
};
typedef struct token token_t;

/* return first token */
token_t *first_tok(char **input, token_t **tok_list);
/* free first token and return following one */
token_t *next_tok(char **input, token_t **tok_list);
toktype_t tok_type(token_t *tok);
char *tok_sym(token_t *tok);

char *tok_type_str(token_t *tok);
void print_tok(token_t *token);

/* add token to front of list (not used?) */
void push_tok(token_t *token, token_t **tok_list);

#endif
