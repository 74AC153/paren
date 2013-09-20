#include <stdint.h>
#include <stddef.h>
#include <assert.h>
//#if defined(NODE_GC_TRACING) || defined(NODE_INIT_TRACING)
#include <stdio.h>
//#endif

#include "dlist.h"
#include "node.h"
#include "memory.h"
#include "libc_custom.h"
#include "stream.h"

char *node_type_names[] = {
	#define X(name) #name,
	NODE_TYPES
	#undef X
	NULL
};

char *node_special_names[] = {
	#define X(name) #name,
	NODE_SPECIALS
	#undef X
	NULL
};

memory_state_t g_memstate;

static void links_cb(void (*cb)(void *link, void *p), void *data, void *p)
{
	node_t *n = (node_t *) data;

	switch(node_type(n)) {
	case NODE_CONS:
#if defined(NODE_GC_TRACING)
		printf("node links_cb: cons %p links to c=%p n=%p\n",
		       n, n->dat.cons.car, n->dat.cons.cdr);
#endif
		cb(n->dat.cons.car, p);
		cb(n->dat.cons.cdr, p);
		break;
	case NODE_LAMBDA:
#if defined(NODE_GC_TRACING)
		printf("node links_cb: lam %p links to e=%p v=%p e=%p\n",
		       n, n->dat.lambda.env, n->dat.lambda.vars, n->dat.lambda.vars);
#endif
		cb(n->dat.lambda.env, p);
		cb(n->dat.lambda.vars, p);
		cb(n->dat.lambda.expr, p);
		break;
	case NODE_HANDLE:
#if defined(NODE_GC_TRACING)
		printf("node links_cb: hdl %p links to l=%p\n", n, n->dat.handle.link);
#endif
		cb(n->dat.handle.link, p);
		break;
	case NODE_CONTINUATION:
#if defined(NODE_GC_TRACING)
		printf("node links_cb: cont %p links to l=%p\n", n, n->dat.cont.bt);
#endif
		cb(n->dat.handle.link, p);
		break;
	default:
		break;
	}
}

static void node_print_wrap(void *p)
{
	node_print((node_t *) p);
}

static void node_init_cb(void *p)
{
	node_t *n = (node_t *) p;
	bzero_custom(n, sizeof(*n));
	n->type = NODE_UNINITIALIZED;
}

static void *node_mem_alloc(size_t len, void *p)
{
	return malloc(len);
}

static void node_mem_free(void *alloc, void *p)
{
	free(alloc);
}

void nodes_initialize()
{
	memory_state_init(&g_memstate,
	                  node_init_cb,
	                  links_cb,
	                  node_print_wrap,
	                  node_mem_alloc,
	                  node_mem_free,
	                  NULL);
}

void nodes_reset()
{
	memory_state_reset(&g_memstate);
}

#if defined(NODE_NO_INCREMENTAL_GC)
	#define NODE_GC_ITERATE()
#else
	#if defined(NODE_INCREMENTAL_FULL_GC)
	#define NODE_GC_ITERATE() do {node_gc();node_gc();} while(0)
	#else
	#define NODE_GC_ITERATE() memory_gc_iterate(&g_memstate)
	#endif
#endif

static node_t *node_new(void)
{
	node_t *n;
	n = memory_request(&g_memstate, sizeof(node_t));
	return n;
}

nodetype_t node_type(node_t *n)
{
	if(n) return n->type;
	return NODE_NIL;
}

static node_t *node_retain(node_t *n)
{
	if(n) {
		memory_gc_advise_new_link(&g_memstate, n);
	}
	return n;
}

static void node_release(node_t *n)
{
	if(n) {
		memory_gc_advise_stale_link(&g_memstate, n);
	}
}

static void node_droproot_int(node_t *n)
{
	if(n && memory_gc_isroot(&g_memstate, n)) {
		memory_gc_unlock(&g_memstate, n);
	}
}

void node_droproot(node_t *n)
{
	node_droproot_int(n);
	NODE_GC_ITERATE();
}

node_t *node_lockroot(node_t *n)
{
	if(n) {
		memory_gc_lock(&g_memstate, n);
	}
	NODE_GC_ITERATE();
	return n;
}

bool node_isroot(node_t *n)
{
	NODE_GC_ITERATE();
	return memory_gc_isroot(&g_memstate, n);
}

