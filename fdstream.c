#include <unistd.h>
#include <errno.h>
#include "fdstream.h"

static int fdstream_readch(void *p, char *ch_out)
{
	unsigned char c;
	int status;

	fdstream_t *fs = (fdstream_t *) p;
	
	status = read(fs->fd, ch_out, 1);
	if(status == 0) {
		return STREAM_END;
	} else if(status < 0) {
		return errno;
	}
	return 0;
}

static int fdstream_writech(void *p, char ch_in)
{
	int status;
	fdstream_t *fs = (fdstream_t *) p;

	status = write(fs->fd, &ch_in, 1);
	if(status != 1) {
		return errno;
	}
	return 0;
}

static fdstream_t fdstream_stdin = {
	.hdr = STREAM_STATIC_INITIALIZER(&fdstream_stdin, fdstream_readch, NULL),
	.fd = STDIN_FILENO
};
stream_t *g_stream_stdin = &(fdstream_stdin.hdr);

static fdstream_t fdstream_stdout = {
	.hdr = STREAM_STATIC_INITIALIZER(&fdstream_stdin, NULL, fdstream_writech),
	.fd = STDOUT_FILENO
};
stream_t *g_stream_stdout = &(fdstream_stdout.hdr);

static fdstream_t fdstream_stderr = {
	.hdr = STREAM_STATIC_INITIALIZER(&fdstream_stdin, NULL, fdstream_writech),
	.fd = STDERR_FILENO
};
stream_t *g_stream_stderr = &(fdstream_stderr.hdr);

stream_t *fdstream_init(void *p, int fd)
{
	fdstream_t *fs = (fdstream_t *) p;
	if(fs) {
		fs->fd = fd;
		stream_init(&(fs->hdr), fdstream_readch, fdstream_writech, fs);
	}
	return &(fs->hdr);
}

