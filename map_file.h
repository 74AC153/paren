#if ! defined(MAP_FILE_H)
#define MAP_FILE_H

typedef struct {
	int fd;
	void *buf;
	size_t len;
	int err;
} filemap_info_t;

int map_file(filemap_info_t *info, char *path);

void unmap_file(filemap_info_t *info);

/* usage:

	filemap_info_t info;

	status = map_file(&info, argv[1]);
	switch(status) {
	default: break;
	case -1:
		fprintf(stderr, "error: cannot open() file %s: %s (errno=%d)\n",
		        argv[1], strerror(info.err), info.err);		
		goto cleanup;
	case -2:
		fprintf(stderr, "error: canno stat() file %s: %s (errno=%d)\n",
		        argv[1], strerror(info.err), info.err);		
		goto cleanup;
	case -3:
		fprintf(stderr, "error: cannot mmap() file %s: %s (errno=%d)\n",
		        argv[1], strerror(info.err), info.err);		
		goto cleanup;
	}


then later ...

	unmap_file(&info);

*/

#endif