bool node_islocked(node_t *n)
{
	NODE_GC_ITERATE();
	return memory_gc_is_locked(n);
}

node_t *node_cons_new(node_t *car, node_t *cdr)
{
	node_t *ret = node_new();
	assert(ret);
	ret->dat.cons.car = node_retain(car);
	ret->dat.cons.cdr = node_retain(cdr);
	ret->type = NODE_CONS;
#if defined(NODE_INIT_TRACING)
	printf("node init cons %p (%p, %p)\n", ret, car, cdr);
#endif
	NODE_GC_ITERATE();
	NODE_GC_ITERATE();
	return ret;
}

void node_cons_patch_car(node_t *n, node_t *newcar)
{
	node_t *oldcar;

	assert(n);
	assert(n->type == NODE_CONS);

	oldcar = n->dat.cons.car;
	n->dat.cons.car = node_retain(newcar);

	node_release(oldcar);
	NODE_GC_ITERATE();
}

void node_cons_patch_cdr(node_t *n, node_t *newcdr)
{
	node_t *oldcdr;

	assert(n);
	assert(n->type == NODE_CONS);

	oldcdr = n->dat.cons.cdr;
	n->dat.cons.cdr = node_retain(newcdr);

	node_release(oldcdr);
	NODE_GC_ITERATE();
}

node_t *node_cons_car(node_t *n)
{
	node_t *ret = NULL;
	if(n) {
		assert(n->type == NODE_CONS);
		ret = n->dat.cons.car;
	}
	NODE_GC_ITERATE();
	return ret;
}

node_t *node_cons_cdr(node_t *n)
{
	node_t *ret = NULL;
	if(n) {
		assert(n->type == NODE_CONS);
		ret = n->dat.cons.cdr;
	}
	NODE_GC_ITERATE();
	return ret;
}

node_t *node_lambda_new(node_t *env, node_t *vars, node_t *expr)
{
	node_t *ret = node_new();
	assert(ret);
	ret->dat.lambda.env = node_retain(env);
	ret->dat.lambda.vars = node_retain(vars);
	ret->dat.lambda.expr = node_retain(expr);
	ret->type = NODE_LAMBDA;
#if defined(NODE_INIT_TRACING)
	printf("node init lambda %p (e=%p v=%p, x=%p)\n", ret, env, vars, expr);
#endif
	NODE_GC_ITERATE();
	NODE_GC_ITERATE();
	NODE_GC_ITERATE();
	return ret;
}

node_t *node_lambda_env(node_t *n)
{
	NODE_GC_ITERATE();
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.env;
}

node_t *node_lambda_vars(node_t *n)
{
	NODE_GC_ITERATE();
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.vars;
}

node_t *node_lambda_expr(node_t *n)
{
	NODE_GC_ITERATE();
	assert(n->type = NODE_LAMBDA);
	return n->dat.lambda.expr;
}

node_t *node_value_new(value_t  val)
{
	node_t *ret = node_new();
	assert(ret);
	ret->dat.value = val;
	ret->type = NODE_VALUE;
#if defined(NODE_INIT_TRACING)
	printf("node init value %p (%llu)\n", ret, (unsigned long long) val);
#endif
	NODE_GC_ITERATE();
	return ret;
}

value_t node_value(node_t *n)
{
	NODE_GC_ITERATE();
	assert(n->type == NODE_VALUE);
	return n->dat.value;
}

node_t *node_symbol_new(char *name)
{
	node_t *ret = node_new();
	assert(ret);
	strncpy_custom(ret->dat.name, name, sizeof(ret->dat.name));
	ret->type = NODE_SYMBOL;
#if defined(NODE_INIT_TRACING)
	printf("node init symbol %p (\"%s\")\n", ret, ret->dat.name);
#endif
	NODE_GC_ITERATE();
	return ret;
}

char *node_symbol_name(node_t *n)
{
	NODE_GC_ITERATE();
	assert(n->type == NODE_SYMBOL);
	return n->dat.name;
}

node_t *node_foreign_new(foreign_t func)
{
	node_t *ret = node_new();
	assert(ret);
	ret->dat.func = func;
	ret->type = NODE_FOREIGN;
#if defined(NODE_INIT_TRACING)
	printf("node init foreign %p (%p)\n", ret, ret->dat.func);
#endif
	NODE_GC_ITERATE();
	return ret;
}

