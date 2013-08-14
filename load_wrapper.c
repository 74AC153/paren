#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>

#include "load_wrapper.h"

#define PATH_SEP '/'
#define FEXT_SEP '.'

int load_wrapper(char *path, ldwrap_ent_t **funs)
{
	void *dl_hdl;
	struct { char *name, *nmemonic; } *fun_names;
	size_t *fun_count;
	size_t i;
	char *names_ident;
	size_t names_ident_len;
	char *count_ident;
	size_t count_ident_len;
	char *name;
	size_t name_len;
	char *start, *end;

	/* library name: chars between last '/' and first '.' after it */
	start = strrchr(path, PATH_SEP);
	if(! start) {
		start = path;
	}
	end = strchr(start, FEXT_SEP);
	if(!end) {
		end = path + strlen(path);
	}
	name_len = end - start;
	name = alloca(name_len + 1);
	strncpy(name, start, end-start);
	name[name_len] = 0;

	dl_hdl = dlopen(path, RTLD_LAZY);
	if(! dl_hdl) {
		fprintf(stderr, "error: dlopen() failed: %s\n", dlerror());
		return -1;
	}

	names_ident_len = name_len + strlen(load_wrapper_names_ident) + 2;
	names_ident = alloca(names_ident_len);
	snprintf(names_ident, names_ident_len,
	         "%s_%s", name, load_wrapper_names_ident);
	fun_names = dlsym(dl_hdl, names_ident);
	if(! fun_names) {
		fprintf(stderr, "dlsym(%s) failed: %s\n",
		        names_ident, dlerror());
		return -1;
	}

	count_ident_len = name_len + strlen(load_wrapper_count_ident) + 2;
	count_ident = alloca(count_ident_len);
	snprintf(count_ident, count_ident_len,
	         "%s_%s", name, load_wrapper_count_ident);
	fun_count = dlsym(dl_hdl, count_ident);
	if(! fun_count) {
		fprintf(stderr, "dlsym(%s) failed: %s\n",
		        count_ident, dlerror());
		return -1;
	}

	*funs = malloc((*fun_count + 1) * sizeof((*funs)[0]));
	if(! *funs) {
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
