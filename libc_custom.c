#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc_custom.h"

static int char_to_num(char c)
{
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'A' && c <= 'Z') return c - 'A' + 10;
	if(c >= 'a' && c <= 'z') return c - 'a' + 10;
	return -1;
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
		}
		str++;
	} else {
		base = 10;
	}

	if(((cval = char_to_num(*str)) >= 0)
	   && (cval < base)) {
		val = cval;
		str++;
	}
	while(((cval = char_to_num(*str)) >= 0)
	      && (cval < base)) {
		val = val * base + cval;
		str++;
	}

	if(end) {
		*end = (char *) str;
	}

	if(neg) val = -val;
	return val;
}

static char num_to_char(unsigned int n)
{
	if(n < 10) return '0' + n;
	if(n < 36) return 'a' + n - 10;
	return 0;
}

/* reverse string from start to limit-1 */
static void str_rev(char *start, char *limit)
{
	char temp;
	char *last = limit - 1;

	while(start < last) {
		temp = *start;
		*start = *last;
		*last= temp;
		start++;
		last--;
	}

}

char *fmt_u64(char buf[17], uint64_t val)
{
	char *cursor = &(buf[0]);
	
	*cursor++ = 0;
	if(val) {
		while(val) {
			*cursor++ = num_to_char(val & 0xF);
			val >>= 4;
		}
	} else {
		*cursor++ = '0';
	}

	str_rev(&(buf[0]), cursor);

	return &(buf[0]);
}

char *fmt_ptr(char buf[17], void *val)
{
	char *cursor = &(buf[0]);
	uintptr_t _val = (uintptr_t ) val;
	unsigned int i;
	*cursor++ = 0;
	for(i = 0; i < sizeof(uintptr_t) * 2; i++) {
		*cursor++ = num_to_char(_val & 0xF);
		_val >>= 4;
	}
	str_rev(&(buf[0]), cursor);

	return &(buf[0]);
}

char *fmt_s64(char buf[21], int64_t val)
{
	char *cursor = &(buf[0]);
	bool neg = val < 0;
	
	*cursor++ = 0;
	if(val) {
		while(val) {
			*cursor++ = num_to_char(val % 10);
			val /= 10;
		}
		if(neg) {
			*cursor++ = '-';
		}
	} else {
		*cursor++ = '0';
	}

	str_rev(&(buf[0]), cursor);

	return &(buf[0]);
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