foreign_t node_foreign_func(node_t *n)
{
	NODE_GC_ITERATE();
	assert(n->type == NODE_FOREIGN);
	return n->dat.func;
}

node_t *node_handle_new(node_t *link)
{
	node_t *ret = node_new();
	assert(ret);
	ret->type = NODE_HANDLE;
	ret->dat.handle.link = node_retain(link);
#if defined(NODE_INIT_TRACING)
	printf("node init handle %p (link=%p)\n", ret, link);
#endif
	NODE_GC_ITERATE();
	return ret;
}

node_t *node_handle(node_t *n)
{
	NODE_GC_ITERATE();
	assert(node_type(n) == NODE_HANDLE);
	return n->dat.handle.link;
}

void node_handle_update(node_t *n, node_t *newlink)
{
	node_t *oldlink;
	assert(node_type(n) == NODE_HANDLE);

	oldlink = n->dat.handle.link;
	n->dat.handle.link = node_retain(newlink);

	node_release(oldlink);
	NODE_GC_ITERATE();
}

node_t *node_cont_new(node_t *bt)
{
	node_t *ret = node_new();
	assert(ret);
	ret->type = NODE_CONTINUATION;
	ret->dat.cont.bt = node_retain(bt);
#if defined(NODE_INIT_TRACING)
	printf("node init cont %p (bt=%p)\n", ret, bt);
#endif
	NODE_GC_ITERATE();
	return ret;
}

node_t *node_cont(node_t *n)
{
	NODE_GC_ITERATE();
	assert(node_type(n) == NODE_CONTINUATION);
	return n->dat.cont.bt;
}

node_t *node_special_func_new(special_func_t func)
{
	node_t *ret = node_new();
	assert(ret);
	ret->type = NODE_SPECIAL_FUNC;
	ret->dat.special = func; 
#if defined(NODE_INIT_TRACING)
	printf("node init special %p (fun=%d)\n", ret, func);
#endif
	NODE_GC_ITERATE();
	return ret;

}

special_func_t node_special_func(node_t *n)
{
	assert(node_type(n) == NODE_SPECIAL_FUNC);
	return n->dat.special;
}

static void blob_fin_wrap(void *data)
{
	node_t *n = (node_t *) data;
	assert(node_type(n) == NODE_BLOB);
	if(n->dat.blob.fin) {
		n->dat.blob.fin(n->dat.blob.addr);
	}
}

node_t *node_blob_new(void *addr, blob_fin_t fin, uintptr_t sig)
{
	node_t *ret = node_new();
	assert(ret);
	memory_set_finalizer(ret, blob_fin_wrap);
	ret->type = NODE_BLOB;
	ret->dat.blob.addr = addr; 
	ret->dat.blob.fin = fin;
	ret->dat.blob.sig = sig;
#if defined(NODE_INIT_TRACING)
	printf("node init blob %p (addr=%p fin=%p sig=%llu)\n",
	       addr, fin, (unsigned long long) sig);
#endif
	NODE_GC_ITERATE();
	return ret;
}

void *node_blob_addr(node_t *n)
{
	assert(node_type(n) == NODE_BLOB);
	return n->dat.blob.addr;
}

uintptr_t node_blob_sig(node_t *n)
{
	assert(node_type(n) == NODE_BLOB);
	return n->dat.blob.sig;
}

void node_print(node_t *n)
{
	if(!n) {
		printf("node NULL");
	} else {
		printf("node@%p: ", n);

		switch(n->type) {
		case NODE_NIL:
			printf("null");
			break;
		case NODE_UNINITIALIZED:
			printf("uninitialized");
			break;
		case NODE_CONS:
			printf("cons car=%p cdr=%p",
			       n->dat.cons.car, n->dat.cons.cdr);
			break;
		case NODE_LAMBDA:
			printf("lam env=%p vars=%p expr=%p",
			       n->dat.lambda.env, n->dat.lambda.vars, n->dat.lambda.expr);
			break;
		case NODE_SYMBOL:
			printf("sym %s", n->dat.name);
			break;
		case NODE_VALUE:
			printf("value %llu", (unsigned long long) n->dat.value);
			break;
		case NODE_FOREIGN:
			printf("foreign %p", n->dat.func);
			break;
		case NODE_HANDLE:
			printf("handle %p", n->dat.handle.link);
			break;
		case NODE_CONTINUATION:
			printf("continuation %p", n->dat.cont.bt);
			break;
		case NODE_SPECIAL_FUNC:
			printf("special %d", n->dat.special);
			break;
		case NODE_BLOB:
			printf("blob addr=%p fin=%p", n->dat.blob.addr, n->dat.blob.fin);
			break;
		}
	}
	printf("\n");
}

