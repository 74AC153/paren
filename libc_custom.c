#include <stdbool.h>
#include <stddef.h>
#include "utils.h"

static unsigned int char_to_num(char c)
{
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'A' && c <= 'Z') return c - 'A' + 10;
	if(c >= 'a' && c <= 'z') return c - 'a' + 10;
	return (unsigned int) -1;
}

long long int strtoll_custom(const char *str, char **end)
{
	long long int val = 0;
	int cval, base;
	bool neg = false;

	if(str[0] == '-') {
		neg = true;
		str++;
	} else if(str[0] == '+') {
		str++;
	}

	/* autodetect base */
	if(str[0] == '0') {
		if(str[1] == 'x') {
			base = 16;
			str++;
		} else {
			base = 8;
			str++;
		}
	} else {
		base = 10;
	}

	if(*str && ((cval = char_to_num(*str)) < base)) {
		val = cval;
		str++;
	}
	while(*str && ((cval = char_to_num(*str)) < base)) {
		val = val * base + cval;
		str++;
	}

	if(end) {
		*end = (char *) str;
	}

	if(neg) val = -val;
	return val;
}

void bzero_custom(void *s, size_t len)
{
	for( ; len; len--) {
		*(char *)s++ = 0;
	}
}

char *strncpy_custom(char *restrict s1, const char *restrict s2, size_t n)
{
	while(n) {
		*s1 = *s2;
		if(! *s2) {
			break;
		}
		n--;
		s1++;
		s2++;
	}
	while(n) {
		*s1++ = 0;
		n--;
	}

	return s1;
}

int strcmp_custom(const char *s1, const char *s2)
{
	if(s1 == s2) {
		return 0;
	}
	while(*s1 && *s2 && *s1 == *s2){
		s1++;
		s2++;
	}
	return *s2 - *s1;
}
