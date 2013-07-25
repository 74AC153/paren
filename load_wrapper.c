#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "load_wrapper.h"

int load_wrapper(char *path, ldwrap_ent_t **funs)
{
	void *dl_hdl;
	struct { char *name, *nmemonic; } *fun_names;
	size_t *fun_count;
	size_t i;

	dl_hdl = dlopen(path, RTLD_LAZY);

	if(! dl_hdl) {
		fprintf(stderr, "error: dlopen() failed: %s\n", dlerror());
		return -1;
	}

	fun_names = dlsym(dl_hdl, load_wrapper_names_ident);
	if(! fun_names) {
		fprintf(stderr, "dlsym(%s) failed: %s\n",
		        load_wrapper_names_ident,
		        dlerror());
		return -1;
	}

	fun_count = (size_t *) dlsym(dl_hdl, load_wrapper_count_ident);
	if(! fun_count) {
		fprintf(stderr, "dlsym(%s) failed: %s\n",
		        load_wrapper_count_ident,
		        dlerror());
		return -1;
	}

	*funs = malloc(*fun_count * sizeof(ldwrap_ent_t) + 1);
	if(! funs) {
		fprintf(stderr, "malloc() failed\n");
		return -1;
	}

	for(i = 0; i < *fun_count; i++) {
		(*funs)[i].name = fun_names[i].nmemonic;
		(*funs)[i].fun = dlsym(dl_hdl, fun_names[i].name);
	}
	(*funs)[i].name = NULL;
	(*funs)[i].fun = NULL;

	return 0;
}