void node_print_stream(stream_t *s, node_t *n)
{
	char buf[21], buf2[21], buf3[21];
	if(!n) {
		stream_putstr(s, "node NULL");
	} else {
		stream_putcomp(s, "node@", fmt_ptr(buf, n), NULL);

		switch(n->type) {
		case NODE_NIL:
			stream_putstr(s, "nil");
			break;
		case NODE_UNINITIALIZED:
			stream_putstr(s, "uninitialized");
			break;
		case NODE_CONS:
			stream_putcomp(s, "cons car=", fmt_ptr(buf, n->dat.cons.car),
			                  " cdr=", fmt_ptr(buf2, n->dat.cons.cdr),
			                  NULL);
			break;
		case NODE_LAMBDA:
			stream_putcomp(s, "lam env=", fmt_ptr(buf, n->dat.lambda.env),
			                  " vars=", fmt_ptr(buf2, n->dat.lambda.vars),
			                  " expr=", fmt_ptr(buf3, n->dat.lambda.expr),
			                  NULL);
			break;
		case NODE_SYMBOL:
			stream_putcomp(s, "sym ", n->dat.name, NULL);
			break;
		case NODE_VALUE:
			stream_putcomp(s, "value ", fmt_s64(buf, n->dat.value), NULL);
			break;
		case NODE_FOREIGN:
			stream_putcomp(s, "foreign ", fmt_ptr(buf, n->dat.func), NULL);
			break;
		case NODE_HANDLE:
			stream_putcomp(s, "handle ", fmt_ptr(buf, n->dat.handle.link), 
			                   NULL);
			break;
		case NODE_CONTINUATION:
			stream_putcomp(s, "cont ", fmt_ptr(buf, n->dat.cont.bt), NULL);
			break;
		case NODE_SPECIAL_FUNC:
			stream_putcomp(s, "special ", fmt_s64(buf, n->dat.special), NULL);
			break;
		case NODE_BLOB:
			stream_putcomp(s, "blob addr=", fmt_ptr(buf, n->dat.blob.addr),
			                  " fin=", fmt_ptr(buf2, n->dat.blob.fin),
			                  NULL);
			break;
		}
	}
	stream_putch(s, '\n');
}

void node_print_recursive(node_t *n)
{
	node_print(n);
	if(!n) {
		return;
	}
	if(n->type == NODE_CONS) {
		if(n->dat.cons.car) node_print_recursive(n->dat.cons.car);
		if(n->dat.cons.cdr) node_print_recursive(n->dat.cons.cdr);
	} else if(n->type == NODE_LAMBDA) {
    	if(n->dat.lambda.env) node_print(n->dat.lambda.env);
		if(n->dat.lambda.vars) node_print_recursive(n->dat.lambda.vars);
		if(n->dat.lambda.expr) node_print_recursive(n->dat.lambda.expr);
	} else if(n->type == NODE_HANDLE) {
    	if(n->dat.handle.link) node_print_recursive(n->dat.handle.link);
	} else if(n->type == NODE_CONTINUATION) {
    	if(n->dat.cont.bt) node_print_recursive(n->dat.cont.bt);
	}
}

void node_print_recursive_stream(stream_t *s, node_t *n)
{
	node_print_stream(s, n);
	if(!n) {
		return;
	}
	if(n->type == NODE_CONS) {
		if(n->dat.cons.car)
			node_print_recursive_stream(s, n->dat.cons.car);
		if(n->dat.cons.cdr)
			node_print_recursive_stream(s, n->dat.cons.cdr);
	} else if(n->type == NODE_LAMBDA) {
    	if(n->dat.lambda.env)
			node_print_stream(s, n->dat.lambda.env);
		if(n->dat.lambda.vars)
			node_print_recursive_stream(s, n->dat.lambda.vars);
		if(n->dat.lambda.expr)
			node_print_recursive_stream(s, n->dat.lambda.expr);
	} else if(n->type == NODE_HANDLE) {
    	if(n->dat.handle.link)
			node_print_recursive_stream(s, n->dat.handle.link);
	} else if(n->type == NODE_CONTINUATION) {
    	if(n->dat.cont.bt)
			node_print_recursive_stream(s, n->dat.cont.bt);
	}
}

