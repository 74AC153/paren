#if ! defined (UTILS_H)
#define UTILS_H

long long int strtoll_custom(const char *str, char **end);
void bzero_custom(void *s, size_t len);
char *strncpy_custom(char *restrict s1, const char *restrict s2, size_t n);
int strcmp_custom(const char *s1, const char *s2);

#endif
