#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
	int fd;
	void *buf;
	size_t len;
	int err;
} filemap_info_t;

int map_file(filemap_info_t *info, char *path)
{
	struct stat sb;
	int status = 0;

	info->fd = -1;
	info->buf = NULL;
	info->len = 0;
	info->err = 0;

	info->fd = open(path, O_RDONLY);
	if(info->fd < 0) {
		status = -1;
		info->err = errno;
		goto finish;
	}

	status = fstat(info->fd, &sb);
	if(status < 0) {
		status = -2;
		info->err = errno;
		goto finish;
	}
	info->len = sb.st_size;

	info->buf = mmap(NULL, info->len, PROT_READ, MAP_PRIVATE, info->fd, 0);
	if(info->buf == MAP_FAILED) {
		status = -3;
		info->err = errno;
		goto finish;
	}

finish:
	return status;
}

void unmap_file(filemap_info_t *info)
{
	if(info->buf != NULL && info->buf != MAP_FAILED) {
		munmap(info->buf, info->len);
	}
	if(info->fd >= 0) {
		close(info->fd);
	}
}