static void node_print_list_shorthand(node_t *n)
{
	if(node_type(n) == NODE_CONS) {
		node_print_pretty(n->dat.cons.car, false);
		node_print_list_shorthand(n->dat.cons.cdr);
	} else if(n == NULL) {

	} else {
		printf(". ");
		node_print_pretty(n, false);
	}
}

static void node_print_list_shorthand_stream(stream_t *s,node_t *n)
{
	if(node_type(n) == NODE_CONS) {
		node_print_pretty_stream(s, n->dat.cons.car, false);
		node_print_list_shorthand_stream(s, n->dat.cons.cdr);
	} else if(n == NULL) {

	} else {
		stream_putstr(s, ". ");
		node_print_pretty_stream(s, n, false);
	}
}

void node_print_pretty(node_t *n, bool isverbose)
{
	switch(node_type(n)) {
	case NODE_UNINITIALIZED:
		printf("<uninitialized memory>");
		break;
	case NODE_NIL:
		printf("() ");
		break;
	case NODE_CONS:
		printf("( ");
		node_print_pretty(n->dat.cons.car, isverbose);
		if(isverbose) {
			printf(". ");
			node_print_pretty(node_cons_cdr(n), true);
		} else if(node_type(n->dat.cons.cdr) == NODE_CONS ||
		          node_type(n->dat.cons.cdr) == NODE_NIL ||
		          node_type(n->dat.cons.cdr) == NODE_LAMBDA){
			node_print_list_shorthand(n->dat.cons.cdr);
		} else {
			printf(". ");
			node_print_pretty(n->dat.cons.cdr, false);
		}
		printf(") ");
		break;
	case NODE_LAMBDA:
		if(isverbose) {
			printf("( lambda . ( ");
			node_print_pretty(n->dat.lambda.vars, true);
			printf(" . ");
			node_print_pretty(n->dat.lambda.expr, true);
			printf(") ) ");
		} else if(node_type(n->dat.lambda.vars) == NODE_SYMBOL){
			printf("( lambda ");
			node_print_pretty(n->dat.lambda.vars, false);
			node_print_list_shorthand(n->dat.lambda.expr);
			printf(") ");
		} else {
			printf("( lambda ( ");
			node_print_list_shorthand(n->dat.lambda.vars);
			printf(") ");
			node_print_list_shorthand(n->dat.lambda.expr);
			printf(") ");
		}
		break;
	case NODE_SYMBOL:
		printf("%s ", n->dat.name);
		break;
	case NODE_VALUE:
		printf("%lld ", (long long) n->dat.value);
		break;
	case NODE_FOREIGN:
		printf("foreign:%p ", n->dat.func);
		break;
	case NODE_HANDLE:
		printf("& ");
		node_print_pretty(n->dat.handle.link, isverbose);
		break;
	case NODE_CONTINUATION:
		printf("@ ");
		break;
	case NODE_SPECIAL_FUNC:
		switch(node_special_func(n)) {
		case SPECIAL_IF:
			printf("if ");
			break;
		case SPECIAL_LAMBDA:
			printf("lambda ");
			break;
		case SPECIAL_QUOTE:
			printf("quote ");
			break;
		case SPECIAL_MK_CONT:
			printf("call/cc ");
			break;
		case SPECIAL_DEF:
			printf("def! ");
			break;
		case SPECIAL_SET:
			printf("set! ");
			break;
		case SPECIAL_DEFINED:
			printf("defined? ");
			break;
		case SPECIAL_EVAL:
			printf("eval ");
		}
		break;
	case NODE_BLOB:
		printf("blob:%p ", n->dat.blob.addr);
		break;
	}
}

