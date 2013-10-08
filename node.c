#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "dlist.h"
#include "node.h"
#include "memory.h"
#include "libc_custom.h"
#include "stream.h"
#include "traceclass.h"
#include "dbgtrace.h"
#include "allocator_def.h"

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

static void links_cb(void (*cb)(void *link, void *p), void *data, void *p)
{
	node_t *n = (node_t *) data;

	DBGSTMT({
		switch(node_type(n)) {
		case NODE_CONS:
		case NODE_LAMBDA:
		case NODE_HANDLE:
		case NODE_CONTINUATION:
			DBGTRACE(TC_NODE_GC, "node links_cb: ");
			DBGRUN(TC_NODE_GC, { node_print_stream(dbgtrace_getstream(), n); });
		default:
			break;
		}
	});

	switch(node_type(n)) {
	case NODE_CONS:
		cb(n->dat.cons.car, p);
		cb(n->dat.cons.cdr, p);
		break;
	case NODE_LAMBDA:
		cb(n->dat.lambda.env, p);
		cb(n->dat.lambda.vars, p);
		cb(n->dat.lambda.expr, p);
		break;
	case NODE_HANDLE:
		cb(n->dat.handle.link, p);
		break;
	case NODE_CONTINUATION:
		cb(n->dat.handle.link, p);
		break;
	default:
		break;
	}
}

static void node_print_wrap(void *p, stream_t *stream)
{
	node_t *n = (node_t *) p;
	node_print_stream(stream, n);
}

static void node_init_cb(void *p)
{
	node_t *n = (node_t *) p;
	bzero_custom(n, sizeof(*n));
	n->type = NODE_UNINITIALIZED;
}

memory_state_t *node_memstate_init(
	memory_state_t *s,
	mem_allocator_fn_t allocfn,
	mem_free_fn_t freefn,
	void *alloc_priv)
{
	memory_state_init(s,
	                  node_init_cb,
	                  links_cb,
	                  node_print_wrap,
	                  allocfn,
	                  freefn,
	                  alloc_priv);
	return s;
}

memory_state_t *node_memstate(node_t *n)
{
	return data_to_memstate(n);
}

#if defined(NODE_NO_INCREMENTAL_GC)
	#define NODE_GC_ITERATE(STATE)
#else
	#if defined(NODE_INCREMENTAL_FULL_GC)
	#define NODE_GC_ITERATE(STATE) \
		do { \
			memory_gc_cycle(STATE); \
			memory_gc_cycle(STATE); \
		} while(0)
	#else
	#define NODE_GC_ITERATE(STATE) memory_gc_iterate(STATE)
	#endif
#endif

static node_t *node_new(memory_state_t *s)
{
	node_t *n;
	n = memory_request(s, sizeof(node_t));
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
		memory_gc_advise_new_link(data_to_memstate(n), n);
	}
	return n;
}

static void node_release(node_t *n)
{
	if(n) {
		memory_gc_advise_stale_link(data_to_memstate(n), n);
	}
}

static void node_droproot_int(node_t *n)
{
	if(n && memory_gc_isroot(data_to_memstate(n), n)) {
		memory_gc_unlock(data_to_memstate(n), n);
	}
}

void node_droproot(node_t *n)
{
	node_droproot_int(n);
	NODE_GC_ITERATE(data_to_memstate(n));
}

node_t *node_lockroot(node_t *n)
{
	if(n) {
		memory_gc_lock(data_to_memstate(n), n);
		NODE_GC_ITERATE(data_to_memstate(n));
	}
	return n;
}

bool node_isroot(node_t *n)
{
	NODE_GC_ITERATE(data_to_memstate(n));
	return memory_gc_isroot(data_to_memstate(n), n);
}

bool node_islocked(node_t *n)
{
	NODE_GC_ITERATE(data_to_memstate(n));
	return memory_gc_is_locked(n);
}

node_t *node_cons_new(memory_state_t *s, node_t *car, node_t *cdr)
{
	node_t *ret = node_new(s);
	assert(ret);
	ret->dat.cons.car = node_retain(car);
	ret->dat.cons.cdr = node_retain(cdr);
	ret->type = NODE_CONS;
	DBGTRACE(TC_NODE_INIT, "node init: ");
	DBGRUN(TC_NODE_INIT, { node_print_stream(dbgtrace_getstream(), ret); });
	NODE_GC_ITERATE(s);
	if(car) NODE_GC_ITERATE(data_to_memstate(car));
	if(cdr) NODE_GC_ITERATE(data_to_memstate(cdr));
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
	NODE_GC_ITERATE(data_to_memstate(n));
}

void node_cons_patch_cdr(node_t *n, node_t *newcdr)
{
	node_t *oldcdr;

	assert(n);
	assert(n->type == NODE_CONS);

	oldcdr = n->dat.cons.cdr;
	n->dat.cons.cdr = node_retain(newcdr);

	node_release(oldcdr);
	NODE_GC_ITERATE(data_to_memstate(n));
}

