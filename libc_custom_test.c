#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "utils.h"

void bytes(unsigned char *str, size_t len)
{
	while(len--) {
		printf("%x ", *str++);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	long long int x;
	unsigned char str[] = "hello world";
	unsigned char str2[] = "goodbye world";
	unsigned char str3[] = "hello world";
	unsigned char str4[] = "hello cruel world";

	printf("%lld\n", strtoll_custom("0", NULL));
	printf("%lld\n", strtoll_custom("10", NULL));
	printf("%lld\n", strtoll_custom("210", NULL));
	printf("%lld\n", strtoll_custom("3210", NULL));
	printf("%lld\n", strtoll_custom("43210", NULL));
	printf("%lld\n", strtoll_custom("543210", NULL));
	printf("%lld\n", strtoll_custom("6543210", NULL));
	printf("%lld\n", strtoll_custom("76543210", NULL));
	printf("%lld\n", strtoll_custom("876543210", NULL));
	printf("%lld\n", strtoll_custom("9876543210", NULL));
	printf("%lld\n", strtoll_custom("109876543210", NULL));
	printf("%lld\n", strtoll_custom("11109876543210", NULL));
	printf("%lld\n", strtoll_custom("1211109876543210", NULL));
	printf("%lld\n", strtoll_custom("131211109876543210", NULL));

	printf("%lld\n", strtoll_custom("9223372036854775807", NULL));
	printf("%lld\n", strtoll_custom("-9223372036854775808", NULL));

	printf("%d\n", strcmp_custom((char*) str, (char*) str2));
	printf("%d\n", strcmp_custom((char*) str2, (char*) str));
	printf("%d\n", strcmp_custom((char*) str, (char*) str4));
	printf("%d\n", strcmp_custom((char*) str4, (char*) str));

	printf("%d\n", strcmp_custom((char*) str, (char*) str2));
	printf("%d\n", strcmp_custom((char*) str, (char*) str3));
	printf("%d\n", strcmp_custom((char*) str, (char*) str));

	bytes(str2, sizeof(str2));
	strncpy_custom((char *) str2, (char *) str, sizeof(str));
	bytes(str2, sizeof(str2));

	bytes(str, sizeof(str));
	bzero_custom(str, sizeof(str));
	bytes(str, sizeof(str));
	
	return 0;
}
