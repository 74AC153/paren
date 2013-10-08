#if ! defined(FOREIGN_COMMON_H)
#define FOREIGN_COMMON_H

#include "node.h"
#include "eval.h"

typedef eval_err_t (*callback_t)(
	memory_state_t *ms,
	node_t **args,
	node_t **result,
	void *p);

eval_err_t extract_args(
	memory_state_t *ms,
	unsigned int n,
	callback_t cb,
	node_t *args,
	node_t **result,
	void *p);

/* sets *len to number of elements in list 'cons'.
   return -1 if cons is not a well-formed list */
int count_list_len(node_t *cons, size_t *len);

/* copies values of clist into character buf strbuf.
   will NULL-terminate strbuf, so make sure it has enough space
   returns -1 if the list is not well-formed
   returns -2 if the list does not contain only NODE_VALUE
   returns -3 if the values in the list are out of bounds for an unsigned char
   returns -4 if the input buffer is not long enough to hold the string and
              a terminating NULL byte.
*/
int list_to_cstr(
	node_t *clist,
	char *strbuf,
	size_t buflen,
	size_t *written);

node_t *generate_argv(memory_state_t *ms, int argc, char *argv[]);

#endif