node_t *node_cons_car(node_t *n)
{
	node_t *ret = NULL;
	if(n) {
		assert(n->type == NODE_CONS);
		ret = n->dat.cons.car;
		NODE_GC_ITERATE(data_to_memstate(n));
	}
	return ret;
}

node_t *node_cons_cdr(node_t *n)
{
	node_t *ret = NULL;
	if(n) {
		assert(n->type == NODE_CONS);
		ret = n->dat.cons.cdr;
		NODE_GC_ITERATE(data_to_memstate(n));
	}
	return ret;
}

node_t *node_lambda_new(memory_state_t *s, node_t *env, node_t *vars, node_t *expr)
{
	node_t *ret = node_new(s);
	assert(ret);
	ret->dat.lambda.env = node_retain(env);
	ret->dat.lambda.vars = node_retain(vars);
	ret->dat.lambda.expr = node_retain(expr);
	ret->type = NODE_LAMBDA;
	DBGTRACE(TC_NODE_INIT, "node init: ");
	DBGRUN(TC_NODE_INIT, { node_print_stream(dbgtrace_getstream(), ret); });
	NODE_GC_ITERATE(s);
	if(env) NODE_GC_ITERATE(data_to_memstate(env));
	if(vars) NODE_GC_ITERATE(data_to_memstate(vars));
	if(expr) NODE_GC_ITERATE(data_to_memstate(expr));
	return ret;
}

node_t *node_lambda_env(node_t *n)
{
	assert(n->type = NODE_LAMBDA);
	NODE_GC_ITERATE(data_to_memstate(n));
	return n->dat.lambda.env;
}

node_t *node_lambda_vars(node_t *n)
{
	assert(n->type = NODE_LAMBDA);
	NODE_GC_ITERATE(data_to_memstate(n));
	return n->dat.lambda.vars;
}

node_t *node_lambda_expr(node_t *n)
{
	assert(n->type = NODE_LAMBDA);
	NODE_GC_ITERATE(data_to_memstate(n));
	return n->dat.lambda.expr;
}

node_t *node_value_new(memory_state_t *s, value_t val)
{
	node_t *ret = node_new(s);
	assert(ret);
	ret->dat.value = val;
	ret->type = NODE_VALUE;
	DBGTRACE(TC_NODE_INIT, "node init: ");
	DBGRUN(TC_NODE_INIT, { node_print_stream(dbgtrace_getstream(), ret); });
	NODE_GC_ITERATE(s);
	return ret;
}

value_t node_value(node_t *n)
{
	assert(n->type == NODE_VALUE);
	NODE_GC_ITERATE(data_to_memstate(n));
	return n->dat.value;
}

node_t *node_symbol_new(memory_state_t *s, char *name)
{
	node_t *ret = node_new(s);
	assert(ret);
	strncpy_custom(ret->dat.name, name, sizeof(ret->dat.name));
	ret->type = NODE_SYMBOL;
	DBGTRACE(TC_NODE_INIT, "node init: ");
	DBGRUN(TC_NODE_INIT, { node_print_stream(dbgtrace_getstream(), ret); });
	NODE_GC_ITERATE(s);
	return ret;
}

char *node_symbol_name(node_t *n)
{
	assert(n->type == NODE_SYMBOL);
	NODE_GC_ITERATE(data_to_memstate(n));
	return n->dat.name;
}

node_t *node_foreign_new(memory_state_t *s, foreign_t func)
{
	node_t *ret = node_new(s);
	assert(ret);
	ret->dat.func = func;
	ret->type = NODE_FOREIGN;
	DBGTRACE(TC_NODE_INIT, "node init: ");
	DBGRUN(TC_NODE_INIT, { node_print_stream(dbgtrace_getstream(), ret); });
	NODE_GC_ITERATE(s);
	return ret;
}

foreign_t node_foreign_func(node_t *n)
{
	assert(n->type == NODE_FOREIGN);
	NODE_GC_ITERATE(data_to_memstate(n));
	return n->dat.func;
}

node_t *node_handle_new(memory_state_t *s, node_t *link)
{
	node_t *ret = node_new(s);
	assert(ret);
	ret->type = NODE_HANDLE;
	ret->dat.handle.link = node_retain(link);
	DBGTRACE(TC_NODE_INIT, "node init: ");
	DBGRUN(TC_NODE_INIT, { node_print_stream(dbgtrace_getstream(), ret); });
	NODE_GC_ITERATE(s);
	if(link) NODE_GC_ITERATE(data_to_memstate(link));
	return ret;
}

node_t *node_handle(node_t *n)
{
	assert(node_type(n) == NODE_HANDLE);
	NODE_GC_ITERATE(data_to_memstate(n));
	return n->dat.handle.link;
}

void node_handle_update(node_t *n, node_t *newlink)
{
	node_t *oldlink;
	assert(node_type(n) == NODE_HANDLE);

	oldlink = n->dat.handle.link;
	n->dat.handle.link = node_retain(newlink);

	node_release(oldlink);
	NODE_GC_ITERATE(data_to_memstate(n));
	if(newlink) NODE_GC_ITERATE(data_to_memstate(newlink));
}

