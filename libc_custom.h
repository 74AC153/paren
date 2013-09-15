#if ! defined (LIBC_CUSTOM_H)
#define LIBC_CUSTOM_H

#include <stdint.h>

long long int strtoll_custom(const char *str, char **end);
char *u64_to_str16(char buf[17], uint64_t val);
char *s64_to_str10(char buf[21], int64_t val);

void bzero_custom(void *s, size_t len);
char *strncpy_custom(char *restrict s1, const char *restrict s2, size_t n);
int strcmp_custom(const char *s1, const char *s2);

#endif
