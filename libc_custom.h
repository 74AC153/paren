#if ! defined (LIBC_CUSTOM_H)
#define LIBC_CUSTOM_H

#include <stdint.h>

long long int strtoll_custom(const char *str, char **end);
char *fmt_u64(char buf[17], uint64_t val); /* hex */
char *fmt_ptr(char buf[21], void *val); /* hex */
char *fmt_s64(char buf[21], int64_t val); /* dec */

void bzero_custom(void *s, size_t len);
char *strncpy_custom(char *restrict s1, const char *restrict s2, size_t n);
int strcmp_custom(const char *s1, const char *s2);

#endif