node_t *node_cont_new(memory_state_t *s, node_t *bt)
{
	node_t *ret = node_new(s);
	assert(ret);
	ret->type = NODE_CONTINUATION;
	ret->dat.cont.bt = node_retain(bt);
	DBGTRACE(TC_NODE_INIT, "node init: ");
	DBGRUN(TC_NODE_INIT, { node_print_stream(dbgtrace_getstream(), ret); });
	NODE_GC_ITERATE(s);
	if(bt) NODE_GC_ITERATE(data_to_memstate(bt));
	return ret;
}

node_t *node_cont(node_t *n)
{
	assert(node_type(n) == NODE_CONTINUATION);
	NODE_GC_ITERATE(data_to_memstate(n));
	return n->dat.cont.bt;
}

node_t *node_special_func_new(memory_state_t *s, special_func_t func)
{
	node_t *ret = node_new(s);
	assert(ret);
	ret->type = NODE_SPECIAL_FUNC;
	ret->dat.special = func; 
	DBGTRACE(TC_NODE_INIT, "node init: ");
	DBGRUN(TC_NODE_INIT, { node_print_stream(dbgtrace_getstream(), ret); });
	NODE_GC_ITERATE(s);
	return ret;

}

special_func_t node_special_func(node_t *n)
{
	assert(node_type(n) == NODE_SPECIAL_FUNC);
	NODE_GC_ITERATE(data_to_memstate(n));
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

node_t *node_blob_new(memory_state_t *s, void *addr, blob_fin_t fin, uintptr_t sig)
{
	node_t *ret = node_new(s);
	assert(ret);
	memory_set_finalizer(ret, blob_fin_wrap);
	ret->type = NODE_BLOB;
	ret->dat.blob.addr = addr; 
	ret->dat.blob.fin = fin;
	ret->dat.blob.sig = sig;
	DBGTRACE(TC_NODE_INIT, "node init: ");
	DBGRUN(TC_NODE_INIT, { node_print_stream(dbgtrace_getstream(), ret); });
	NODE_GC_ITERATE(s);
	return ret;
}

void *node_blob_addr(node_t *n)
{
	assert(node_type(n) == NODE_BLOB);
	NODE_GC_ITERATE(data_to_memstate(n));
	return n->dat.blob.addr;
}

uintptr_t node_blob_sig(node_t *n)
{
	assert(node_type(n) == NODE_BLOB);
	NODE_GC_ITERATE(data_to_memstate(n));
	return n->dat.blob.sig;
}

void node_print_stream(stream_t *s, node_t *n)
{
	char buf[21], buf2[21], buf3[21];
	if(!n) {
		stream_putstr(s, "node NULL");
	} else {
		stream_put(s, "node@", fmt_ptr(buf, n), " ", NULL);

		switch(n->type) {
		case NODE_NIL:
			stream_putstr(s, "nil");
			break;
		case NODE_UNINITIALIZED:
			stream_putstr(s, "uninitialized");
			break;
		case NODE_CONS:
			stream_put(s, "cons car=", fmt_ptr(buf, n->dat.cons.car),
			              " cdr=", fmt_ptr(buf2, n->dat.cons.cdr),
			              NULL);
			break;
		case NODE_LAMBDA:
			stream_put(s, "lam env=", fmt_ptr(buf, n->dat.lambda.env),
			              " vars=", fmt_ptr(buf2, n->dat.lambda.vars),
			              " expr=", fmt_ptr(buf3, n->dat.lambda.expr),
			              NULL);
			break;
		case NODE_SYMBOL:
			stream_put(s, "sym '", n->dat.name, "'", NULL);
			break;
		case NODE_VALUE:
			stream_put(s, "value ", fmt_s64(buf, n->dat.value), NULL);
			break;
		case NODE_FOREIGN:
			stream_put(s, "foreign ", fmt_ptr(buf, n->dat.func), NULL);
			break;
		case NODE_HANDLE:
			stream_put(s, "handle lnk=", fmt_ptr(buf, n->dat.handle.link), 
			                   NULL);
			break;
		case NODE_CONTINUATION:
			stream_put(s, "cont bt=", fmt_ptr(buf, n->dat.cont.bt), NULL);
			break;
		case NODE_SPECIAL_FUNC:
			stream_put(s, "special ", fmt_s64(buf, n->dat.special), NULL);
			break;
		case NODE_BLOB:
			stream_put(s, "blob addr=", fmt_ptr(buf, n->dat.blob.addr),
			              " fin=", fmt_ptr(buf2, n->dat.blob.fin),
			              " sig=", fmt_ptr(buf3, (void*) n->dat.blob.sig),
			              NULL);
			break;
		}
	}
	stream_putch(s, '\n');
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
		stream_put(s, n->dat.name, " ", NULL);
		break;
	case NODE_VALUE:
		stream_put(s, fmt_s64(buf, n->dat.value), " ", NULL);
		break;
	case NODE_FOREIGN:
		stream_put(s, "foreign:", fmt_ptr(buf, n->dat.func), " ", NULL);
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
		stream_put(s, "blob:", fmt_ptr(buf, n->dat.blob.addr), " ", NULL);
		break;
	}
}