void node_print_pretty_stream(stream_t *s, node_t *n, bool isverbose)
{
	char buf[21];

	switch(node_type(n)) {
	case NODE_UNINITIALIZED:
		stream_putstr(s, "<uninitialized memory>");
		break;
	case NODE_NIL:
		stream_putstr(s, "() ");
		break;
	case NODE_CONS:
		stream_putstr(s, "( ");
		node_print_pretty_stream(s, n->dat.cons.car, isverbose);
		if(isverbose) {
			stream_putstr(s, ". ");
			node_print_pretty_stream(s, node_cons_cdr(n), true);
		} else if(node_type(n->dat.cons.cdr) == NODE_CONS ||
		          node_type(n->dat.cons.cdr) == NODE_NIL ||
		          node_type(n->dat.cons.cdr) == NODE_LAMBDA){
			node_print_list_shorthand_stream(s, n->dat.cons.cdr);
		} else {
			stream_putstr(s, ". ");
			node_print_pretty_stream(s, n->dat.cons.cdr, false);
		}
		stream_putstr(s, ") ");
		break;
	case NODE_LAMBDA:
		if(isverbose) {
			stream_putstr(s, "( lambda . ( ");
			node_print_pretty_stream(s, n->dat.lambda.vars, true);
			stream_putstr(s, " . ");
			node_print_pretty_stream(s, n->dat.lambda.expr, true);
			stream_putstr(s, ") ) ");
		} else if(node_type(n->dat.lambda.vars) == NODE_SYMBOL){
			stream_putstr(s, "( lambda ");
			node_print_pretty_stream(s, n->dat.lambda.vars, false);
			node_print_list_shorthand_stream(s, n->dat.lambda.expr);
			stream_putstr(s, ") ");
		} else {
			stream_putstr(s, "( lambda ( ");
			node_print_list_shorthand_stream(s, n->dat.lambda.vars);
			stream_putstr(s, ") ");
			node_print_list_shorthand_stream(s, n->dat.lambda.expr);
			stream_putstr(s, ") ");
		}
		break;
	case NODE_SYMBOL:
		stream_putcomp(s, n->dat.name, " ", NULL);
		break;
	case NODE_VALUE:
		stream_putcomp(s, fmt_s64(buf, n->dat.value), " ", NULL);
		break;
	case NODE_FOREIGN:
		stream_putcomp(s, "foreign:", fmt_ptr(buf, n->dat.func), " ", NULL);
		break;
	case NODE_HANDLE:
		stream_putstr(s, "& ");
		node_print_pretty_stream(s, n->dat.handle.link, isverbose);
		break;
	case NODE_CONTINUATION:
		stream_putstr(s, "@ ");
		break;
	case NODE_SPECIAL_FUNC:
		switch(node_special_func(n)) {
		case SPECIAL_IF:
			stream_putstr(s, "if ");
			break;
		case SPECIAL_LAMBDA:
			stream_putstr(s, "lambda ");
			break;
		case SPECIAL_QUOTE:
			stream_putstr(s, "quote ");
			break;
		case SPECIAL_MK_CONT:
			stream_putstr(s, "call/cc ");
			break;
		case SPECIAL_DEF:
			stream_putstr(s, "def! ");
			break;
		case SPECIAL_SET:
			stream_putstr(s, "set! ");
			break;
		case SPECIAL_DEFINED:
			stream_putstr(s, "defined? ");
			break;
		case SPECIAL_EVAL:
			stream_putstr(s, "eval ");
		}
		break;
	case NODE_BLOB:
		stream_putcomp(s, "blob:", fmt_ptr(buf, n->dat.blob.addr), " ", NULL);
		break;
	}
}

void node_gc(void)
{
	while(! memory_gc_iterate(&g_memstate));
}

void node_gc_print_state(void)
{
	memory_gc_print_state(&g_memstate);
}

void node_gc_statistics(
	uintptr_t *total_alloc,
	uintptr_t *total_free,
	unsigned long long *iter_count,
	unsigned long long *cycle_count)
{
	if(total_alloc) {
		*total_alloc = memory_gc_count_total(&g_memstate);
	}
	if(total_free) {
		*total_free = memory_gc_count_free(&g_memstate);
	}
	if(iter_count) {
		*iter_count = memory_gc_count_iters(&g_memstate);
	}
	if(cycle_count) {
		*cycle_count = memory_gc_count_cycles(&g_memstate);
	}
}
